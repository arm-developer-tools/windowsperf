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
#include "pe_file.h"

void parse_pe_file(std::wstring pe_file, uint64_t& static_entry_point, uint64_t& image_base, std::vector<SectionDesc>& sec_info)
{
#if 1
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
    PIMAGE_SECTION_HEADER sections = reinterpret_cast<PIMAGE_SECTION_HEADER>(sec_buf);
    for (uint32_t i = 0; i < nt_hdr->FileHeader.NumberOfSections; i++)
    {
        SectionDesc sec_desc = { i, sections[i].VirtualAddress, std::string(reinterpret_cast<char*>(sections[i].Name)) };
        sec_info.push_back(sec_desc);
        //std::cout << "addr: 0x" << std::hex << sec_desc.offset << ", name: " <<  sec_desc.name << std::endl;
    }

    delete[] hdr_buf;
    delete[] sec_buf;
#else // legacy code passing llvm result
    HANDLE g_hChildStd_IN_Rd = NULL;
    HANDLE g_hChildStd_IN_Wr = NULL;
    HANDLE g_hChildStd_OUT_Rd = NULL;
    HANDLE g_hChildStd_OUT_Wr = NULL;
    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;
    if (!CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0))
        throw fatal_exception("CreatePipe failed for child stdout");
    if (!SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0))
        throw fatal_exception("SetHandleInformation failed for child stdout");
    if (!CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0))
        throw fatal_exception("CreatePipe failed for child stdin");
    if (!SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0))
        throw fatal_exception("SetHandleInformation failed for child stdin");
    std::wstring dump_cmd = L"llvm-readobj.exe --file-headers --section-headers " + pe_file;
    PROCESS_INFORMATION piProcInfo;
    STARTUPINFOW siStartInfo;
    BOOL bSuccess = FALSE;

    ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));
    ZeroMemory(&siStartInfo, sizeof(STARTUPINFOW));
    siStartInfo.cb = sizeof(STARTUPINFO);
    siStartInfo.hStdError = g_hChildStd_OUT_Wr;
    siStartInfo.hStdOutput = g_hChildStd_OUT_Wr;
    siStartInfo.hStdInput = g_hChildStd_IN_Rd;
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;
    wchar_t* dump_cmd_chr = new wchar_t[dump_cmd.length() + 1];
    std::wcscpy(dump_cmd_chr, dump_cmd.c_str());
    bSuccess = CreateProcessW(NULL, dump_cmd_chr, NULL, NULL, TRUE, 0, NULL, NULL, &siStartInfo, &piProcInfo);
    if (!bSuccess)
    {
        throw fatal_exception("CreateProcess failed for llvm-readobj");
    }
    else
    {
        CloseHandle(piProcInfo.hProcess);
        CloseHandle(piProcInfo.hThread);
        CloseHandle(g_hChildStd_OUT_Wr);
        CloseHandle(g_hChildStd_IN_Rd);
    }
#define BUFSIZE  4096
#define LINESIZE 256
    int line_pos = 0;
    SectionDesc sec_desc = { UINT32_MAX, UINT64_MAX, "" };
    bool inside_section = false;
    for (;;)
    {
        CHAR chBuf[BUFSIZE];
        CHAR lineBuf[LINESIZE];
        DWORD dwRead;
        bSuccess = ReadFile(g_hChildStd_OUT_Rd, chBuf, BUFSIZE, &dwRead, NULL);
        if (!bSuccess || dwRead == 0)
            break;
        for (DWORD i = 0; i < dwRead; i++)
        {
            if (chBuf[i] == ' ')
                continue;
            lineBuf[line_pos++] = chBuf[i];
            if (chBuf[i] != '\n')
                continue;
            // remove newline plus end the string.
            lineBuf[line_pos - 1] = '\0';
            line_pos = 0;
            std::string lineString(lineBuf);
            size_t delim_pos = 0;
            if ((delim_pos = lineString.find(":")) == std::string::npos)
            {
                if (lineString == "Section{")
                {
                    inside_section = true;
                }
                else if (lineString == "}" && inside_section)
                {
                    inside_section = false;
                    sec_info.push_back(sec_desc);
                }
            }
            else if (inside_section)
            {
                std::string key, value;
                key = lineString.substr(0, delim_pos);
                value = lineString.substr(delim_pos + 1, std::string::npos);
                if (key == "Number")
                    sec_desc.idx = std::stoi(value, nullptr, 10);
                else if (key == "VirtualAddress")
                    sec_desc.offset = std::stoll(value, nullptr, 0);
                else if (key == "Name")
                {
                    if ((delim_pos = value.find("(")) != std::string::npos)
                    {
                        sec_desc.name = value.substr(0, delim_pos);
                    }
                    else
                    {
                        sec_desc.name = value;
                    }
                }
            }
            else
            {
                std::string key, value;
                key = lineString.substr(0, delim_pos);
                if (key == "AddressOfEntryPoint" || key == "ImageBase")
                {
                    value = lineString.substr(delim_pos + 1, std::string::npos);
                    if (key == "AddressOfEntryPoint")
                        static_entry_point = std::stoll(value, nullptr, 0);
                    else
                        image_base = std::stoll(value, nullptr, 0);
                }
            }
        }
    }
    delete[] dump_cmd_chr;
#endif
}

void parse_pdb_file(std::wstring pdb_file, std::vector<FuncSymDesc>& sym_info, bool sample_display_short)
{
#if 1
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
            std::cout << "The DIA SDK requires msdia.dll. Its normally installed and automatically registered with Visual Studio." << std::endl;
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

#else // legacy code for getting result from llvm-pdbutil dump
    HANDLE g_hChildStd_IN_Rd = NULL;
    HANDLE g_hChildStd_IN_Wr = NULL;
    HANDLE g_hChildStd_OUT_Rd = NULL;
    HANDLE g_hChildStd_OUT_Wr = NULL;
    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;
    if (!CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0))
        throw fatal_exception("CreatePipe failed for child stdout");
    if (!SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0))
        throw fatal_exception("SetHandleInformation failed for child stdout");
    if (!CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0))
        throw fatal_exception("CreatePipe failed for child stdin");
    if (!SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0))
        throw fatal_exception("SetHandleInformation failed for child stdin");
    std::wstring dump_cmd = L"llvm-pdbutil.exe dump -symbols " + pdb_file;
    PROCESS_INFORMATION piProcInfo;
    STARTUPINFOW siStartInfo;
    BOOL bSuccess = FALSE;

    ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));
    ZeroMemory(&siStartInfo, sizeof(STARTUPINFOW));
    siStartInfo.cb = sizeof(STARTUPINFO);
    siStartInfo.hStdError = g_hChildStd_OUT_Wr;
    siStartInfo.hStdOutput = g_hChildStd_OUT_Wr;
    siStartInfo.hStdInput = g_hChildStd_IN_Rd;
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;
    wchar_t* dump_cmd_chr = new wchar_t[dump_cmd.length() + 1];
    std::wcscpy(dump_cmd_chr, dump_cmd.c_str());
    bSuccess = CreateProcessW(NULL, dump_cmd_chr, NULL, NULL, TRUE, 0, NULL, NULL, &siStartInfo, &piProcInfo);
    if (!bSuccess)
    {
        throw fatal_exception("CreateProcess failed for llvm-readobj");
    }
    else
    {
        CloseHandle(piProcInfo.hProcess);
        CloseHandle(piProcInfo.hThread);
        CloseHandle(g_hChildStd_OUT_Wr);
        CloseHandle(g_hChildStd_IN_Rd);
    }
#define BUFSIZE  4096
#define LINESIZE2 1024
    int line_pos = 0;
    FuncSymDesc sym_desc = { UINT32_MAX, 0, UINT64_MAX, "" };
    bool got_sym = false;
    for (;;)
    {
        CHAR chBuf[BUFSIZE];
        CHAR lineBuf[LINESIZE2];
        DWORD dwRead;
        bSuccess = ReadFile(g_hChildStd_OUT_Rd, chBuf, BUFSIZE, &dwRead, NULL);
        if (!bSuccess || dwRead == 0)
            break;
        for (DWORD i = 0; i < dwRead; i++)
        {
            if (chBuf[i] == ' ')
                continue;
            lineBuf[line_pos++] = chBuf[i];
            if (chBuf[i] != '\n')
                continue;
            // remove newline plus end the string.
            lineBuf[line_pos - 1] = '\0';
            line_pos = 0;
            std::string lineString(lineBuf);
            size_t delim_pos = 0;
            if (got_sym)
            {
                got_sym = false;
                std::string key = "addr=";
                if ((delim_pos = lineString.find(key)) != std::string::npos)
                {
                    std::string sym_value = lineString.substr(delim_pos + key.length(), std::string::npos);
                    delim_pos = sym_value.find(",");
                    if (delim_pos != std::string::npos)
                        sym_value = sym_value.substr(0, delim_pos);
                    delim_pos = sym_value.find(":");
                    std::string value = sym_value.substr(0, delim_pos);
                    sym_desc.sec_idx = std::stoi(value, nullptr, 10);
                    value = sym_value.substr(delim_pos + 1, std::string::npos);
                    sym_desc.offset = std::stoll(value, nullptr, 10);
                }
                key = "codesize=";
                if ((delim_pos = lineString.find(key)) != std::string::npos)
                {
                    std::string sym_value = lineString.substr(delim_pos + key.length(), std::string::npos);
                    delim_pos = sym_value.find(",");
                    if (delim_pos != std::string::npos)
                        sym_value = sym_value.substr(0, delim_pos);
                    sym_desc.size = std::stoi(sym_value, nullptr, 10);
                }
                sym_info.push_back(sym_desc);
            }
            else
            {
                if ((delim_pos = lineString.find("|S_LPROC32")) != std::string::npos)
                {
                    got_sym = true;
                    delim_pos = lineString.find("`");
                    if (delim_pos != std::string::npos)
                    {
                        std::string sym_name = lineString.substr(delim_pos + 1, std::string::npos);
                        sym_name.pop_back();
                        sym_desc.name = sym_name;
                    }
                }
            }
        }
    }
    delete[] dump_cmd_chr;
#endif
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
