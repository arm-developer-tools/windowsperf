#pragma once
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

#include <string>
#include <vector>
#include <tlhelp32.h>
#include <psapi.h>

#define MAX_PROCESSES					1024
#define MAX_SPAWN_RETRIES				4

struct HardwareInformation
{
    struct GroupInformation
    {
        UINT32 m_processorCount;
        KAFFINITY m_affinityMask;
    };
    UINT32 m_groupCount;
    UINT32 m_fullProcessorCount;
    std::vector<GroupInformation> m_groupInformation;
};

VOID GetHardwareInfo(HardwareInformation &hinfo);
DWORD FindProcess(std::wstring lpcszFileName);
HMODULE GetModule(HANDLE pHandle, std::wstring pname);
VOID SpawnProcess(const wchar_t* pe_file, const wchar_t* command_line, PROCESS_INFORMATION* pi, uint32_t delay);
BOOL SetAffinity(HardwareInformation& hInfo, DWORD pid, UINT8 core);
std::vector<DWORD> EnumerateThreads(DWORD pid);