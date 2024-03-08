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
#include <exception>
#include <sstream>
#include <vector>
#include "output.h"
#include "utils.h"

DWORD WINAPI ReadStdOut(LPVOID lpParam);

struct DisassembledInstruction
{
    DWORD_PTR m_address{};
    DWORD_PTR m_instruction{};
    std::wstring m_asm;
};

class DisassembledSource
{
public:
    std::vector<DisassembledInstruction> m_source;

    DisassembledSource() { m_source.clear(); }

    std::vector<DisassembledInstruction> GetFunction(DWORD_PTR startAddress, DWORD_PTR instruction_count)
    {
        assert(startAddress > (m_source[0].m_address & 0xFFFFFF));
        assert(startAddress + instruction_count < (m_source[m_source.size() - 1].m_address & 0xFFFFFF));

        DWORD_PTR pos = (startAddress - (m_source[0].m_address & 0xFFFFFF)) >> 2;
        
        return std::vector<DisassembledInstruction>(m_source.begin() + pos, m_source.begin() + pos + instruction_count);
    }
};

class Disassembler
{
    std::wstring m_command;
    std::wstring m_commandLine;
    std::wstring m_commandLineFrom;
    std::wstring m_commandLineTo;

    HANDLE m_hChildStd_OUT_Wr = NULL;
    HANDLE m_hChildStd_IN_Rd = NULL;
    HANDLE m_hChildStd_IN_Wr = NULL;

    PROCESS_INFORMATION m_piProcInfo{ 0 };

    HANDLE m_hInputFile = NULL;
    HANDLE m_thread = NULL;

    VOID SpawnProcess(const std::wstring& command)
    {
        STARTUPINFO siStartInfo{ 0 };
        siStartInfo.cb = sizeof(STARTUPINFO);
        siStartInfo.hStdOutput = m_hChildStd_OUT_Wr;
        siStartInfo.hStdError = m_hChildStd_OUT_Wr;
        siStartInfo.hStdInput = m_hChildStd_IN_Rd;
        siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

        if (!CreateProcess(NULL,
            (LPWSTR)command.c_str(),    // command line 
            NULL,                       // process security attributes 
            NULL,                       // primary thread security attributes 
            TRUE,                       // handles are inherited 
            0,                          // creation flags 
            NULL,                       // use parent's environment 
            NULL,                       // use parent's current directory 
            &siStartInfo,               // STARTUPINFO pointer 
            &m_piProcInfo))             // receives PROCESS_INFORMATION 
        {
            throw std::exception("Error creating process");
        }
    }

    VOID Spawn(const std::wstring& command);
public:
    Disassembler(const std::wstring& cmd,
        const std::wstring& cmdline,
        const std::wstring& cmdFrom,
        const std::wstring& cmdTo) : m_command(cmd), m_commandLine(cmdline), m_commandLineFrom(cmdFrom), m_commandLineTo(cmdTo) {}

    VOID Close()
    {
        CloseHandle(m_piProcInfo.hProcess);
        CloseHandle(m_piProcInfo.hThread);

        CloseHandle(m_hChildStd_OUT_Wr);
        CloseHandle(m_hChildStd_OUT_Rd);
        CloseHandle(m_hChildStd_IN_Wr);
        CloseHandle(m_hChildStd_IN_Rd);

        if (m_thread) CloseHandle(m_thread);
    }

    VOID Disassemble(const std::wstring& target);
    VOID Disassemble(uint64_t from, uint64_t to, const std::wstring& target);

    BOOL CheckCommand()
    {
        try {
            Spawn(m_command);
        }
        catch (const std::exception&)
        {
            return false;
        }

        CloseHandle(m_piProcInfo.hProcess);
        CloseHandle(m_piProcInfo.hThread);

        return true;
    }

    std::wstring GetCommand()
    {
        return m_command;
    }

    std::wstringstream m_processOutput;

    HANDLE m_hChildStd_OUT_Rd = NULL;
};

class LLVMDisassembler : public Disassembler
{
public:
    LLVMDisassembler() : Disassembler(L"llvm-objdump", L"--disassemble --disassemble-zeroes", L"--start-address=", L"--stop-address=") {}

    VOID ParseOutput(std::vector<DisassembledInstruction>& source)
    {
        std::vector<std::wstring> lines;

        TokenizeWideStringOfStrings(m_processOutput.str(), '\n', lines);

        for (auto& line : lines)
        {
            std::vector<std::wstring> line_split;
            TokenizeWideStringOfStrings(line, ' ', line_split);

            if(line.find(L"file format coff-arm64") == std::wstring::npos)
            {
                if (line_split.size() >= 3)
                {
                    try
                    {
                        DWORD_PTR address = std::stoll(line_split[0].erase(line_split[0].length() - 1, 1), nullptr, 16);
                        DWORD_PTR insn = std::stoll(line_split[1], nullptr, 16);
                        std::wstring::size_type cut_length = line_split[0].length() + line_split[1].length() + 2;
                        std::wstring dasm = line.substr(cut_length, line.length() - cut_length);
                        std::wstring dasm_clean = TrimWideString(dasm);
                        source.push_back(DisassembledInstruction{ address & 0xFFFFFF, insn, dasm_clean });
                    }
                    catch (const std::exception&)
                    {
                        // Error parsing, we don't care this line is not useful
                    }
                }
            }
        }
    }
};
