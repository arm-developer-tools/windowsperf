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
#include "wperf/arg_parser.h"
#include "wperf/exception.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace ArgParser;

namespace wperftest
{

    TEST_CLASS(ArgParser)
    {
    public:
        bool check_value_in_vector(const std::vector<std::wstring>& vec, const std::wstring& value)
        {
            return std::find(vec.begin(), vec.end(), value) != vec.end();
        }
        TEST_METHOD(test_correct_test_command)
        {
            const wchar_t* argv[] = { L"wperf", L"test", L"-v", L"--json" };
            arg_parser parser;
            parser.parse(_countof(argv), argv);

            Assert::AreEqual(true, parser.verbose_opt.is_set());
            Assert::AreEqual(true, parser.json_opt.is_set());
            Assert::IsTrue(COMMAND_CLASS::TEST == parser.m_command);
        }
        TEST_METHOD(test_random_args_rejection)
        {
            const wchar_t* argv[] = { L"wperf", L"test", L"-v", L"--json", L"random" };
            const int argc = _countof(argv);
            arg_parser parser;
            Assert::ExpectException<fatal_exception>([&parser, argc, &argv]() {
                parser.parse(argc, argv);
                }
            );
        }
        // Test parsing the "help" command with no arguments
        TEST_METHOD(test_help_command)
        {
            const wchar_t* argv[] = { L"wperf", L"--help" };
            arg_parser parser;
            parser.parse(_countof(argv), argv);

            Assert::IsTrue(parser.help_command.is_set());
            Assert::IsTrue(COMMAND_CLASS::HELP == parser.m_command);
        }

        // Test parsing the 'version' command with no arguments
        TEST_METHOD(test_version_command)
        {
            const wchar_t* argv[] = { L"wperf", L"--version" };
            arg_parser parser;
            parser.parse(_countof(argv), argv);

            Assert::IsTrue(parser.version_command.is_set());
            Assert::IsTrue(COMMAND_CLASS::VERSION == parser.m_command);
        }

        // Test parsing the 'list' command with no arguments
        TEST_METHOD(test_list_command)
        {
            const wchar_t* argv[] = { L"wperf", L"list" };
            arg_parser parser;
            parser.parse(_countof(argv), argv);

            Assert::IsTrue(parser.list_command.is_set());
            Assert::IsTrue(COMMAND_CLASS::LIST == parser.m_command);
        }

        // Test parsing the 'record' command with command line separator and arguments
        TEST_METHOD(test_record_command_with_process)
        {
            const wchar_t* argv[] = { L"wperf", L"record", L"--", L"notepad.exe", L"test_arg" };
            arg_parser parser;
            parser.parse(_countof(argv), argv);
            Assert::IsTrue(parser.record_command.is_set());
            Assert::IsTrue(check_value_in_vector(parser.extra_args_arg.get_values(), L"notepad.exe"));
            Assert::IsTrue(check_value_in_vector(parser.extra_args_arg.get_values(), L"test_arg"));
            Assert::IsTrue(COMMAND_CLASS::RECORD == parser.m_command);
        }

        // Test that missing required arguments cause exceptions
        TEST_METHOD(test_sample_command_missing_required_arg)
        {
            const wchar_t* argv[] = { L"wperf", L"sample", L"--timeout" };
            const int argc = _countof(argv);
            arg_parser parser;
            Assert::ExpectException<fatal_exception>([&parser, argc, &argv]() {
                parser.parse(argc, argv);
                }
            );
        }

        // Test that invalid arguments cause exceptions
        TEST_METHOD(test_simple_invalid_argument)
        {
            const wchar_t* argv[] = { L"wperf", L"invalid_command" };
            const int argc = _countof(argv);
            arg_parser parser;
            Assert::ExpectException<fatal_exception>([&parser, argc, &argv]() {
                parser.parse(argc, argv);
                }
            );
        }

        // Test parsing timeout with different units
        TEST_METHOD(test_timeout_with_units)
        {
            const wchar_t* argv[] = { L"wperf", L"sample", L"--timeout", L"2m" };
            arg_parser parser;
            parser.parse(_countof(argv), argv);

            Assert::IsTrue(check_value_in_vector(parser.timeout_arg.get_values(), L"2m"));
        }

        TEST_METHOD(test_invalid_timeout_with_wrong_format)
        {
            const wchar_t* argv[] = { L"wperf", L"sample", L"--timeout", L"5.4", L"ms" };
            const int argc = _countof(argv);
            arg_parser parser;
            Assert::ExpectException<fatal_exception>([&parser, argc, &argv]() {
                parser.parse(argc, argv);
                }
            );
        }

        // Test parsing the 'detect' command
        TEST_METHOD(test_detect_command)
        {
            const wchar_t* argv[] = { L"wperf", L"detect" };
            arg_parser parser;
            parser.parse(_countof(argv), argv);

            Assert::IsTrue(parser.detect_command.is_set());
            Assert::IsTrue(COMMAND_CLASS::DETECT == parser.m_command);
        }

        // Test parsing multiple flags tois_sether
        TEST_METHOD(test_multiple_flags)
        {
            const wchar_t* argv[] = { L"wperf", L"sample", L"--verbose", L"-q", L"--json" };
            arg_parser parser;
            parser.parse(_countof(argv), argv);

            Assert::IsTrue(parser.verbose_opt.is_set());
            Assert::IsTrue(parser.quite_opt.is_set());
            Assert::IsTrue(parser.json_opt.is_set());
            Assert::IsTrue(COMMAND_CLASS::SAMPLE == parser.m_command);
        }

        // Test parsing sample command with symbol argument
        TEST_METHOD(test_sample_command_with_symbol)
        {
            const wchar_t* argv[] = { L"wperf", L"sample", L"--symbol", L"main" };
            arg_parser parser;
            parser.parse(_countof(argv), argv);

            Assert::IsTrue(check_value_in_vector(parser.symbol_arg.get_values(), L"main"));
            Assert::IsTrue(COMMAND_CLASS::SAMPLE == parser.m_command);
        }

        // Test parsing sample command with sample display row
        TEST_METHOD(test_sample_display_row)
        {
            const wchar_t* argv[] = { L"wperf", L"sample", L"--sample-display-row", L"100" };
            arg_parser parser;
            parser.parse(_countof(argv), argv);

            Assert::IsTrue(check_value_in_vector(parser.sample_display_row_arg.get_values(), L"100"));
            Assert::IsTrue(COMMAND_CLASS::SAMPLE == parser.m_command);
        }

        // Test parsing sample command with pe_file
        TEST_METHOD(test_sample_command_with_pe_file)
        {
            const wchar_t* argv[] = { L"wperf", L"sample", L"--pe_file", L"C:\\Program\\sample.exe" };
            arg_parser parser;

            parser.parse(_countof(argv), argv);

            Assert::IsTrue(check_value_in_vector(parser.pe_file_arg.get_values(), L"C:\\Program\\sample.exe"));
            Assert::IsTrue(COMMAND_CLASS::SAMPLE == parser.m_command);
        }

        // Test parsing sample command with pdb_file
        TEST_METHOD(test_sample_command_with_pdb_file)
        {
            const wchar_t* argv[] = { L"wperf", L"sample", L"--pdb_file", L"C:\\Program\\sample.pdb" };
            arg_parser parser;

            // Similarly, adjust or mock check_file_path for testing
            parser.parse(_countof(argv), argv);

            Assert::IsTrue(check_value_in_vector(parser.pdb_file_arg.get_values(), L"C:\\Program\\sample.pdb"));
            Assert::IsTrue(COMMAND_CLASS::SAMPLE == parser.m_command);
        }

        // Test parsing sample command with image_name
        TEST_METHOD(test_sample_command_with_image_name)
        {
            const wchar_t* argv[] = { L"wperf", L"sample", L"--image_name", L"notepad.exe" };
            arg_parser parser;
            parser.parse(_countof(argv), argv);

            Assert::IsTrue(check_value_in_vector(parser.image_name_arg.get_values(), L"notepad.exe"));
            Assert::IsTrue(COMMAND_CLASS::SAMPLE == parser.m_command);
        }

        // Test parsing with --force-lock flag
        TEST_METHOD(test_force_lock_flag)
        {
            const wchar_t* argv[] = { L"wperf", L"test", L"--force-lock" };
            arg_parser parser;
            parser.parse(_countof(argv), argv);

            Assert::IsTrue(parser.force_lock_opt.is_set());
            Assert::IsTrue(COMMAND_CLASS::TEST == parser.m_command);
        }

        // Test parsing the 'stat' command (not fully implemented in the parser)
        TEST_METHOD(test_stat_command)
        {
            const wchar_t* argv[] = { L"wperf", L"stat" };
            arg_parser parser;
            parser.parse(_countof(argv), argv);

            // Assuming command is set correctly
            Assert::IsTrue(COMMAND_CLASS::STAT == parser.m_command);
        }

        // Test parsing with unknown flags
        TEST_METHOD(test_unknown_flag)
        {
            const wchar_t* argv[] = { L"wperf", L"sample", L"--unknown" };
            const int argc = _countof(argv);
            arg_parser parser;
            Assert::ExpectException<fatal_exception>([&parser, argc, &argv]() {
                parser.parse(argc, argv);
                }
            );
        }

        // Test parsing with no command
        TEST_METHOD(test_no_command)
        {
            const wchar_t* argv[] = { L"wperf", L"--annotate", L"--json" };
            const int argc = _countof(argv);
            arg_parser parser;
            Assert::ExpectException<fatal_exception>([&parser, argc, &argv]() {
                parser.parse(argc, argv);
                }
            );
        }
        // Test parsing with the same command called twice
        TEST_METHOD(test_duplicate_argument)
        {
            const wchar_t* argv[] = { L"wperf", L"stat", L"-e", L"ld_spec", L"-e", L"vfp_spec" };
            arg_parser parser;
            parser.parse(_countof(argv), argv);
            Assert::AreEqual(parser.events_arg.get_values().size(), size_t(2));
            Assert::IsTrue(parser.events_arg.is_set());
            Assert::IsTrue(check_value_in_vector(parser.events_arg.get_values(), L"vfp_spec"));
            Assert::IsTrue(check_value_in_vector(parser.events_arg.get_values(), L"ld_spec"));
        }

        // Test complex stat command 
        TEST_METHOD(test_full_stat_command)
        {
            const wchar_t* argv[] = { L"wperf", L"stat", L"--output", L"_output_02.json", L"-e", L"inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec,br_immed_spec,crypto_spec", L"-c", L"0", L"sleep", L"5" };
            arg_parser parser;
            parser.parse(_countof(argv), argv);

            // Check all _command flags setup
            Assert::IsFalse(parser.record_command.is_set());
            Assert::IsFalse(parser.list_command.is_set());
            Assert::IsFalse(parser.help_command.is_set());
            Assert::IsFalse(parser.version_command.is_set());
            Assert::IsFalse(parser.detect_command.is_set());
            Assert::IsFalse(parser.sample_command.is_set());
            Assert::IsTrue(parser.count_command.is_set());
            Assert::IsFalse(parser.man_command.is_set());

            // Check all _arg flags setup
            Assert::IsFalse(parser.extra_args_arg.is_set());
            Assert::IsTrue(parser.cores_arg.is_set());
            Assert::IsTrue(parser.timeout_arg.is_set());
            Assert::IsFalse(parser.symbol_arg.is_set());
            Assert::IsFalse(parser.record_spawn_delay_arg.is_set());
            Assert::IsFalse(parser.sample_display_row_arg.is_set());
            Assert::IsFalse(parser.pe_file_arg.is_set());
            Assert::IsFalse(parser.image_name_arg.is_set());
            Assert::IsFalse(parser.pdb_file_arg.is_set());
            Assert::IsFalse(parser.metric_config_arg.is_set());
            Assert::IsFalse(parser.event_config_arg.is_set());
            Assert::IsTrue(parser.output_filename_arg.is_set());
            Assert::IsFalse(parser.output_csv_filename_arg.is_set());
            Assert::IsFalse(parser.output_prefix_arg.is_set());
            Assert::IsFalse(parser.config_arg.is_set());
            Assert::IsFalse(parser.interval_arg.is_set());
            Assert::IsFalse(parser.iteration_arg.is_set());
            Assert::IsFalse(parser.dmc_arg.is_set());
            Assert::IsFalse(parser.metrics_arg.is_set());
            Assert::IsTrue(parser.events_arg.is_set());

            Assert::IsTrue(check_value_in_vector(parser.timeout_arg.get_values(), L"5"));
            Assert::IsTrue(check_value_in_vector(parser.cores_arg.get_values(), L"0"));
            Assert::IsTrue(check_value_in_vector(parser.output_filename_arg.get_values(), L"_output_02.json"));
            Assert::IsTrue(check_value_in_vector(parser.events_arg.get_values(), L"inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec,br_immed_spec,crypto_spec"));

            Assert::IsTrue(COMMAND_CLASS::STAT == parser.m_command);
        }

        // Test test command with set of config
        TEST_METHOD(test_test_json_config)
        {
            const wchar_t* argv[] = { L"wperf", L"test", L"--json", L"--config", L"count.period=13" };
            const int argc = _countof(argv);
            arg_parser parser;
            parser.parse(argc, argv);

            // Check all _command flags setup
            Assert::IsTrue(parser.test_command.is_set());
            Assert::IsFalse(parser.record_command.is_set());
            Assert::IsFalse(parser.list_command.is_set());
            Assert::IsFalse(parser.help_command.is_set());
            Assert::IsFalse(parser.version_command.is_set());
            Assert::IsFalse(parser.detect_command.is_set());
            Assert::IsFalse(parser.sample_command.is_set());
            Assert::IsFalse(parser.count_command.is_set());
            Assert::IsFalse(parser.man_command.is_set());

            // Check all _arg flags setup
            Assert::IsFalse(parser.extra_args_arg.is_set());
            Assert::IsFalse(parser.cores_arg.is_set());
            Assert::IsFalse(parser.timeout_arg.is_set());
            Assert::IsFalse(parser.symbol_arg.is_set());
            Assert::IsFalse(parser.record_spawn_delay_arg.is_set());
            Assert::IsFalse(parser.sample_display_row_arg.is_set());
            Assert::IsFalse(parser.pe_file_arg.is_set());
            Assert::IsFalse(parser.image_name_arg.is_set());
            Assert::IsFalse(parser.pdb_file_arg.is_set());
            Assert::IsFalse(parser.metric_config_arg.is_set());
            Assert::IsFalse(parser.event_config_arg.is_set());
            Assert::IsFalse(parser.output_filename_arg.is_set());
            Assert::IsFalse(parser.output_csv_filename_arg.is_set());
            Assert::IsFalse(parser.output_prefix_arg.is_set());
            Assert::IsTrue(parser.config_arg.is_set());
            Assert::IsFalse(parser.interval_arg.is_set());
            Assert::IsFalse(parser.iteration_arg.is_set());
            Assert::IsFalse(parser.dmc_arg.is_set());
            Assert::IsFalse(parser.metrics_arg.is_set());
            Assert::IsFalse(parser.events_arg.is_set());

            // Check all _opt flags setup
            Assert::IsTrue(parser.json_opt.is_set());
            Assert::IsFalse(parser.kernel_opt.is_set());
            Assert::IsFalse(parser.force_lock_opt.is_set());
            Assert::IsFalse(parser.sample_display_long_opt.is_set());
            Assert::IsFalse(parser.verbose_opt.is_set());
            Assert::IsFalse(parser.quite_opt.is_set());
            Assert::IsFalse(parser.annotate_opt.is_set());
            Assert::IsFalse(parser.disassembly_opt.is_set());
            Assert::IsFalse(parser.timeline_opt.is_set());


            Assert::IsTrue(check_value_in_vector(parser.config_arg.get_values(), L"count.period=13"));
            Assert::IsTrue(COMMAND_CLASS::TEST == parser.m_command);

            // Double-dash aka "--"
            Assert::IsFalse(parser.extra_args_arg.is_set());
        }

        TEST_METHOD(test_test_record_spe_with_double_dash)
        {
            const wchar_t* argv[] = { L"wperf", L"record", L"-e", L"arm_spe_0/load_filter=0,store_filter/", L"-c", L"4", L"--timeout", L"3", L"--json", L"--", L"python_d.exe", L"-c", L"10**10**100" };
            const int argc = _countof(argv);
            arg_parser parser;
            parser.parse(argc, argv);

            Assert::IsTrue(parser.record_command.is_set());
            Assert::IsFalse(parser.list_command.is_set());
            Assert::IsFalse(parser.test_command.is_set());
            Assert::IsFalse(parser.help_command.is_set());
            Assert::IsFalse(parser.version_command.is_set());
            Assert::IsFalse(parser.detect_command.is_set());
            Assert::IsFalse(parser.sample_command.is_set());
            Assert::IsFalse(parser.count_command.is_set());
            Assert::IsFalse(parser.man_command.is_set());

            // Check all _opt flags setup
            Assert::IsTrue(parser.json_opt.is_set());
            Assert::IsFalse(parser.kernel_opt.is_set());
            Assert::IsFalse(parser.force_lock_opt.is_set());
            Assert::IsFalse(parser.sample_display_long_opt.is_set());
            Assert::IsFalse(parser.verbose_opt.is_set());
            Assert::IsFalse(parser.quite_opt.is_set());
            Assert::IsFalse(parser.annotate_opt.is_set());
            Assert::IsFalse(parser.disassembly_opt.is_set());
            Assert::IsFalse(parser.timeline_opt.is_set());

            // Check all _arg flags setup
            Assert::IsTrue(parser.extra_args_arg.is_set());
            Assert::IsTrue(parser.cores_arg.is_set());
            Assert::IsTrue(parser.timeout_arg.is_set());
            Assert::IsFalse(parser.symbol_arg.is_set());
            Assert::IsFalse(parser.record_spawn_delay_arg.is_set());
            Assert::IsFalse(parser.sample_display_row_arg.is_set());
            Assert::IsFalse(parser.pe_file_arg.is_set());
            Assert::IsFalse(parser.image_name_arg.is_set());
            Assert::IsFalse(parser.pdb_file_arg.is_set());
            Assert::IsFalse(parser.metric_config_arg.is_set());
            Assert::IsFalse(parser.event_config_arg.is_set());
            Assert::IsFalse(parser.output_filename_arg.is_set());
            Assert::IsFalse(parser.output_csv_filename_arg.is_set());
            Assert::IsFalse(parser.output_prefix_arg.is_set());
            Assert::IsFalse(parser.config_arg.is_set());
            Assert::IsFalse(parser.interval_arg.is_set());
            Assert::IsFalse(parser.iteration_arg.is_set());
            Assert::IsFalse(parser.dmc_arg.is_set());
            Assert::IsFalse(parser.metrics_arg.is_set());
            Assert::IsTrue(parser.events_arg.is_set());

            // Double-dash aka "--"
            Assert::IsTrue(parser.extra_args_arg.is_set());
            Assert::IsTrue(check_value_in_vector(parser.extra_args_arg.get_values(), L"-c"));
            Assert::IsTrue(check_value_in_vector(parser.extra_args_arg.get_values(), L"10**10**100"));

            Assert::IsTrue(COMMAND_CLASS::RECORD == parser.m_command);
        }

        TEST_METHOD(test_record_pmu_with_double_dash)
        {
            const wchar_t* argv[] = { L"wperf", L"record", L"-e", L"ld_spec:100000", L"-c", L"1", L"--symbol", L"^x_mul$", L"--timeout", L"3", L"--", L"cpython\\PCbuild\\arm64\\python_d.exe", L"-c", L"10**10**100" };
            const int argc = _countof(argv);
            arg_parser parser;
            parser.parse(argc, argv);

            Assert::IsTrue(parser.timeout_arg.is_set());
            Assert::IsTrue(parser.cores_arg.is_set());
            Assert::IsTrue(parser.events_arg.is_set());
            Assert::IsTrue(parser.symbol_arg.is_set());

            // Double-dash aka "--"
            Assert::IsTrue(parser.extra_args_arg.is_set());
            Assert::IsTrue(check_value_in_vector(parser.extra_args_arg.get_values(), L"-c"));
            Assert::IsTrue(check_value_in_vector(parser.extra_args_arg.get_values(), L"10**10**100"));

            Assert::IsTrue(COMMAND_CLASS::RECORD == parser.m_command);
        }

    };
}
