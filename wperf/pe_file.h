#pragma once
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

#include <windows.h>
#include <string>
#include <vector>

#include "dia2.h"

typedef struct _SectionDesc
{
    uint32_t idx{};
    uint64_t offset{};
    uint64_t virtual_size{};
    std::wstring name;
} SectionDesc;

typedef struct _LineNumberDesc
{
    std::wstring source_file;
    DWORD lineNum{};
    DWORD colNum{};
    BOOL isStatement{};
    DWORD addressSection{};
    DWORD addressOffset{};
    DWORD length{};
    DWORD rva{};
    ULONGLONG virtualAddress{};
} LineNumberDesc;

typedef struct _FuncSymDesc
{
    uint32_t sec_idx{};
    uint32_t size{};
    uint64_t offset{};
    std::wstring name;      // Name of the whole symbol, e.g. x_mul:python312.dll
    std::wstring sname;     // Symbol name only
    std::vector<LineNumberDesc> lines;
} FuncSymDesc;

typedef struct _ModuleMetaData
{
    std::wstring mod_name;
    std::wstring mod_path;
    uint64_t mod_baseOfDll{};
    HMODULE handle{};
    std::vector<FuncSymDesc> sym_info;
} ModuleMetaData;

typedef struct _SampleDesc
{
    uint32_t freq{};
    FuncSymDesc desc;
    ModuleMetaData* module{};
    uint32_t event_src{};
    std::vector<std::pair<uint64_t, uint64_t>> pc;
} SampleDesc;

typedef struct _PeFileMetaData
{
    std::wstring pe_name;
    std::wstring pdb_file;
    uint64_t static_entry_point{};
    uint64_t image_base{};
    std::vector<SectionDesc> sec_info;
    std::vector<std::wstring> sec_import;
} PeFileMetaData;

std::wstring gen_pdb_name(std::wstring str);
void parse_pdb_file(std::wstring pdb_file, std::vector<FuncSymDesc>& sym_info, bool sample_display_short);
void parse_pe_file(const std::wstring& pe_file, uint64_t& image_base);
void parse_pe_file(std::wstring pe_file, uint64_t& static_entry_point, uint64_t& image_base, std::vector<SectionDesc>& sec_info, std::vector<std::wstring>& sec_import);
void parse_pe_file(std::wstring pe_file, PeFileMetaData& pefile_metadata);
bool sort_samples(const SampleDesc& a, const SampleDesc& b);
bool sort_pcs(const std::pair<uint64_t, uint64_t>& a, const std::pair<uint64_t, uint64_t>& b);

void read_function_lines(FuncSymDesc& funcSymDesc, IDiaSymbol* pSymbol, IDiaSession* pSession);