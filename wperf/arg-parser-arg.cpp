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

#include "arg-parser-arg.h"
#include <stdexcept>

namespace ArgParserArg {
    arg_parser_arg::arg_parser_arg(
        const std::wstring name,
        const std::vector<std::wstring> alias,
        const std::wstring description,
        const std::vector<std::wstring> default_values,
        const int arg_count
    ) : m_name(name), m_aliases(alias), m_description(description), m_arg_count(arg_count), m_values(default_values) {};

    inline bool arg_parser_arg::operator==(const std::wstring& other_arg) const
    {
        return is_match(other_arg);
    }

    bool arg_parser_arg::is_match(const std::wstring& other_arg) const
    {
        return !other_arg.empty() && (other_arg == m_name || std::find(m_aliases.begin(), m_aliases.end(), other_arg) != m_aliases.end());
    }

    std::wstring arg_parser_arg::get_help() const
    {

        return L"\t" + get_all_flags_string() + L"\n" + arg_parser_add_wstring_behind_multiline_text(arg_parser_format_string_to_length(m_description), L"\t   ");
    }

    std::wstring arg_parser_arg::get_all_flags_string() const
    {
        if (get_alias_string().empty()) return m_name;
        return m_name + L", " + get_alias_string();
    }

    std::wstring arg_parser_arg::get_usage_text() const
    {
        return m_description;
    }

    arg_parser_arg arg_parser_arg::add_alias(std::wstring new_alias)
    {
        m_aliases.push_back(new_alias);
        return *this;
    }

    size_t arg_parser_arg::get_arg_count() const
    {
        return m_arg_count;
    }

    std::wstring arg_parser_arg::get_name() const
    {
        return m_name;
    }

    std::wstring arg_parser_arg::get_alias_string() const
    {
        // convert alias vector to wstring
        std::wstring alias_string;
        for (auto& m_alias : m_aliases) {
            if (m_alias.empty()) continue;
            alias_string.append(m_alias + L", ");
        }
        if (alias_string.find(L", ") != std::wstring::npos) alias_string.erase(alias_string.end() - 2, alias_string.end());

        return alias_string;
    }

    arg_parser_arg arg_parser_arg::add_check_func(std::function<bool(const std::wstring&)> check_func)
    {
        m_check_funcs.push_back(check_func);
        return *this;
    }

    void arg_parser_arg::set_is_parsed()
    {
        m_is_parsed = true;
    }

    bool arg_parser_arg::is_parsed() const
    {
        return m_is_parsed;
    }

    bool arg_parser_arg::is_set() const
    {
        return is_parsed() && m_values.size() >= m_arg_count;
    }

    std::vector<std::wstring> arg_parser_arg::get_values() const
    {
        return m_values;
    }

    bool arg_parser_arg::parse(std::vector<std::wstring> arg_vect)
    {
        if (arg_vect.size() == 0 || !is_match(arg_vect[0]))
            return false;

        if (m_arg_count == -1 && arg_vect.size() > 0) m_arg_count = arg_vect.size() - 1;

        if (arg_vect.size() < m_arg_count + 1)
            throw std::invalid_argument("Not enough arguments provided.");

        if (m_arg_count == 0)
        {
            set_is_parsed();
            return true;
        }

        for (int i = 1; i < m_arg_count + 1; ++i)
        {
            for (auto& check_func : m_check_funcs)
            {
                if (!check_func(arg_vect[i]))
                    throw std::invalid_argument("Invalid arguments provided.");
            }
            m_values.push_back(arg_vect[i]);
        }
        set_is_parsed();
        return true;
    }


    std::wstring arg_parser_add_wstring_behind_multiline_text(const std::wstring& str, const std::wstring& prefix)
    {
        std::wstring formatted_str;
        std::wstring current_line;
        std::wistringstream iss(str);
        while (getline(iss, current_line, L'\n'))
        {
            if (current_line.empty())
            {
                formatted_str += L"\n";
                continue;
            }

            formatted_str += prefix + current_line + L"\n";
        }
        return formatted_str;
    }

    std::wstring arg_parser_format_string_to_length(const std::wstring& str, size_t max_width)
    {
        std::wstring formatted_str;
        std::wistringstream lines_stream(str);
        std::wstring line;

        while (std::getline(lines_stream, line, L'\n'))
        {
            std::wstring current_line;
            std::wistringstream word_stream(line);
            std::wstring word;

            while (word_stream >> word)
            {
                if (current_line.size() + word.size() > max_width && !current_line.empty())
                {
                    if (!current_line.empty() && current_line.back() == L' ')
                    {
                        current_line.pop_back();
                    }
                    formatted_str += current_line + L"\n";
                    current_line = word + L" ";
                }
                else
                {
                    current_line += word + L" ";
                }
            }
            if (!current_line.empty() && current_line.back() == L' ')
            {
                current_line.pop_back();
            }
            if (!current_line.empty())
            {
                formatted_str += current_line + L"\n\n";
            }
        }

        // Remove the trailing newline, if any
        if (!formatted_str.empty() && formatted_str.back() == L'\n')
            formatted_str.pop_back();
        if (!formatted_str.empty() && formatted_str.back() == L'\n')
            formatted_str.pop_back();

        return formatted_str;
    }
}