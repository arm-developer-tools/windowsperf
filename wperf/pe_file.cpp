// BSD 3-Clause License
//
// Copyright (c) 2024, Arm Limited
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
#include <memory>
#include <atlbase.h>

#include "exception.h"
#include "wperf-common\macros.h"
#include "output.h"
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


void parse_pe_file(const std::wstring& pe_file, uint64_t& image_base)
{
    std::ifstream pe_file_stream(pe_file, std::ios::binary);
    // make sure we have enough space for maximum read
    std::streamsize buf_size = sizeof(IMAGE_DOS_HEADER) > sizeof(IMAGE_NT_HEADERS)
        ? sizeof(IMAGE_DOS_HEADER) : sizeof(IMAGE_NT_HEADERS);

    std::unique_ptr<char[]> hdr_buf = std::make_unique<char[]>(buf_size);
    pe_file_stream.read(hdr_buf.get(), sizeof(IMAGE_DOS_HEADER));

    PIMAGE_DOS_HEADER dos_hdr = reinterpret_cast<PIMAGE_DOS_HEADER>(hdr_buf.get());
    if (dos_hdr->e_magic != IMAGE_DOS_SIGNATURE)
    {
        m_out.GetOutputStream() << pe_file << std::endl;
        throw fatal_exception("PE file specified is not in valid PE format");
    }

    std::streampos pos = dos_hdr->e_lfanew;
    pe_file_stream.seekg(pos);
    pe_file_stream.read(hdr_buf.get(), sizeof(IMAGE_NT_HEADERS));
    PIMAGE_NT_HEADERS nt_hdr = reinterpret_cast<PIMAGE_NT_HEADERS>(hdr_buf.get());
    if (nt_hdr->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
    {
        throw fatal_exception("PE file specified is not 64bit format");
    }

    image_base = nt_hdr->OptionalHeader.ImageBase;
    //std::cout << "static entry: 0x" << std::hex << static_entry_point << ", image_base: 0x" << image_base << std::endl;
    pe_file_stream.close();
}

void parse_pe_file(std::wstring pe_file, uint64_t& static_entry_point, uint64_t& image_base, std::vector<SectionDesc>& sec_info, std::vector<std::wstring>& sec_import)
{
    std::ifstream pe_file_stream(pe_file, std::ios::binary);
    // make sure we have enough space for maximum read
    std::streamsize buf_size = sizeof(IMAGE_DOS_HEADER) > sizeof(IMAGE_NT_HEADERS)
        ? sizeof(IMAGE_DOS_HEADER) : sizeof(IMAGE_NT_HEADERS);

    std::unique_ptr<char[]> hdr_buf = std::make_unique<char[]>(buf_size);
    pe_file_stream.read(hdr_buf.get(), sizeof(IMAGE_DOS_HEADER));

    PIMAGE_DOS_HEADER dos_hdr = reinterpret_cast<PIMAGE_DOS_HEADER>(hdr_buf.get());
    if (dos_hdr->e_magic != IMAGE_DOS_SIGNATURE)
    {
        m_out.GetOutputStream() << pe_file << std::endl;
        throw fatal_exception("PE file specified is not in valid PE format");
    }

    std::streampos pos = dos_hdr->e_lfanew;
    pe_file_stream.seekg(pos);
    pe_file_stream.read(hdr_buf.get(), sizeof(IMAGE_NT_HEADERS));
    PIMAGE_NT_HEADERS nt_hdr = reinterpret_cast<PIMAGE_NT_HEADERS>(hdr_buf.get());
    if (nt_hdr->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
    {
        throw fatal_exception("PE file specified is not 64bit format");
    }

    static_entry_point = nt_hdr->OptionalHeader.AddressOfEntryPoint;
    image_base = nt_hdr->OptionalHeader.ImageBase;
    //std::cout << "static entry: 0x" << std::hex << static_entry_point << ", image_base: 0x" << image_base << std::endl;

    buf_size = sizeof(IMAGE_SECTION_HEADER) * nt_hdr->FileHeader.NumberOfSections;
    std::unique_ptr<char[]> sec_buf = std::make_unique<char[]>(buf_size);
    pos += FIELD_OFFSET(IMAGE_NT_HEADERS, OptionalHeader) + std::streampos(nt_hdr->FileHeader.SizeOfOptionalHeader);
    pe_file_stream.seekg(pos);
    pe_file_stream.read(sec_buf.get(), buf_size);

    ULONGLONG importDirectoryRVA = nt_hdr->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
    PIMAGE_SECTION_HEADER importSection = NULL;

    PIMAGE_SECTION_HEADER sections = reinterpret_cast<PIMAGE_SECTION_HEADER>(sec_buf.get());
    for (uint32_t i = 0; i < nt_hdr->FileHeader.NumberOfSections; i++)
    {
        char section_name[16] = { 0 };
        memcpy_s(section_name, sizeof section_name, sections[i].Name, IMAGE_SIZEOF_SHORT_NAME);

        std::string name(section_name);
        SectionDesc sec_desc = { i, sections[i].VirtualAddress, sections[i].Misc.VirtualSize, std::wstring(name.begin(), name.end()) };
        sec_info.push_back(sec_desc);
        //std::cout << "addr: 0x" << std::hex << sec_desc.offset << ", name: " <<  sec_desc.name << std::endl;

        ULONGLONG section_vaddr_with_size = (ULONGLONG)sections[i].VirtualAddress + sections[i].Misc.VirtualSize;
        if (importDirectoryRVA >= sections[i].VirtualAddress && importDirectoryRVA < section_vaddr_with_size) {
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

        std::unique_ptr<char[]> import_buf = std::make_unique<char[]>(length_to_read);
        pe_file_stream.read(import_buf.get(), length_to_read);
        PIMAGE_IMPORT_DESCRIPTOR importDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)(import_buf.get() + (nt_hdr->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress - importSection->VirtualAddress));

        for (; importDescriptor->Name != 0; importDescriptor++) {
            std::string name = import_buf.get() + (importDescriptor->Name - importSection->VirtualAddress);
            sec_import.push_back(std::wstring(name.begin(), name.end()));
            // std::cout << "dll: " << name << std::endl;
        }
    }

    pe_file_stream.close();
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
        m_out.GetOutputStream() << L"Status REGDB_E_CLASSNOTREG indicates that DIA SDK class is not registered!" << std::endl;
        m_out.GetOutputStream() << L"See https://learn.microsoft.com/en-us/visualstudio/debugger/debug-interface-access/getting-started-debug-interface-access-sdk?view=vs-2019" << std::endl;
        m_out.GetOutputStream() << L"Try registering this service (as Administrator) with command:" << std::endl;
        m_out.GetOutputStream() << std::endl;
        m_out.GetOutputStream() << L"\t" << L"> cd \"C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\DIA SDK\\bin\\arm64\"" << std::endl;
        m_out.GetOutputStream() << L"\t" << L"> regsvr32 msdia140.dll" << std::endl;

        if (status != REGDB_E_CLASSNOTREG)
            m_out.GetOutputStream() << L"CoCreateInstance failed with status: 0x" << std::hex << status << std::endl;

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
                            read_function_lines(sym_desc, symbol, DiaSession);
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
    DiaDataSource->Release();
    CoUninitialize();
}

void read_function_lines(FuncSymDesc& funcSymDesc, IDiaSymbol* pSymbol, IDiaSession* pSession)
{
    ULONGLONG length = 0;
    DWORD     isect = 0;
    DWORD     offset = 0;

    pSymbol->get_addressSection(&isect);
    pSymbol->get_addressOffset(&offset);
    pSymbol->get_length(&length);
    if (isect != 0 && length > 0)
    {
        CComPtr<IDiaEnumLineNumbers> pLines;
        if (SUCCEEDED(pSession->findLinesByAddr(
            isect,
            offset,
            static_cast<DWORD>(length),
            &pLines)))
        {
            LONG enumCount = 0;
            if (SUCCEEDED(pLines->get_Count(&enumCount)))
            {
                for (auto i = 0; i < enumCount; i++)
                {
                    CComPtr<IDiaLineNumber> pLine;
                    if (SUCCEEDED(pLines->Item(i, &pLine)))
                    {
                        DWORD offset_ = 0;
                        DWORD seg = 0;
                        DWORD linenum = 0;
                        DWORD colnum = 0;
                        DWORD block_length = 0;
                        DWORD rva = 0;
                        BOOL isStatement = false;
                        CComPtr<IDiaSourceFile> pSrc;
                        BSTR fName;
                        std::wstring file_name_wstr;
                        ULONGLONG addr = 0;

                        pLine->get_sourceFile(&pSrc);
                        pLine->get_addressSection(&seg);
                        pLine->get_addressOffset(&offset_);
                        pLine->get_lineNumber(&linenum);
                        pLine->get_columnNumber(&colnum);
                        pLine->get_virtualAddress(&addr);
                        pLine->get_length(&block_length);
                        pLine->get_relativeVirtualAddress(&rva);
                        pLine->get_statement(&isStatement);

                        if (pSrc->get_fileName(&fName) == S_OK)
                        {
                            file_name_wstr = std::wstring(fName);
                            SysFreeString(fName);
                        }
                        else
                        {
                            file_name_wstr = std::wstring(L"unknown");
                        }

                        funcSymDesc.lines.push_back(LineNumberDesc{
                            std::wstring(file_name_wstr),
                            linenum,
                            colnum,
                            isStatement,
                            seg,
                            offset_,
                            block_length,
                            rva,
                            addr });
                    }
                }
            }
        }
    }
    std::sort(funcSymDesc.lines.begin(), funcSymDesc.lines.end(), [](LineNumberDesc& a, LineNumberDesc& b) -> bool { return a.lineNum < b.lineNum; });
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
