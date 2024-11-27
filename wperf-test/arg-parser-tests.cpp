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
#include "wperf/arg-parser.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace ArgParser;

namespace arg_parser_tests
{

    TEST_CLASS(parsertests)
    {
    public:
        bool check_value_in_vector(const std::vector<std::wstring>& vec, const std::wstring& value)
        {
            return std::find(vec.begin(), vec.end(), value) != vec.end();
        }
        TEST_METHOD(TEST_CORRECT_TEST_COMMAND)
        {
            const wchar_t* argv[] = { L"wperf", L"test", L"-v", L"--json" };
            int argc = 4;
            arg_parser parser;
            parser.parse(argc, argv);

            Assert::AreEqual(true, parser.verbose_opt.is_set());
            Assert::AreEqual(true, parser.json_opt.is_set());
            Assert::IsTrue(COMMAND_CLASS::TEST == parser.m_command);
        }
        TEST_METHOD(TEST_RANDOM_ARGS_REJECTION)
        {
            const wchar_t* argv[] = { L"wperf", L"test", L"-v", L"--json", L"random" };
            int argc = 5;
            arg_parser parser;
            Assert::ExpectException<std::invalid_argument>([&parser, argc, &argv]() {
                parser.parse(argc, argv);
                }
            );
        }
        // Test parsing the "help" command with no arguments
        TEST_METHOD(TEST_HELP_COMMAND)
        {
            const wchar_t* argv[] = { L"wperf", L"--help" };
            int argc = 2;
            arg_parser parser;
            parser.parse(argc, argv);

            Assert::IsTrue(parser.help_command.is_set());
            Assert::IsTrue(COMMAND_CLASS::HELP == parser.m_command);
        }

        // Test parsing the 'version' command with no arguments
        TEST_METHOD(TEST_VERSION_COMMAND)
        {
            const wchar_t* argv[] = { L"wperf", L"--version" };
            int argc = 2;
            arg_parser parser;
            parser.parse(argc, argv);

            Assert::IsTrue(parser.version_command.is_set());
            Assert::IsTrue(COMMAND_CLASS::VERSION == parser.m_command);
        }

        // Test parsing the 'list' command with no arguments
        TEST_METHOD(TEST_LIST_COMMAND)
        {
            const wchar_t* argv[] = { L"wperf", L"list" };
            int argc = 2;
            arg_parser parser;
            parser.parse(argc, argv);

            Assert::IsTrue(parser.list_command.is_set());
            Assert::IsTrue(COMMAND_CLASS::LIST == parser.m_command);
        }

        // Test parsing the 'record' command with command line separator and arguments
        TEST_METHOD(TEST_RECORD_COMMAND_WITH_PROCESS)
        {
            const wchar_t* argv[] = { L"wperf", L"record", L"--", L"notepad.exe", L"test_arg" };
            int argc = 5;
            arg_parser parser;
            parser.parse(argc, argv);
            Assert::IsTrue(parser.record_command.is_set());
            Assert::IsTrue(check_value_in_vector(parser.extra_args_arg.get_values(), L"notepad.exe"));
            Assert::IsTrue(COMMAND_CLASS::RECORD == parser.m_command);
        }

        // Test that missing required arguments cause exceptions
        TEST_METHOD(TEST_SAMPLE_COMMAND_MISSING_REQUIRED_ARG)
        {
            const wchar_t* argv[] = { L"wperf", L"sample", L"--timeout" };
            int argc = 3;
            arg_parser parser;
            Assert::ExpectException<std::invalid_argument>([&parser, argc, &argv]() {
                parser.parse(argc, argv);
                }
            );
        }

        // Test that invalid arguments cause exceptions
        TEST_METHOD(TEST_SIMPLE_INVALID_ARGUMENT)
        {
            const wchar_t* argv[] = { L"wperf", L"invalid_command" };
            int argc = 2;
            arg_parser parser;
            Assert::ExpectException<std::invalid_argument>([&parser, argc, &argv]() {
                parser.parse(argc, argv);
                }
            );
        }

        // Test parsing timeout with different units
        TEST_METHOD(TEST_TIMEOUT_WITH_UNITS)
        {
            const wchar_t* argv[] = { L"wperf", L"sample", L"--timeout", L"2m" };
            int argc = 4;
            arg_parser parser;
            parser.parse(argc, argv);

            Assert::IsTrue(check_value_in_vector(parser.timeout_arg.get_values(), L"2m"));
        }

        TEST_METHOD(TEST_INVALID_TIMEOUT_WITH_WRONG_FORMAT)
        {
            const wchar_t* argv[] = { L"wperf", L"sample", L"--timeout", L"5.4", L"ms" };
            int argc = 5;
            arg_parser parser;
            Assert::ExpectException<std::invalid_argument>([&parser, argc, &argv]() {
                parser.parse(argc, argv);
                }
            );
        }

        // Test parsing the 'detect' command
        TEST_METHOD(TEST_DETECT_COMMAND)
        {
            const wchar_t* argv[] = { L"wperf", L"detect" };
            int argc = 2;
            arg_parser parser;
            parser.parse(argc, argv);

            Assert::IsTrue(parser.detect_command.is_set());
            Assert::IsTrue(COMMAND_CLASS::DETECT == parser.m_command);
        }

        // Test parsing multiple flags tois_sether
        TEST_METHOD(TEST_MULTIPLE_FLAGS)
        {
            const wchar_t* argv[] = { L"wperf", L"sample", L"--verbose", L"-q", L"--json" };
            int argc = 5;
            arg_parser parser;
            parser.parse(argc, argv);

            Assert::IsTrue(parser.verbose_opt.is_set());
            Assert::IsTrue(parser.quite_opt.is_set());
            Assert::IsTrue(parser.json_opt.is_set());
            Assert::IsTrue(COMMAND_CLASS::SAMPLE == parser.m_command);
        }

        // Test parsing sample command with symbol argument
        TEST_METHOD(TEST_SAMPLE_COMMAND_WITH_SYMBOL)
        {
            const wchar_t* argv[] = { L"wperf", L"sample", L"--symbol", L"main" };
            int argc = 4;
            arg_parser parser;
            parser.parse(argc, argv);

            Assert::IsTrue(check_value_in_vector(parser.symbol_arg.get_values(), L"main"));
            Assert::IsTrue(COMMAND_CLASS::SAMPLE == parser.m_command);
        }

        // Test parsing sample command with sample display row
        TEST_METHOD(TEST_SAMPLE_DISPLAY_ROW)
        {
            const wchar_t* argv[] = { L"wperf", L"sample", L"--sample-display-row", L"100" };
            int argc = 4;
            arg_parser parser;
            parser.parse(argc, argv);

            Assert::IsTrue(check_value_in_vector(parser.sample_display_row_arg.get_values(), L"100"));
            Assert::IsTrue(COMMAND_CLASS::SAMPLE == parser.m_command);
        }

        // Test parsing sample command with pe_file
        TEST_METHOD(TEST_SAMPLE_COMMAND_WITH_PE_FILE)
        {
            const wchar_t* argv[] = { L"wperf", L"sample", L"--pe_file", L"C:\\Program\\sample.exe" };
            int argc = 4;
            arg_parser parser;

            parser.parse(argc, argv);

            Assert::IsTrue(check_value_in_vector(parser.pe_file_arg.get_values(), L"C:\\Program\\sample.exe"));
            Assert::IsTrue(COMMAND_CLASS::SAMPLE == parser.m_command);
        }

        // Test parsing sample command with pdb_file
        TEST_METHOD(TEST_SAMPLE_COMMAND_WITH_PDB_FILE)
        {
            const wchar_t* argv[] = { L"wperf", L"sample", L"--pdb_file", L"C:\\Program\\sample.pdb" };
            int argc = 4;
            arg_parser parser;

            // Similarly, adjust or mock check_file_path for testing
            parser.parse(argc, argv);

            Assert::IsTrue(check_value_in_vector(parser.pdb_file_arg.get_values(), L"C:\\Program\\sample.pdb"));
            Assert::IsTrue(COMMAND_CLASS::SAMPLE == parser.m_command);
        }

        // Test parsing sample command with image_name
        TEST_METHOD(TEST_SAMPLE_COMMAND_WITH_IMAGE_NAME)
        {
            const wchar_t* argv[] = { L"wperf", L"sample", L"--image_name", L"notepad.exe" };
            int argc = 4;
            arg_parser parser;
            parser.parse(argc, argv);

            Assert::IsTrue(check_value_in_vector(parser.image_name_arg.get_values(), L"notepad.exe"));
            Assert::IsTrue(COMMAND_CLASS::SAMPLE == parser.m_command);
        }

        // Test parsing with --force-lock flag
        TEST_METHOD(TEST_FORCE_LOCK_FLAG)
        {
            const wchar_t* argv[] = { L"wperf", L"test", L"--force-lock" };
            int argc = 3;
            arg_parser parser;
            parser.parse(argc, argv);

            Assert::IsTrue(parser.force_lock_opt.is_set());
            Assert::IsTrue(COMMAND_CLASS::TEST == parser.m_command);
        }

        // Test parsing the 'stat' command (not fully implemented in the parser)
        TEST_METHOD(TEST_STAT_COMMAND)
        {
            const wchar_t* argv[] = { L"wperf", L"stat" };
            int argc = 2;
            arg_parser parser;
            parser.parse(argc, argv);

            // Assuming command is set correctly
            Assert::IsTrue(COMMAND_CLASS::STAT == parser.m_command);
        }

        // Test parsing with unknown flags
        TEST_METHOD(TEST_UNKNOWN_FLAG)
        {
            const wchar_t* argv[] = { L"wperf", L"sample", L"--unknown" };
            int argc = 3;
            arg_parser parser;
            Assert::ExpectException<std::invalid_argument>([&parser, argc, &argv]() {
                parser.parse(argc, argv);
                }
            );
        }

        // Test parsing with no command
        TEST_METHOD(TEST_NO_COMMAND)
        {
            const wchar_t* argv[] = { L"wperf", L"--annotate", L"--json" };
            int argc = 3;
            arg_parser parser;
            Assert::ExpectException<std::invalid_argument>([&parser, argc, &argv]() {
                parser.parse(argc, argv);
                }
            );
        }
        // Test parsing with the same command called twice
        TEST_METHOD(TEST_DUPLICATE_ARGUMENT)
        {
            const wchar_t* argv[] = { L"wperf", L"stat", L"-e", L"ld_spec", L"-e", L"vfp_spec" };
            int argc = 6;
            arg_parser parser;
            parser.parse(argc, argv);
            Assert::AreEqual(parser.events_arg.get_values().size(), size_t(2));
            Assert::IsTrue(parser.events_arg.is_set());
            Assert::IsTrue(check_value_in_vector(parser.events_arg.get_values(), L"vfp_spec"));
            Assert::IsTrue(check_value_in_vector(parser.events_arg.get_values(), L"ld_spec"));
        }

        // Test complex stat command 
        TEST_METHOD(TEST_FULL_STAT_COMMAND)
        {
            const wchar_t* argv[] = { L"wperf", L"stat", L"--output", L"_output_02.json", L"-e", L"inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec,br_immed_spec,crypto_spec", L"-c", L"0", L"sleep", L"5" };
            int argc = 10;
            arg_parser parser;
            parser.parse(argc, argv);
            Assert::IsTrue(COMMAND_CLASS::STAT == parser.m_command);
            Assert::IsTrue(parser.events_arg.is_set());
            Assert::IsTrue(parser.output_filename_arg.is_set());
            Assert::IsTrue(parser.cores_arg.is_set());
            Assert::IsTrue(parser.timeout_arg.is_set());
            Assert::IsTrue(check_value_in_vector(parser.timeout_arg.get_values(), L"5"));
            Assert::IsTrue(check_value_in_vector(parser.cores_arg.get_values(), L"0"));
            Assert::IsTrue(check_value_in_vector(parser.output_filename_arg.get_values(), L"_output_02.json"));
            Assert::IsTrue(check_value_in_vector(parser.events_arg.get_values(), L"inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec,br_immed_spec,crypto_spec"));
        }
    };
}
