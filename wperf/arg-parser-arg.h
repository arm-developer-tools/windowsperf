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

#pragma once
#include <string>
#include <vector>
#include <set>
#include <functional>
#include <sstream>
#include <string>

namespace ArgParserArg {
    struct arg_parser_arg {
        const std::wstring m_name;
        std::vector<std::wstring> m_aliases{};
        const std::wstring m_description;

        int m_arg_count; // -1 for variable number of arguments
        bool m_is_parsed = false;
        std::vector<std::wstring> m_values{};

        std::vector <std::function<bool(const std::wstring&)>> m_check_funcs = {};

        inline bool operator==(const std::wstring& other) const;

    public:
        arg_parser_arg(
            const std::wstring name,
            const std::vector<std::wstring> alias,
            const std::wstring description,
            const std::vector<std::wstring> default_values = {},
            const int arg_count = 0
        );
        bool is_match(const std::wstring& arg) const;
        virtual std::wstring get_help() const;
        virtual std::wstring get_all_flags_string() const;
        virtual std::wstring get_usage_text() const;
        arg_parser_arg add_alias(std::wstring new_alias);
        int get_arg_count() const;
        std::wstring get_name() const;
        std::wstring get_alias_string() const;
        arg_parser_arg add_check_func(std::function<bool(const std::wstring&)> check_func);
        void set_is_parsed();
        bool is_parsed() const;
        bool is_set() const;
        std::vector<std::wstring> get_values() const;
        bool parse(std::vector<std::wstring> arg_vect);
    };

    class arg_parser_arg_opt : public arg_parser_arg {
    public:
        arg_parser_arg_opt(
            const std::wstring name,
            const std::vector<std::wstring> alias,
            const std::wstring description,
            const std::vector<std::wstring> default_values = {}
        ) : arg_parser_arg(name, alias, description, default_values, 0) {};
    };

    class arg_parser_arg_pos : public arg_parser_arg {
    public:
        arg_parser_arg_pos(
            const std::wstring name,
            const std::vector<std::wstring> alias,
            const std::wstring description,
            const std::vector<std::wstring> default_values = {},
            const int arg_count = 1
        ) : arg_parser_arg(name, alias, description, default_values, arg_count) {};
    };

    constexpr auto MAX_HELP_WIDTH = 80;
    std::wstring arg_parser_add_wstring_behind_multiline_text(const std::wstring& str, const std::wstring& prefix);
    std::wstring arg_parser_format_string_to_length(const std::wstring& str, size_t max_width = MAX_HELP_WIDTH);
}
