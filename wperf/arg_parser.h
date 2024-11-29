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

#include <map>
#include <vector>
#include <deque>
#include <array>
#include <string>
#include <set>
#include <unordered_map>
#include "arg_parser_arg.h"

using namespace std;


typedef std::vector<std::wstring> wstr_vec;
using namespace ArgParserArg;

namespace ArgParser {
    enum class COMMAND_CLASS {
        STAT,
        SAMPLE,
        RECORD,
        TEST,
        DETECT,
        HELP,
        VERSION,
        LIST,
        MAN,
        NO_COMMAND
    };
    class arg_parser_arg_command : public arg_parser_arg_opt {

    public:
        arg_parser_arg_command(
            const std::wstring name,
            const std::vector<std::wstring> alias,
            const std::wstring description,
            const std::wstring useage_text,
            const COMMAND_CLASS command,
            const wstr_vec examples
        ) : arg_parser_arg_opt(name, alias, description), m_examples(examples), m_command(command), m_useage_text(useage_text) {};
        const COMMAND_CLASS m_command = COMMAND_CLASS::NO_COMMAND;
        const wstr_vec m_examples;
        const std::wstring m_useage_text;

        wstring get_usage_text() const override;

        wstring get_examples() const;

    };
#pragma region arg structs


#pragma endregion

    class arg_parser
    {
    public:
#pragma region Methods
        arg_parser();
        void parse(
            _In_ const int argc,
            _In_reads_(argc) const wchar_t* argv[]
        );
        void print_help() const;
#pragma endregion

#pragma region Commands
        arg_parser_arg_command list_command = arg_parser_arg_command::arg_parser_arg_command(
            L"list",
            { L"-l" },
            L"List supported events and metrics. Enable verbose mode for more details.",
            L"wperf list [-v] [--json] [--force-lock]",
            COMMAND_CLASS::LIST,
            {
                L"> wperf list -v List all events and metrics available on your host with extended information."
            }
        );
        arg_parser_arg_command test_command = arg_parser_arg_command::arg_parser_arg_command(
            L"test",
            { L"" },
            L"Configuration information about driver and application.",
            L"wperf test [--json] [OPTIONS]",
            COMMAND_CLASS::TEST,
            {}
        );
        arg_parser_arg_command help_command = arg_parser_arg_command::arg_parser_arg_command(
            L"-h",
            { L"--help" },
            L"Run wperf help command.",
            L"wperf help",
            COMMAND_CLASS::HELP,
            {}
        );
        arg_parser_arg_command version_command = arg_parser_arg_command::arg_parser_arg_command(
            L"--version",
            { L"" },
            L"Display version.",
            L"wperf --version",
            COMMAND_CLASS::VERSION,
            {}
        );
        arg_parser_arg_command detect_command = arg_parser_arg_command::arg_parser_arg_command(
            L"detect",
            { L"" },
            L"List installed WindowsPerf-like Kernel Drivers (match GUID).",
            L"wperf detect [--json] [OPTIONS]",
            COMMAND_CLASS::DETECT,
            {}
        );
        arg_parser_arg_command sample_command = arg_parser_arg_command::arg_parser_arg_command(
            L"sample",
            { L"" },
            L"Sampling mode, for determining the frequencies of event occurrences produced by program locations at the function, basic block, and /or instruction levels.",
            L"wperf sample [-e] [--timeout] [-c] [-C] [-E] [-q] [--json] [--output] [--config] [--image_name] [--pe_file] [--pdb_file] [--sample-display-long] [--force-lock] [--sample-display-row] [--symbol] [--record_spawn_delay] [--annotate] [--disassemble]",
            COMMAND_CLASS::SAMPLE,
            {
                L"> wperf sample -e ld_spec:100000 --pe_file python_d.exe -c 1 Sample event `ld_spec` with frequency `100000` already running process `python_d.exe` on core #1. Press Ctrl + C to stop sampling and see the results.",
            }
            );
        arg_parser_arg_command record_command = arg_parser_arg_command::arg_parser_arg_command(
            L"record",
            { L"" },
            L"Same as sample but also automatically spawns the process and pins it to the core specified by `-c`. Process name is defined by COMMAND.User can pass verbatim arguments to the process with[ARGS].",
            L"wperf record [-e] [--timeout] [-c] [-C] [-E] [-q] [--json] [--output] [--config] [--image_name] [--pe_file] [--pdb_file] [--sample-display-long] [--force-lock] [--sample-display-row] [--symbol] [--record_spawn_delay] [--annotate] [--disassemble] --COMMAND[ARGS]",
            COMMAND_CLASS::RECORD,
            {
                L"> wperf record -e ld_spec:100000 -c 1 --timeout 30 -- python_d.exe -c 10**10**100 Launch `python_d.exe - c 10 * *10 * *100` process and start sampling event `ld_spec` with frequency `100000` on core #1 for 30 seconds. Hint: add `--annotate` or `--disassemble` to `wperf record` command line parameters to increase sampling \"resolution\"."
    #ifdef ENABLE_SPE
               ,L"> wperf record -e arm_spe_0/ld=1/ -c 8 --cpython\\PCbuild\\arm64\\python_d.exe -c 10**10**100 Launch `python_d.exe -c 10**10**100` process on core no. 8 and start SPE sampling, enable collection of load sampled operations, including atomic operations that return a value to a register. Hint: add `--annotate` or `--disassemble` to `wperf record` command."
    #endif
            }
        );
        arg_parser_arg_command count_command = arg_parser_arg_command::arg_parser_arg_command(
            L"stat",
            { L"" },
            L"Counting mode, for obtaining aggregate counts of occurrences of special events.",
            L"wperf stat [-e] [-m] [-t] [-i] [-n] [-c] [-C] [-E] [-k] [--dmc] [-q] [--json] [--output][--config] [--force-lock] --COMMAND[ARGS]",
            COMMAND_CLASS::STAT,
            {
                L"> wperf stat -e inst_spec,vfp_spec,ase_spec,ld_spec -c 0 --timeout 3 Count events `inst_spec`, `vfp_spec`, `ase_spec` and `ld_spec` on core #0 for 3 seconds.",
                L"> wperf stat -m imix -e l1i_cache -c 7 --timeout 10.5 Count metric `imix` (metric events will be grouped) and additional event `l1i_cache` on core #7 for 10.5 seconds.",
                L"> wperf stat -m imix -c 1 -t -i 2 -n 3 --timeout 5 Count in timeline mode(output counting to CSV file) metric `imix` 3 times on core #1 with 2 second intervals(delays between counts).Each count will last 5 seconds."
            }
        );
        arg_parser_arg_command man_command = arg_parser_arg_command::arg_parser_arg_command(
            L"man",
            { L"" },
            L"Plain text information about one or more specified event(s), metric(s), and or group metric(s).",
            L"wperf man [--json]",
            COMMAND_CLASS::MAN,
            {
            }
            );

#pragma endregion

#pragma region Boolean Flags
        arg_parser_arg_opt json_opt = arg_parser_arg_opt::arg_parser_arg_opt(
            L"--json",
            {},
            L"Define output type as JSON.",
            {}
        );
        arg_parser_arg_opt kernel_opt = arg_parser_arg_opt::arg_parser_arg_opt(
            L"-k",
            {},
            L"Count kernel mode as well (disabled by default).",
            {}
        );
        arg_parser_arg_opt force_lock_opt = arg_parser_arg_opt::arg_parser_arg_opt(
            L"--force-lock",
            {},
            L"Force driver to give lock to current `wperf` process, use when you want to interrupt currently executing `wperf` session or to recover from the lock.",
            {}
        );
        // used to be called sample_display_short
        arg_parser_arg_opt sample_display_long_opt = arg_parser_arg_opt::arg_parser_arg_opt(
            L"--sample-display-long",
            {},
            L"Display decorated symbol names.",
            {}
        );
        arg_parser_arg_opt verbose_opt = arg_parser_arg_opt::arg_parser_arg_opt(
            L"--verbose",
            { L"-v" },
            L"Enable verbose output also in JSON output.",
            {}
        );
        arg_parser_arg_opt quite_opt = arg_parser_arg_opt::arg_parser_arg_opt(
            L"-q",
            {},
            L"Quiet mode, no output is produced.",
            {}
        );
        arg_parser_arg_opt annotate_opt = arg_parser_arg_opt::arg_parser_arg_opt(
            L"--annotate",
            {},
            L"Enable translating addresses taken from samples in sample/record mode into source code line numbers.",
            {}
        );
        arg_parser_arg_opt disassembly_opt = arg_parser_arg_opt::arg_parser_arg_opt(
            L"--disassemble",
            {},
            L"Enable disassemble output on sampling mode. Implies 'annotate'.",
            {}
        );
        arg_parser_arg_opt timeline_opt = arg_parser_arg_opt::arg_parser_arg_opt(
            L"-t",
            {},
            L"Enable timeline mode (count multiple times with specified interval). Use `-i` to specify timeline interval, and `-n` to specify number of counts.",
            {}
        );


#pragma endregion

#pragma region Flags with arguments
        arg_parser_arg_pos extra_args_arg = arg_parser_arg_pos::arg_parser_arg_pos(
            L"--",
            {},
            L"-- Process name is defined by COMMAND. User can pass verbatim arguments to the process with[ARGS].",
            {},
            -1
        );

        arg_parser_arg_pos cores_arg = arg_parser_arg_pos::arg_parser_arg_pos(
            L"-c",
            { L"--cores" },
            L"Specify comma separated list of CPU cores, and or ranges of CPU cores, to count on, or one CPU to sample on.",
            {}
        );

        arg_parser_arg_pos timeout_arg = arg_parser_arg_pos::arg_parser_arg_pos(
            L"--timeout",
            { L"sleep" },
            L"Specify counting or sampling duration. If not specified, press Ctrl+C to interrupt counting or sampling. Input may be suffixed by one (or none) of the following units, with up to 2 decimal points: \"ms\", \"s\", \"m\", \"h\", \"d\" (i.e. milliseconds, seconds, minutes, hours, days). If no unit is provided, the default unit is seconds. Accuracy is 0.1 sec.",
            {}
        );
        arg_parser_arg_pos symbol_arg = arg_parser_arg_pos::arg_parser_arg_pos(
            L"--symbol",
            { L"-s" },
            L"Filter results for specific symbols (for use with 'record' and 'sample' commands).",
            {}
        );
        arg_parser_arg_pos record_spawn_delay_arg = arg_parser_arg_pos::arg_parser_arg_pos(
            L"--record_spawn_delay",
            {},
            L"Set the waiting time, in milliseconds, before reading process data after spawning it with `record`.",
            {}
        );
        arg_parser_arg_pos sample_display_row_arg = arg_parser_arg_pos::arg_parser_arg_pos(
            L"--sample-display-row",
            {},
            L"Set how many samples you want to see in the summary (50 by default).",
            { L"50" }
        );
        arg_parser_arg_pos pe_file_arg = arg_parser_arg_pos::arg_parser_arg_pos(
            L"--pe_file",
            {},
            L"Specify the PE filename (and path).",
            {}
        );
        arg_parser_arg_pos image_name_arg = arg_parser_arg_pos::arg_parser_arg_pos(
            L"--image_name",
            {},
            L"Specify the image name you want to sample.",
            {}
        );
        arg_parser_arg_pos pdb_file_arg = arg_parser_arg_pos::arg_parser_arg_pos(
            L"--pdb_file",
            {},
            L"Specify the PDB filename (and path), PDB file should directly corresponds to a PE file set with `--pe_file`.",
            {}
        );
        arg_parser_arg_pos metric_config_arg = arg_parser_arg_pos::arg_parser_arg_pos(
            L"-C",
            {},
            L"Provide customized config file which describes metrics.",
            {}
        );
        arg_parser_arg_pos event_config_arg = arg_parser_arg_pos::arg_parser_arg_pos(
            L"-E",
            {},
            L"Provide customized config file which describes custom events or provide custom events from the command line.",
            {}
        );
        arg_parser_arg_pos output_filename_arg = arg_parser_arg_pos::arg_parser_arg_pos(
            L"--output",
            { L"-o" },
            L"Specify JSON output filename.",
            {}
        );
        arg_parser_arg_pos output_csv_filename_arg = arg_parser_arg_pos::arg_parser_arg_pos(
            L"--output-csv",
            {},
            L"Specify CSV output filename. Only with timeline `-t`.",
            {}
        );
        arg_parser_arg_pos output_prefix_arg = arg_parser_arg_pos::arg_parser_arg_pos(
            L"--output-prefix",
            { L"--cwd" },
            L"Set current working dir for storing output JSON and CSV file.",
            {}
        );
        arg_parser_arg_pos config_arg = arg_parser_arg_pos::arg_parser_arg_pos(
            L"--config",
            {},
            L"Specify configuration parameters.",
            {}
        );
        arg_parser_arg_pos interval_arg = arg_parser_arg_pos::arg_parser_arg_pos(
            L"-i",
            {},
            L"Specify counting interval. `0` seconds is allowed. Input may be suffixed with one(or none) of the following units, with up to 2 decimal points : \"ms\", \"s\", \"m\", \"h\", \"d\" (i.e.milliseconds, seconds, minutes, hours, days).If no unit is provided, the default unit is seconds(60s by default).",
            {}
        );
        arg_parser_arg_pos iteration_arg = arg_parser_arg_pos::arg_parser_arg_pos(
            L"-i",
            {},
            L"Number of consecutive counts in timeline mode (disabled by default).",
            {}
        );
        arg_parser_arg_pos dmc_arg = arg_parser_arg_pos::arg_parser_arg_pos(
            L"--dmc",
            {},
            L"Profile on the specified DDR controller. Skip `--dmc` to count on all DMCs.",
            {}
        );
        arg_parser_arg_pos metrics_arg = arg_parser_arg_pos::arg_parser_arg_pos(
            L"-m",
            {},
            L"Specify comma separated list of metrics to count.\n\nNote: see list of available metric names using `list` command.",
            {}
        );
        arg_parser_arg_pos events_arg = arg_parser_arg_pos::arg_parser_arg_pos(
            L"-e",
            {},
            L"Specify comma separated list of event names (or raw events) to count, for example `ld_spec,vfp_spec,r10`. Use curly braces to group events. Specify comma separated list of event names with sampling frequency to sample, for example `ld_spec:100000`. Raw events: specify raw evens with `r<VALUE>` where `<VALUE>` is a 16-bit hexadecimal event index value without leading `0x`. For example `r10` is event with index `0x10`. Note: see list of available event names using `list` command.",
            {}
        );
#pragma endregion

#pragma region Attributes

        COMMAND_CLASS m_command = COMMAND_CLASS::NO_COMMAND;
        std::vector<arg_parser_arg_command*> m_commands_list = {
           &help_command,
           &version_command,
           &sample_command,
           &count_command,
           &record_command,
           &list_command,
           &test_command,
           &detect_command,
           &man_command
        };

        std::vector<arg_parser_arg*> m_flags_list = {
           &json_opt,
           &metrics_arg,
           &events_arg,
           &kernel_opt,
           &force_lock_opt,
           &sample_display_long_opt,
           &verbose_opt,
           &quite_opt,
           &annotate_opt,
           &disassembly_opt,
           &timeline_opt,
           &cores_arg,
           &timeout_arg,
           &symbol_arg,
           &record_spawn_delay_arg,
           &sample_display_row_arg,
           &pe_file_arg,
           &image_name_arg,
           &pdb_file_arg,
           &metric_config_arg,
           &event_config_arg,
           &output_filename_arg,
           &output_csv_filename_arg,
           &output_prefix_arg,
           &config_arg,
           &interval_arg,
           &iteration_arg,
           &dmc_arg,
           &extra_args_arg
        };

        wstr_vec m_arg_array;

#pragma endregion

#pragma region Protected Methods
    protected:
        void throw_invalid_arg(const std::wstring& arg, const std::wstring& additional_message = L"") const;
#pragma endregion
    };

}
