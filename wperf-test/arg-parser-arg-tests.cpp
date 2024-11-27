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

#include "pch.h"
#include "CppUnitTest.h"
#include <unordered_map>
#include "wperf/arg-parser-arg.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace ArgParserArg;

namespace arg_parser_arg_tests
{

    TEST_CLASS(ArgParserArgTests)
    {
    public:
        TEST_METHOD(TestConstructor)
        {
            arg_parser_arg arg(L"--name", { L"-n" }, L"Description of the argument", { L"default" }, 1);
            Assert::AreEqual(std::wstring(L"--name"), arg.get_name());
            Assert::AreEqual(std::wstring(L"-n"), arg.get_alias_string());
            Assert::AreEqual(std::wstring(L"Description of the argument"), arg.get_usage_text());
        }

        TEST_METHOD(TestIsMatch)
        {
            arg_parser_arg arg(L"--name", { L"-n" }, L"Test argument");
            Assert::IsTrue(arg.is_match(L"--name"));
            Assert::IsTrue(arg.is_match(L"-n"));
            Assert::IsFalse(arg.is_match(L"--unknown"));
        }

        TEST_METHOD(TestAddAlias)
        {
            arg_parser_arg arg(L"--name", { L"-n" }, L"Test argument");
            arg.add_alias(L"-alias");
            Assert::AreEqual(std::wstring(L"-n, -alias"), arg.get_alias_string());
            Assert::AreEqual(std::wstring(L"--name, -n, -alias"), arg.get_all_flags_string());
        }

        TEST_METHOD(TestAddCheckFunc)
        {
            arg_parser_arg arg(L"--name", {}, L"Test argument", {}, 1);
            arg.add_check_func([](const std::wstring& value) { return value.length() < 10; });
            Assert::ExpectException<std::invalid_argument>([&]() {
                arg.parse({ L"--name", L"value_that_is_too_long" });
                });
        }

        TEST_METHOD(TestParseSuccess)
        {
            arg_parser_arg arg(L"--name", {}, L"Test argument", {}, 1);
            Assert::IsTrue(arg.parse({ L"--name", L"value" }));
            auto values = arg.get_values();
            Assert::AreEqual(size_t(1), values.size());
            Assert::AreEqual(std::wstring(L"value"), values[0]);
        }

        TEST_METHOD(TestParseFailure)
        {
            arg_parser_arg arg(L"--name", {}, L"Test argument", {}, 1);
            Assert::ExpectException<std::invalid_argument>([&]() {
                arg.parse({ L"--name" }); // Not enough arguments
                });
        }

        TEST_METHOD(TestGetHelp)
        {
            arg_parser_arg arg(L"--help", { L"-h" }, L"Shows help information");
            auto help_text = arg.get_help();
            Assert::IsTrue(help_text.find(L"--help, -h") != std::wstring::npos);
            Assert::IsTrue(help_text.find(L"Shows help information") != std::wstring::npos);
        }

        TEST_METHOD(TestOptionalArgument)
        {
            arg_parser_arg_opt arg(L"--optional", {}, L"An optional argument");
            Assert::AreEqual(size_t(0), arg.get_arg_count());
            Assert::IsTrue(arg.parse({ L"--optional" }));
            Assert::IsTrue(arg.is_set());
        }

        TEST_METHOD(TestPositionalArgument)
        {
            arg_parser_arg_pos arg(L"filename", {}, L"Input file", {}, 1);
            Assert::AreEqual(size_t(1), arg.get_arg_count());
            Assert::IsTrue(arg.parse({ L"filename", L"input.txt" }));
            auto values = arg.get_values();
            Assert::AreEqual(std::wstring(L"input.txt"), values[0]);
            Assert::IsTrue(arg.is_set());
        }
        TEST_METHOD(TestGetHelpSingleAlias)
        {
            arg_parser_arg arg(L"--help", { L"-h" }, L"Displays help information.");
            auto help_text = arg.get_help();

            // Check if the help text includes all flags and description
            Assert::IsTrue(help_text.find(L"--help, -h") != std::wstring::npos);
            Assert::IsTrue(help_text.find(L"Displays help information.") != std::wstring::npos);
        }

        TEST_METHOD(TestGetHelpMultipleAliases)
        {
            arg_parser_arg arg(L"--output", { L"-o", L"-out" }, L"Specifies the output file.");
            auto help_text = arg.get_help();

            // Verify the help output contains all aliases and description
            Assert::IsTrue(help_text.find(L"--output, -o, -out") != std::wstring::npos);
            Assert::IsTrue(help_text.find(L"Specifies the output file.") != std::wstring::npos);
        }

        TEST_METHOD(TestGetHelpWithoutAliases)
        {
            arg_parser_arg arg(L"--verbose", {}, L"Enables verbose mode.");
            auto help_text = arg.get_help();

            // Ensure the help text only includes the main name when no aliases are defined
            Assert::IsTrue(help_text.find(L"--verbose") != std::wstring::npos);
            Assert::IsFalse(help_text.find(L",") != std::wstring::npos); // No aliases
            Assert::IsTrue(help_text.find(L"Enables verbose mode.") != std::wstring::npos);
        }

        TEST_METHOD(TestGetAllFlagsStringSingleAlias)
        {
            arg_parser_arg arg(L"--help", { L"-h" }, L"Displays help information.");
            auto flags = arg.get_all_flags_string();

            // Verify that all flags are correctly concatenated
            Assert::AreEqual(std::wstring(L"--help, -h"), flags);
        }

        TEST_METHOD(TestGetAllFlagsStringMultipleAliases)
        {
            arg_parser_arg arg(L"--input", { L"-i", L"-in" }, L"Specifies the input file.");
            auto flags = arg.get_all_flags_string();

            // Check concatenation of all flags with multiple aliases
            Assert::AreEqual(std::wstring(L"--input, -i, -in"), flags);
        }

        TEST_METHOD(TestGetAllFlagsStringNoAlias)
        {
            arg_parser_arg arg(L"--quiet", {}, L"Enables quiet mode.");
            auto flags = arg.get_all_flags_string();

            // Ensure that the main flag name is returned without extra formatting
            Assert::AreEqual(std::wstring(L"--quiet"), flags);
        }
    };
}