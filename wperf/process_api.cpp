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

#include <windows.h>
#include <iostream>
#include <tchar.h>
#include <psapi.h>
#include "process_api.h"

DWORD FindProcess(std::wstring lpcszFileName)
{
    LPDWORD lpdwProcessIds;
    LPWSTR  lpszBaseName;
    HANDLE  hProcess;
    DWORD   cdwProcesses, dwProcessId = 0;

    lpdwProcessIds = (LPDWORD)HeapAlloc(GetProcessHeap(), 0, MAX_PROCESSES*sizeof(DWORD));
    if (!lpdwProcessIds)
        return 0;

    if (!EnumProcesses(lpdwProcessIds, MAX_PROCESSES*sizeof(DWORD), &cdwProcesses))
        goto release_and_exit;

    lpszBaseName = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, MAX_PATH*sizeof(wchar_t));
    if (!lpszBaseName)
        goto release_and_exit;

    cdwProcesses /= sizeof(DWORD);
    for (DWORD i = 0; i < cdwProcesses; i++)
    {
        hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, lpdwProcessIds[i]);
        if (!hProcess)
            continue;

        if (GetModuleBaseNameW(hProcess, NULL, lpszBaseName, MAX_PATH) > 0)
        {
            if (lpszBaseName == lpcszFileName)
            {
                dwProcessId = lpdwProcessIds[i];
                CloseHandle(hProcess);
                break;
            }
        }

        CloseHandle(hProcess);
    }

    HeapFree(GetProcessHeap(), 0, (LPVOID)lpszBaseName);

release_and_exit:
    HeapFree(GetProcessHeap(), 0, (LPVOID)lpdwProcessIds);
    return dwProcessId;
}

HMODULE GetModule(HANDLE pHandle, std::wstring pname)
{
    HMODULE hMods[1024];
    DWORD cbNeeded;

    if (!EnumProcessModules(pHandle, hMods, sizeof(hMods), &cbNeeded))
        return nullptr;

    for (unsigned int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++)
    {
        wchar_t szModName[MAX_PATH];
        if (GetModuleFileNameExW(pHandle, hMods[i], szModName, sizeof(szModName) / sizeof(wchar_t)))
        {
            std::wstring wstrModName(szModName);

            if (wstrModName.find(pname) != std::wstring::npos)
                return hMods[i];
        }
    }

    return nullptr;
}
