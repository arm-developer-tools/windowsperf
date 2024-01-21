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
#include <iostream>
#include <tchar.h>
#include <psapi.h>
#include "process_api.h"
#include "exception.h"
#include "output.h"

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

VOID SpawnProcess(const wchar_t* pe_file, const wchar_t* command_line, PROCESS_INFORMATION* pi, uint32_t delay)
{
    STARTUPINFO si;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(pi, sizeof(*pi));

    // Start the child process. 
    if (!CreateProcess((LPWSTR)pe_file,     // Module name
        (LPWSTR)command_line,               // Command line
        NULL,                               // Process handle not inheritable
        NULL,                               // Thread handle not inheritable
        FALSE,                              // Set handle inheritance to FALSE
        CREATE_NEW_CONSOLE,                 // Create a new console
        NULL,                               // Use parent's environment block
        NULL,                               // Use parent's starting directory 
        &si,                                // Pointer to STARTUPINFO structure
        pi)                                 // Pointer to PROCESS_INFORMATION structure
        )
    {
        DWORD error = GetLastError();
        m_out.GetErrorOutputStream() << "CreateProcess failed (0x" << std::hex << error << ")." << std::endl;
        throw fatal_exception("CreateProcess failed");
    }

    //Wait for the process to properly start 
    Sleep(delay);
    DWORD pid = GetProcessId(pi->hProcess);
    USHORT retries = 0;
    while (pid == 0 && retries < MAX_SPAWN_RETRIES)
    {
        Sleep(1000);
        pid = GetProcessId(pi->hProcess);
        retries++;
    }
    if (pid == 0)
    {
        TerminateProcess(pi->hProcess, 0);
        CloseHandle(pi->hThread);
        CloseHandle(pi->hProcess);
        throw fatal_exception("Process took too long to spawn");
    }
}

std::vector<DWORD> EnumerateThreads(DWORD pid)
{
    std::vector<DWORD> thread_ids;
    // TH32CS_SNAPTHREAD return all threads for all pids so we have to filter manually
    HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (h != INVALID_HANDLE_VALUE) {
        THREADENTRY32 te{ 0 };
        te.dwSize = sizeof(te);
        if (Thread32First(h, &te)) {
            do {
                if (te.dwSize >= 
                    FIELD_OFFSET(THREADENTRY32, th32OwnerProcessID) + sizeof(te.th32OwnerProcessID))
                {
                    if(te.th32OwnerProcessID == pid)
                    {
                        thread_ids.push_back(te.th32ThreadID);
                    }
                }
                te.dwSize = sizeof(te);
            } while (Thread32Next(h, &te));
        }
        CloseHandle(h);
    }
    return thread_ids;
};

VOID GetHardwareInfo(HardwareInformation& hinfo)
{
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX buffer = NULL;
    DWORD returnLength = 0;
    
    GetLogicalProcessorInformationEx(RelationAll, buffer, &returnLength);
    auto buffer_raw = std::make_unique<char[]>(returnLength);

    if (!GetLogicalProcessorInformationEx(RelationAll, reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(buffer_raw.get()), &returnLength))
    {
        throw fatal_exception("Error getting logical processor information");
    }

    size_t offset = 0;
    char* cur = buffer_raw.get();

    hinfo.m_groupCount = 0;
    hinfo.m_fullProcessorCount = 0;
    hinfo.m_groupInformation.clear();

    while (offset < returnLength)
    {
        PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX pointer = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(cur);
        if (pointer->Relationship == RelationGroup)
        {
            PGROUP_RELATIONSHIP gR = reinterpret_cast<PGROUP_RELATIONSHIP>(&pointer->Group);
            
            hinfo.m_groupCount += gR->ActiveGroupCount;
            for (auto i = 0; i < gR->ActiveGroupCount; i++)
            {
                HardwareInformation::GroupInformation groupInfo;
                groupInfo.m_processorCount = gR->GroupInfo[i].ActiveProcessorCount;
                groupInfo.m_affinityMask = gR->GroupInfo[i].ActiveProcessorMask;

                hinfo.m_fullProcessorCount += groupInfo.m_processorCount;
                hinfo.m_groupInformation.push_back(groupInfo);
            }
        }
        offset += pointer->Size;
        cur += pointer->Size;
    }
}

BOOL SetAffinity(HardwareInformation& hInfo, DWORD pid, UINT8 core)
{
    DWORD_PTR affinity_mask = 0;

    //We could just use translated_core/group as core % 64 and core / 64
    //However, this will fail if/when Microsoft changes the processour group max size
    //So we're taking the slower path. Also we don't know how Windows would behave
    //with hotplug processors so this is safer.
    UINT8 translated_core = 0;
    WORD translated_group = 0;
    UINT32 accCores = 0;
    for (auto groupInfo : hInfo.m_groupInformation)
    {
        if (core >= groupInfo.m_processorCount + accCores)
        {
            accCores += groupInfo.m_processorCount;
            translated_group++;
        }
        else {
            translated_core = core - static_cast<UINT8>(accCores);
            break;
        }        
    }

    affinity_mask |= 1ULL << translated_core;

    // Sanity check
    if (!(affinity_mask & hInfo.m_groupInformation[translated_group].m_affinityMask))
    {
        m_out.GetErrorOutputStream() << "Affinity mask is not a subset of group's " << translated_group << " affinity mask" << std::endl;
        return false;
    }

    // Set thread affinity for all threads. We can't change the group affinity
    // of a process so this is the only way to pin a job to a core.
    auto tids = EnumerateThreads(pid);
    for (auto thread : tids)
    {
        auto threadHandle = OpenThread(THREAD_ALL_ACCESS, false, thread);
        if (!threadHandle)
        {
            m_out.GetErrorOutputStream() << "Error opening thread for TID " << thread << " with error " << GetLastError() << std::endl;
            return false;
        }

        GROUP_AFFINITY groupAffinity{ 0 };

        groupAffinity.Mask = affinity_mask;
        groupAffinity.Group = translated_group;

        if (!SetThreadGroupAffinity(threadHandle, &groupAffinity, NULL))
        {
            CloseHandle(threadHandle);
            m_out.GetErrorOutputStream() << "Error setting thread group affinity " << GetLastError() << std::endl;
            return false;
        }

        if (!SetThreadAffinityMask(threadHandle, affinity_mask))
        {
            CloseHandle(threadHandle);
            m_out.GetErrorOutputStream() << "Error setting thread affinity " << GetLastError() << std::endl;
            return false;
        }
        CloseHandle(threadHandle);
    }

    return true;
}
