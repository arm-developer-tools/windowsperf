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

namespace arg_parser_arg_utils_tests
{

    TEST_CLASS(ArgParserArgFormatToLengthTests)
    {
    public:
        TEST_METHOD(TestEmptyString)
        {
            std::wstring input = L"";
            size_t max_width = 10;
            std::wstring expected = L"";
            Assert::AreEqual(expected, arg_parser_format_string_to_length(input, max_width));
        }

        TEST_METHOD(TestSingleShortLine)
        {
            std::wstring input = L"Short line.";
            size_t max_width = 20;
            std::wstring expected = L"Short line.";
            Assert::AreEqual(expected, arg_parser_format_string_to_length(input, max_width));
        }

        TEST_METHOD(TestSingleLongLine)
        {
            std::wstring input = L"This is a line that exceeds the width.";
            size_t max_width = 15;
            std::wstring expected = L"This is a line\nthat exceeds the\nwidth.";
            Assert::AreEqual(expected, arg_parser_format_string_to_length(input, max_width));
        }
        TEST_METHOD(TestMultipleLines)
        {
            std::wstring input = L"Line one.\nLine two is a bit longer.\nShort.";
            size_t max_width = 15;
            std::wstring expected = L"Line one.\n\nLine two is a\nbit longer.\n\nShort.";
            Assert::AreEqual(expected, arg_parser_format_string_to_length(input, max_width));
        }
        TEST_METHOD(TestTrailingNewlineRemoval)
        {
            std::wstring input = L"Line one.\nLine two is a bit longer.\nShort.\n";
            size_t max_width = 15;
            std::wstring expected = L"Line one.\n\nLine two is a\nbit longer.\n\nShort.";
            Assert::AreEqual(expected, arg_parser_format_string_to_length(input, max_width));
        }
        TEST_METHOD(TestMultipleLinesAndMultipleTrailingReturnToLines)
        {
            std::wstring input = L"Line one.\nLine two is a bit longer.\nShort.\n\n\n\n\n\n\n";
            size_t max_width = 15;
            std::wstring expected = L"Line one.\n\nLine two is a\nbit longer.\n\nShort.\n\n\n\n\n\n";
            Assert::AreEqual(expected, arg_parser_format_string_to_length(input, max_width));
        }
        TEST_METHOD(MultipleRetrunToLines)
        {
            std::wstring input = L"\n\n\n";
            size_t max_width = 15;
            std::wstring expected = L"\n\n\n";
            Assert::AreEqual(expected, arg_parser_format_string_to_length(input, max_width));
        }
        TEST_METHOD(TestExactFit)
        {
            std::wstring input = L"Exactly fifteen";
            size_t max_width = 15;
            std::wstring expected = L"Exactly fifteen";
            Assert::AreEqual(expected, arg_parser_format_string_to_length(input, max_width));
        }

        TEST_METHOD(TestSingleWordExceedsWidth)
        {
            std::wstring input = L"Supercalifragilisticexpialidocious";
            size_t max_width = 10;
            std::wstring expected = L"Supercalifragilisticexpialidocious";
            Assert::AreEqual(expected, arg_parser_format_string_to_length(input, max_width));
        }
    };
    TEST_CLASS(ArgParserAddPrefixTests)
    {
    public:
        TEST_METHOD(TestEmptyString)
        {
            std::wstring input = L"";
            std::wstring prefix = L"-> ";
            std::wstring expected = L"";
            Assert::AreEqual(expected, arg_parser_add_wstring_behind_multiline_text(input, prefix));
        }

        TEST_METHOD(TestSingleLineWithPrefix)
        {
            std::wstring input = L"Hello, world!";
            std::wstring prefix = L"\t-> ";
            std::wstring expected = L"\t-> Hello, world!";
            Assert::AreEqual(expected, arg_parser_add_wstring_behind_multiline_text(input, prefix));
        }

        TEST_METHOD(TestMultipleLinesWithPrefix)
        {
            std::wstring input = L"Line one.\nLine two.\nLine three.";
            std::wstring prefix = L"\t* ";
            std::wstring expected = L"\t* Line one.\n\t* Line two.\n\t* Line three.";
            Assert::AreEqual(expected, arg_parser_add_wstring_behind_multiline_text(input, prefix));
        }

        TEST_METHOD(TestLinesWithEmptyLines)
        {
            std::wstring input = L"Line one.\n\nLine three.";
            std::wstring prefix = L"# ";
            std::wstring expected = L"# Line one.\n\n# Line three.";
            Assert::AreEqual(expected, arg_parser_add_wstring_behind_multiline_text(input, prefix));
        }

        TEST_METHOD(TestNoPrefixOnEmptyLines)
        {
            std::wstring input = L"\n\n";
            std::wstring prefix = L"-> ";
            std::wstring expected = L"\n\n";
            Assert::AreEqual(expected, arg_parser_add_wstring_behind_multiline_text(input, prefix));
        }

        TEST_METHOD(TestPrefixWithSpecialCharacters)
        {
            std::wstring input = L"Special line.";
            std::wstring prefix = L"*** ";
            std::wstring expected = L"*** Special line.";
            Assert::AreEqual(expected, arg_parser_add_wstring_behind_multiline_text(input, prefix));
        }
        TEST_METHOD(TestPrefixWithSeriesOfNewLines)
        {
            std::wstring input = L"\n\n\n";
            std::wstring prefix = L"*** ";
            std::wstring expected = L"\n\n\n";
            Assert::AreEqual(expected, arg_parser_add_wstring_behind_multiline_text(input, prefix));
        }
        TEST_METHOD(TestPrefixAndMultilineSpacing)
        {
            std::wstring input = L"Line one.\n\nLine two.\n\n";
            std::wstring prefix = L"--> ";
            std::wstring expected = L"--> Line one.\n\n--> Line two.\n";
            Assert::AreEqual(expected, arg_parser_add_wstring_behind_multiline_text(input, prefix));
        }

    };
}