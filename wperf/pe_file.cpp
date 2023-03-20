// BSD 3-Clause License
//
// Copyright (c) 2022, Arm Limited
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <fstream>
#include <iostream>
#include "dia2.h"

#include "exception.h"
#include "wperf-common\macros.h"
#include "utils.h"
#include "pe_file.h"


std::wstring gen_pdb_name(std::wstring str)
{
    return ReplaceFileExtension(str, L"pdb");
}

void parse_pe_file(std::wstring pe_file, PeFileMetaData& pefile_metadata)
{
    pefile_metadata.pe_name = pe_file;

    parse_pe_file(pe_file,
        pefile_metadata.static_entry_point,
        pefile_metadata.image_base,
        pefile_metadata.sec_info,
        pefile_metadata.sec_import);
}

void parse_pe_file(std::wstring pe_file, uint64_t& static_entry_point, uint64_t& image_base, std::vector<SectionDesc>& sec_info, std::vector<std::wstring>& sec_import)
{
    std::ifstream pe_file_stream(pe_file, std::ios::binary);
    // make sure we have enough space for maximum read
    std::streamsize buf_size = sizeof(IMAGE_DOS_HEADER) > sizeof(IMAGE_NT_HEADERS)
        ? sizeof(IMAGE_DOS_HEADER) : sizeof(IMAGE_NT_HEADERS);

    char* hdr_buf = new char[buf_size];
    pe_file_stream.read(hdr_buf, sizeof(IMAGE_DOS_HEADER));

    PIMAGE_DOS_HEADER dos_hdr = reinterpret_cast<PIMAGE_DOS_HEADER>(hdr_buf);
    if (dos_hdr->e_magic != IMAGE_DOS_SIGNATURE)
    {
        delete[] hdr_buf;
        std::wcout << pe_file << std::endl;
        throw fatal_exception("PE file specified is not in valid PE format");
    }

    std::streampos pos = dos_hdr->e_lfanew;
    pe_file_stream.seekg(pos);
    pe_file_stream.read(hdr_buf, sizeof(IMAGE_NT_HEADERS));
    PIMAGE_NT_HEADERS nt_hdr = reinterpret_cast<PIMAGE_NT_HEADERS>(hdr_buf);
    if (nt_hdr->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
    {
        delete[] hdr_buf;
        throw fatal_exception("PE file specified is not 64bit format");
    }

    static_entry_point = nt_hdr->OptionalHeader.AddressOfEntryPoint;
    image_base = nt_hdr->OptionalHeader.ImageBase;
    //std::cout << "static entry: 0x" << std::hex << static_entry_point << ", image_base: 0x" << image_base << std::endl;

    buf_size = sizeof(IMAGE_SECTION_HEADER) * nt_hdr->FileHeader.NumberOfSections;
    char* sec_buf = new char[buf_size];
    pos += FIELD_OFFSET(IMAGE_NT_HEADERS, OptionalHeader) + nt_hdr->FileHeader.SizeOfOptionalHeader;
    pe_file_stream.seekg(pos);
    pe_file_stream.read(sec_buf, buf_size);

    ULONGLONG importDirectoryRVA = nt_hdr->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
    PIMAGE_SECTION_HEADER importSection = NULL;

    PIMAGE_SECTION_HEADER sections = reinterpret_cast<PIMAGE_SECTION_HEADER>(sec_buf);
    for (uint32_t i = 0; i < nt_hdr->FileHeader.NumberOfSections; i++)
    {
        char section_name[16] = { 0 };
        memcpy_s(section_name, 16, sections[i].Name, IMAGE_SIZEOF_SHORT_NAME);

        std::string name(section_name);
        SectionDesc sec_desc = { i, sections[i].VirtualAddress, sections[i].Misc.VirtualSize, std::wstring(name.begin(), name.end()) };
        sec_info.push_back(sec_desc);
        //std::cout << "addr: 0x" << std::hex << sec_desc.offset << ", name: " <<  sec_desc.name << std::endl;

        if (importDirectoryRVA >= sections[i].VirtualAddress && importDirectoryRVA < sections[i].VirtualAddress + sections[i].Misc.VirtualSize) {
            importSection = sections + i;
        }
    }

	/** DLL IMPORTS **/
	if (importSection)
	{
		pos = importSection->PointerToRawData;
		pe_file_stream.seekg(0, pe_file_stream.end);
		std::streampos length = pe_file_stream.tellg();
		std::streampos length_to_read = length - pos;
		pe_file_stream.seekg(pos);

		char* import_buf = new char[length_to_read];

		pe_file_stream.read(import_buf, length_to_read);

		PIMAGE_IMPORT_DESCRIPTOR importDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)(import_buf + (nt_hdr->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress - importSection->VirtualAddress));

		for (; importDescriptor->Name != 0; importDescriptor++) {
			std::string name = import_buf + (importDescriptor->Name - importSection->VirtualAddress);
			sec_import.push_back(std::wstring(name.begin(), name.end()));
			// std::cout << "dll: " << name << std::endl;
		}

		delete[] import_buf;
	}

    pe_file_stream.close();
    delete[] hdr_buf;
    delete[] sec_buf;
}


void parse_pdb_file(std::wstring pdb_file, std::vector<FuncSymDesc>& sym_info, bool sample_display_short)
{
    // Init DIA COM
    IDiaDataSource* DiaDataSource;
    IDiaSession* DiaSession;
    IDiaSymbol* DiaSymbol;

    HRESULT status = CoInitialize(NULL);
    status = CoCreateInstance(__uuidof(DiaSource), NULL,
        CLSCTX_INPROC_SERVER, __uuidof(IDiaDataSource), (void**)&DiaDataSource);

    if (status != S_OK)
    {
        if (status == REGDB_E_CLASSNOTREG)
        {
            std::cout << "Status REGDB_E_CLASSNOTREG indicates that DIA SDK class is not registered!" << std::endl;
            std::cout << "See https://learn.microsoft.com/en-us/visualstudio/debugger/debug-interface-access/getting-started-debug-interface-access-sdk?view=vs-2019" << std::endl;
            std::cout << "Try registering this service with command:" << std::endl;
            std::cout << std::endl;
            std::cout << "\t" << "C:\\Program Files (x86)\\Microsoft Visual Studio\\2017\\Professional\\DIA SDK\\bin>regsvr32 msdia140.dll" << std::endl;
        }
        else
            std::cout << "status: 0x" << std::hex << status << std::endl;

        throw fatal_exception("CoCreateInstance failed for DIA");
    }

    status = DiaDataSource->loadDataFromPdb(pdb_file.c_str());
    if (status < 0)
        throw fatal_exception("loadDataFromPdb failed for the PDB file");

    status = DiaDataSource->openSession(&DiaSession);
    if (status < 0)
        throw fatal_exception("openSession failed for DiaSession");

    status = DiaSession->get_globalScope(&DiaSymbol);
    if (status != S_OK)
        throw fatal_exception("get_globalScope failed for DiaSymbol");

    IDiaEnumSymbols* objs;
    if (DiaSymbol->findChildren(SymTagCompiland, NULL, nsNone, &objs) < 0)
        throw fatal_exception("Failed to find obj from pdb");

    IDiaSymbol* obj;
    ULONG done_obj_num = 0;
    while (objs->Next(1, &obj, &done_obj_num) >= 0 && done_obj_num == 1)
    {
        std::wstring obj_name_wstr;
        BSTR obj_name;

        if (obj->get_name(&obj_name) != S_OK)
        {
            obj_name_wstr = std::wstring(L"unknown");
        }
        else
        {
            obj_name_wstr = std::wstring(obj_name);
            SysFreeString(obj_name);
        }

        IDiaEnumSymbols* symbols;
        if (obj->findChildren(SymTagNull, NULL, nsNone, &symbols) >= 0)
        {
            IDiaSymbol* symbol;
            ULONG done_sym_num = 0;
            while (symbols->Next(1, &symbol, &done_sym_num) >= 0 && done_sym_num == 1)
            {
                DWORD sym_tag;
                if (symbol->get_symTag(&sym_tag) == S_OK && sym_tag == SymTagFunction)
                {
                    DWORD loc_type;
                    if (symbol->get_locationType(&loc_type) == S_OK && loc_type == LocIsStatic)
                    {
                        std::wstring func_name_wstr;
                        BSTR func_name;
                        if ((!sample_display_short && symbol->get_undecoratedName(&func_name) == S_OK)
                            || symbol->get_name(&func_name) == S_OK)
                        {
                            func_name_wstr = std::wstring(func_name);
                            SysFreeString(func_name);
                        }
                        else
                        {
                            func_name_wstr = std::wstring(L"unknown");
                        }
                        ULONGLONG func_len;
                        if (symbol->get_length(&func_len) != S_OK)
                            func_len = 0;

                        DWORD sec_idx = 0, sec_off = 0;
                        if (symbol->get_addressSection(&sec_idx) == S_OK && symbol->get_addressOffset(&sec_off) == S_OK)
                        {
                            FuncSymDesc sym_desc = { sec_idx, static_cast<uint32_t>(func_len), sec_off, func_name_wstr };
                            sym_info.push_back(sym_desc);
                        }
                    }
                }
                symbol->Release();
            }
            symbols->Release();
        }
        obj->Release();
    }
    objs->Release();

    // Free DIA COM
    DiaSymbol->Release();
    DiaSession->Release();
    CoUninitialize();
}

bool sort_samples(const SampleDesc& a, const SampleDesc& b)
{
    if (a.event_src != b.event_src)
    {
        if (a.event_src == CYCLE_EVT_IDX)
            return true;

        if (b.event_src == CYCLE_EVT_IDX)
            return false;

        return a.event_src < b.event_src;
    }

    return a.freq > b.freq;
}

bool sort_pcs(const std::pair<uint64_t, uint64_t>& a, const std::pair<uint64_t, uint64_t>& b)
{
    return a.second > b.second;
}
