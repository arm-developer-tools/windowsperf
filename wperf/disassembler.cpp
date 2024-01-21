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

#include "disassembler.h"

DWORD WINAPI ReadStdOut(LPVOID lpParam)
{
    constexpr auto BUFSIZE = 1024;

    DWORD dwRead;
    CHAR chBuf[BUFSIZE]{ 0 };
    Disassembler* disassembler = (Disassembler*)lpParam;

    for (;;)
    {
        DWORD error = 0;

        if (!ReadFile(disassembler->m_hChildStd_OUT_Rd, chBuf, BUFSIZE, &dwRead, NULL))
        {
            error = GetLastError();
            if (error == ERROR_BROKEN_PIPE)
            {
                break;
            }
            if (error != ERROR_MORE_DATA)
            {
                m_out.GetErrorOutputStream() << "Error reading pipe with " << error << std::endl;
                break;
            }
        }
        std::wstring wdata = WideStringFromMultiByte(std::string(chBuf, dwRead).c_str());
        disassembler->m_processOutput << wdata;
    }

    return 0;
}

VOID Disassembler::Spawn(const std::wstring& command)
{
    SECURITY_ATTRIBUTES saAttr{ 0 };

    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    if (!CreatePipe(&m_hChildStd_OUT_Rd, &m_hChildStd_OUT_Wr, &saAttr, 0))
    {
        throw std::exception("Failed to create pipe");
    }

    if (!CreatePipe(&m_hChildStd_IN_Rd, &m_hChildStd_IN_Wr, &saAttr, 0))
    {
        throw std::exception("Failed to create pipe");
    }

    if (!SetHandleInformation(m_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0))
    {
        throw std::exception("Failed to set pipe handle information");
    }

    if (!SetHandleInformation(m_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0))
    {
        throw std::exception("Failed to set pipe handle information");
    }

    m_processOutput.str(std::wstring());
    m_processOutput.clear();

    try
    {
        SpawnProcess(command);
    }
    catch (const std::exception&) {
        DWORD errorCode = GetLastError();
        switch (errorCode)
        {
        case 2:
            std::throw_with_nested(std::exception("Failed to SpawnProcess file not found"));
            break;
        default:
            m_out.GetErrorOutputStream() << "Failed to SpawnProcess with " << errorCode << std::endl;
            std::throw_with_nested(std::exception("Failed to SpawnProcess"));
        }
    }
}

VOID Disassembler::Disassemble(const std::wstring& target)
{
    DWORD threadId;
    std::wstringstream commandline;

    commandline << m_command << TEXT(" ") << m_commandLine << TEXT(" ") << target;

    Spawn(commandline.str());

    m_processOutput.clear();
    m_thread = CreateThread(NULL, 0, ReadStdOut, (LPVOID)this, 0, &threadId);
    if (m_thread == NULL)
    {
        throw std::exception("Error creating thread");
    }

    WaitForSingleObject(m_piProcInfo.hProcess, INFINITE);
}

VOID Disassembler::Disassemble(uint64_t from, uint64_t to, const std::wstring& target)
{
    DWORD threadId;
    std::wstringstream commandline;

    commandline << m_command << L" " << m_commandLine << L" " <<
        m_commandLineFrom << L"0x" << std::hex << from << L" " << m_commandLineTo << L"0x" << to << L" " << target;

    //std::wcout << commandline.str() << std::endl;

    Spawn(commandline.str());

    m_processOutput.clear();
    m_thread = CreateThread(NULL, 0, ReadStdOut, (LPVOID)this, 0, &threadId);
    if (m_thread == NULL)
    {
        throw std::exception("Error creating thread");
    }

    WaitForSingleObject(m_piProcInfo.hProcess, INFINITE);
}
