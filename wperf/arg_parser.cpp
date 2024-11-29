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

#include <iostream>
#include <codecvt>
#include <locale>
#include <cwchar>
#include <vector>
#include <sstream>
#include "arg_parser.h"
#include "output.h"
#include "exception.h"

namespace ArgParser {
    arg_parser::arg_parser() {}

    void arg_parser::parse(
        _In_ const int argc,
        _In_reads_(argc) const wchar_t* argv[]
    )
    {
        wstr_vec raw_args;
        for (int i = 1; i < argc; i++)
        {
            raw_args.push_back(argv[i]);
            m_arg_array.push_back(argv[i]);
        }

        if (raw_args.size() == 0)
            throw_invalid_arg(L"", L"warning: No arguments were found!");

#pragma region Command Selector
        for (auto& command : m_commands_list)
        {
            if (command->parse(raw_args)) {
                m_command = command->m_command;
                raw_args.erase(raw_args.begin());

                break;
            }
        }
        if (m_command == COMMAND_CLASS::NO_COMMAND) {
            throw_invalid_arg(raw_args.front(), L"warning: command not recognized!");
        }
        if (m_command == COMMAND_CLASS::HELP) {
            return;
        }
#pragma endregion

        while (raw_args.size() > 0)
        {
            size_t initial_size = raw_args.size();

            for (auto& current_flag : m_flags_list)
            {
                try
                {
                    if (current_flag->parse(raw_args)) {
                        raw_args.erase(raw_args.begin(), raw_args.begin() + current_flag->get_arg_count() + 1);
						parsed_args.push_back(current_flag);
                    }
                }
                catch (const std::exception& err)
                {
                    throw_invalid_arg(raw_args.front(), L"Error: " + std::wstring(err.what(), err.what() + std::strlen(err.what())));
                }
            }

            // if going over all the known arguments doesn't affect the size of the raw_args, then the command is unkown
            if (initial_size == raw_args.size())
            {
                throw_invalid_arg(raw_args.front(), L"Error: Unrecognized command");
            }

        }
    }

    void arg_parser::print_help() const
    {
        m_out.GetOutputStream() << L"NAME:\n"
            << L"\twperf - Performance analysis tools for Windows on Arm\n\n"
            << L"\tUsage: wperf <command> [options]\n\n"
            << L"SYNOPSIS:\n\n";
        for (auto& command : m_commands_list)
        {
            m_out.GetOutputStream() << L"\t" << command->get_all_flags_string() << L"\n" << command->get_usage_text() << L"\n";
        }

        m_out.GetOutputStream() << L"OPTIONS:\n\n";
        for (auto& flag : m_flags_list)
        {
            m_out.GetOutputStream() << L" " << flag->get_help() << L"\n";
        }
        m_out.GetOutputStream() << L"EXAMPLES:\n\n";
        for (auto& command : m_commands_list)
        {
            if (command->get_examples().empty()) continue;
            m_out.GetOutputStream() << L"  " << command->get_examples() << L"\n";
        }
    }

#pragma region error handling
    void arg_parser::throw_invalid_arg(const std::wstring& arg, const std::wstring& additional_message) const
    {
        std::wstring command = L"wperf";
        for (int i = 1; i < m_arg_array.size(); ++i) {
            if (i > 0) {
                command.append(L" ");
            }
            command.append(m_arg_array.at(i));
        }

        std::size_t pos = command.find(arg);
        if (pos == 0 || pos == std::wstring::npos) {
            pos = command.length();
        }

        std::wstring indicator(pos, L'~');
        indicator += L'^';

        /*
        TODO: THIS function should change to use GetErrorOutputStream before migrating to wperf

         */

        std::wostringstream error_message;
        error_message << L"Invalid argument detected:\n"
            << command << L"\n"
            << indicator << L"\n";
        if (!additional_message.empty()) {
            error_message << additional_message << L"\n";
        }
        m_out.GetErrorOutputStream() << error_message.str();
        throw fatal_exception("INVALID_ARGUMENT");
    }

#pragma endregion

    wstring arg_parser_arg_command::get_usage_text() const
    {
        return arg_parser_add_wstring_behind_multiline_text(arg_parser_format_string_to_length(
            m_useage_text + L"\n" + m_description), L"\t   ") + L"\n";
    }

    wstring arg_parser_arg_command::get_examples() const
    {
        std::wstring example_output;
        for (auto& example : m_examples) {
            example_output += example + L"\n";
        }
        return arg_parser_add_wstring_behind_multiline_text(arg_parser_format_string_to_length(example_output), L"\t");
    }
}
