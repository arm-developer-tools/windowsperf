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

#include <filesystem>
#include <numeric>
#include <sstream>
#include <cwctype>
#include <array>
#include <unordered_map>
#include <regex>
#include <assert.h>
#include "user_request.h"
#include "exception.h"
#include "padding.h"
#include "wperf-common/public.h"
#include "wperf/config.h"

void user_request::print_help_usage()
{
    std::wstring wsHelp = LR"(
NAME:
    wperf - Performance analysis tools for Windows on Arm

SYNOPSIS:
    wperf [--version] [--help] [OPTIONS]

    wperf stat [-e] [-m] [-t] [-i] [-n] [-c] [-C] [-E] [-k] [--dmc] [-q] [--json]
               [--output] [--config] [--force-lock]
    wperf stat [-e] [-m] [-t] [-i] [-n] [-c] [-C] [-E] [-k] [--dmc] [-q] [--json]
               [--output] [--config] -- COMMAND [ARGS]
        Counting mode, for obtaining aggregate counts of occurrences of special
        events.

    wperf sample [-e] [--timeout] [-c] [-C] [-E] [-q] [--json] [--output] [--config]
                 [--image_name] [--pe_file] [--pdb_file] [--sample-display-long] [--force-lock]
                 [--sample-display-row] [--symbol] [--record_spawn_delay] [--annotate] [--disassemble]
        Sampling mode, for determining the frequencies of event occurrences
        produced by program locations at the function, basic block, and/or
        instruction levels.

    wperf record [-e] [--timeout] [-c] [-C] [-E] [-q] [--json] [--output] [--config]
                 [--image_name] [--pe_file] [--pdb_file] [--sample-display-long] [--force-lock]
                 [--sample-display-row] [--symbol] [--record_spawn_delay] [--annotate] [--disassemble] -- COMMAND [ARGS]
        Same as sample but also automatically spawns the process and pins it to
        the core specified by `-c`. Process name is defined by COMMAND. User can
        pass verbatim arguments to the process with [ARGS].

    wperf list [-v] [--json] [--force-lock]
        List supported events and metrics. Enable verbose mode for more details.

    wperf test [--json] [OPTIONS]
        Configuration information about driver and application.

    wperf detect [--json] [OPTIONS]
        List installed WindowsPerf-like Kernel Drivers (match GUID).

    wperf man [--json]
        Plain text information about one or more specified event(s), metric(s), and or group metric(s).

OPTIONS:
    -h, --help
        Run wperf help command.

    --version
        Display version.

    -v, --verbose
        Enable verbose output also in JSON output.

    -q
        Quiet mode, no output is produced.

    -e
        Specify comma separated list of event names (or raw events) to count, for
        example `ld_spec,vfp_spec,r10`. Use curly braces to group events.
        Specify comma separated list of event names with sampling frequency to
        sample, for example `ld_spec:100000`.

        Raw events: specify raw evens with `r<VALUE>` where `<VALUE>` is a 16-bit
        hexadecimal event index value without leading `0x`. For example `r10` is
        event with index `0x10`.

        Note: see list of available event names using `list` command.

    -m
        Specify comma separated list of metrics to count.

        Note: see list of available metric names using `list` command.

    --timeout
        Specify counting or sampling duration. If not specified, press
        Ctrl+C to interrupt counting or sampling. Input may be suffixed by
        one (or none) of the following units, with up to 2 decimal
        points: "ms", "s", "m", "h", "d" (i.e. milliseconds, seconds,
        minutes, hours, days). If no unit is provided, the default unit
        is seconds. Accuracy is 0.1 sec.

    -t
        Enable timeline mode (count multiple times with specified interval).
        Use `-i` to specify timeline interval, and `-n` to specify number of
        counts.

    -i
        Specify counting interval. `0` seconds is allowed. Input may be
        suffixed with one (or none) of the following units, with up to
        2 decimal points: "ms", "s", "m", "h", "d" (i.e. milliseconds,
        seconds, minutes, hours, days). If no unit is provided, the default
        unit is seconds (60s by default).

    -n
        Number of consecutive counts in timeline mode (disabled by default).

    --annotate
        Enable translating addresses taken from samples in sample/record mode into source code line numbers.

    --disassemble
        Enable disassemble output on sampling mode. Implies 'annotate'.

    --image_name
        Specify the image name you want to sample.

    --pe_file
        Specify the PE filename (and path).

    --pdb_file
        Specify the PDB filename (and path), PDB file should directly
        corresponds to a PE file set with `--pe_file`.

    --sample-display-long
        Display decorated symbol names.

    --sample-display-row
        Set how many samples you want to see in the summary (50 by default).

    --symbol
        Filter results for specific symbols (for use with 'record' and 'sample' commands).

    --record_spawn_delay
        Set the waiting time, in milliseconds, before reading process data after
        spawning it with `record`.

    --force-lock
        Force driver to give lock to current `wperf` process, use when you want
        to interrupt currently executing `wperf` session or to recover from the lock.

    -c, --cpu
        Specify comma separated list of CPU cores, and or ranges of CPU cores, to count
        on, or one CPU to sample on.

    -k
        Count kernel mode as well (disabled by default).

    --dmc
        Profile on the specified DDR controller. Skip `--dmc` to count on all
        DMCs.

    -C
        Provide customized config file which describes metrics.

    -E
        Provide customized config file which describes custom events or
        provide custom events from the command line.

    --json
        Define output type as JSON.

    --output, -o
        Specify JSON output filename.

    --output-csv
        Specify CSV output filename. Only with timeline `-t`.

    --output-prefix, --cwd
         Set current working dir for storing output JSON and CSV file.

    --config
        Specify configuration parameters.

OPTIONS aliases:
    -l
        Alias of 'list'.

    sleep
        Alias of `--timeout`.
    -s
        Alias of `--symbol`.

EXAMPLES:

    > wperf list -v
    List all events and metrics available on your host with extended
    information.

    > wperf stat -e inst_spec,vfp_spec,ase_spec,ld_spec -c 0 --timeout 3
    Count events `inst_spec`, `vfp_spec`, `ase_spec` and `ld_spec` on core #0
    for 3 seconds.

    > wperf stat -m imix -e l1i_cache -c 7 --timeout 10.5
    Count metric `imix` (metric events will be grouped) and additional event
    `l1i_cache` on core #7 for 10.5 seconds.

    > wperf stat -m imix -c 1 -t -i 2 -n 3 --timeout 5
    Count in timeline mode (output counting to CSV file) metric `imix` 3 times
    on core #1 with 2 second intervals (delays between counts). Each count
    will last 5 seconds.

    > wperf sample -e ld_spec:100000 --pe_file python_d.exe -c 1
    Sample event `ld_spec` with frequency `100000` already running process
    `python_d.exe` on core #1. Press Ctrl+C to stop sampling and see the results.

    > wperf record -e ld_spec:100000 -c 1 --timeout 30 -- python_d.exe -c 10**10**100
    Launch `python_d.exe -c 10**10**100` process and start sampling event `ld_spec`
    with frequency `100000` on core #1 for 30 seconds.
    Hint: add `--annotate` or `--disassemble` to `wperf record` command line
    parameters to increase sampling "resolution".
)";

#ifdef ENABLE_SPE
    wsHelp += LR"(
    > wperf record -e arm_spe_0/ld=1/ -c 8 --cpython\PCbuild\arm64\python_d.exe -c 10**10**100
    Launch `python_d.exe -c 10**10**100` process on core no. 8 and start SPE sampling, enable
    collection of load sampled operations, including atomic operations that return a value to a register.
    Hint: add `--annotate` or `--disassemble` to `wperf record` command.
)";
#endif

    m_out.GetOutputStream() << wsHelp << std::endl;
}

//
// This file will be modified with wperf's pre-build step
//
#include "wperf-common\gitver.h"

void user_request::print_help_header()
{
    m_out.GetOutputStream() << L"WindowsPerf"
        << L" ver. " << MAJOR << "." << MINOR << "." << PATCH
        << L" ("
        << WPERF_GIT_VER_STR
        << L"/"
#ifdef _DEBUG
        << L"Debug"
#else
        << L"Release"
#endif
        << ENABLE_FEAT_STR
        << L") WOA profiling with performance counters."
        << std::endl;

    m_out.GetOutputStream() << L"Report bugs to: https://gitlab.com/Linaro/WindowsPerf/windowsperf/-/issues"
        << std::endl;
}

void user_request::print_help_prompt()
{
    m_out.GetOutputStream() << L"Use --help for help." << std::endl;
}

void user_request::print_help()
{
    print_help_header();
    print_help_usage();
}


user_request::user_request() : do_list{ false }, do_disassembly(false), do_count(false), do_kernel(false), do_timeline(false),
    do_sample(false), do_record(false), do_annotate(false), do_version(false), do_verbose(false), do_test(false),
    do_help(false), do_man(false), do_export_perf_data(false), do_symbol(false), dmc_idx(_UI8_MAX), count_duration(-1.0),
    sample_image_name(L""), sample_pe_file(L""), sample_pdb_file(L""),
    sample_display_row(50), sample_display_short(true), count_timeline(0),
    count_interval(-1.0), report_l3_cache_metric(false), report_ddr_bw_metric(false) {}

bool user_request::is_cli_option_in_args(const wstr_vec& raw_args, std::wstring opt)
{
    // We will search only before double-dash is present as after "--" WindowsPerf CLI options end!
    auto last_iter = std::find_if(raw_args.begin(), raw_args.end(),
        [](const auto& arg) { return arg == std::wstring(L"--"); });
    return std::count_if(raw_args.begin(), last_iter,
        [opt](const auto& arg) { return arg == opt; });
}

bool user_request::is_force_lock(const wstr_vec& raw_args)
{
    return is_cli_option_in_args(raw_args, std::wstring(L"--force-lock"));
}

bool user_request::is_help(const wstr_vec& raw_args)
{
    return is_cli_option_in_args(raw_args, std::wstring(L"--help"))
        || is_cli_option_in_args(raw_args, std::wstring(L"-h"));
}

void user_request::init(wstr_vec& raw_args, const struct pmu_device_cfg& pmu_cfg,
    std::map<std::wstring, metric_desc>& builtin_metrics,
    const std::map <std::wstring, std::vector<std::wstring>>& groups_of_metrics,
    std::map<enum evt_class, std::vector<struct extra_event>>& extra_events)
{
    std::map<enum evt_class, std::deque<struct evt_noted>> events;
    std::map<enum evt_class, std::vector<struct evt_noted>> groups;

    // Fill cores_idx with {0, ... core_num}
    cores_idx.clear();
    cores_idx.resize(pmu_cfg.core_num);
    std::iota(cores_idx.begin(), cores_idx.end(), (UINT8)0);

    parse_raw_args(raw_args, pmu_cfg, events, groups, builtin_metrics, groups_of_metrics, extra_events);

    // Deduce image name and PDB file name from PE file name
    if (sample_pe_file.size())
    {
        if (sample_image_name.empty())
        {
            sample_image_name = sample_pe_file;
            if (do_verbose)
                m_out.GetOutputStream() << L"deduced image name '" << sample_image_name << L"'" << std::endl;
        }

        if (sample_pdb_file.empty())
        {
            sample_pdb_file = ReplaceFileExtension(sample_pe_file, L"pdb");
            if (do_verbose)
                m_out.GetOutputStream() << L"deduced image PDB file '" << sample_pdb_file << L"'" << std::endl;
        }

        if (do_verbose)
        {
            m_out.GetOutputStream() << L"pe_file '" << sample_pe_file << L"'";
            if (record_commandline.size())
                m_out.GetOutputStream() << L", args '" << record_commandline << L"'" << std::endl;
            m_out.GetOutputStream() << std::endl;
        }
    }
    else if (do_sample || do_record)
    {
        m_out.GetErrorOutputStream() << "no pid or process name specified, sample address are not de-ASLRed" << std::endl;
        throw fatal_exception("ERROR_IMAGE_NAME");
    }

    for (uint32_t core_idx : cores_idx)
        if (core_idx >= pmu_cfg.core_num) {
            m_out.GetErrorOutputStream() << L"core index " << core_idx << L" not allowed. Use 0-" << (pmu_cfg.core_num - 1)
                << L", see option -c <n>" << std::endl;
            throw fatal_exception("ERROR_CORES");
        }

    set_event_padding(ioctl_events, pmu_cfg, events, groups);
    check_events(EVT_CORE, MAX_MANAGED_CORE_EVENTS);
    check_events(EVT_DSU, MAX_MANAGED_DSU_EVENTS);
    check_events(EVT_DMC_CLK, MAX_MANAGED_DMC_CLK_EVENTS);
    check_events(EVT_DMC_CLKDIV2, MAX_MANAGED_DMC_CLKDIV2_EVENTS);
}

void user_request::parse_raw_args(wstr_vec& raw_args, const struct pmu_device_cfg& pmu_cfg,
    std::map<enum evt_class, std::deque<struct evt_noted>>& events,
    std::map<enum evt_class, std::vector<struct evt_noted>>& groups,
    std::map<std::wstring, metric_desc>& builtin_metrics,
    const std::map <std::wstring, std::vector<std::wstring>>& groups_of_metrics,
    std::map<enum evt_class, std::vector<struct extra_event>>& extra_events)
{   
    bool waiting_events = false;
    bool waiting_metrics = false;
    bool waiting_core_idx = false;
    bool waiting_dmc_idx = false;
    bool waiting_duration = false;
    bool waiting_interval = false;
    bool waiting_metric_config = false;
    bool waiting_events_config = false;
    bool waiting_output_filename = false;
    bool waiting_output_csv_filename = false;
    bool waiting_image_name = false;
    bool waiting_pe_file = false;
    bool waiting_pdb_file = false;
    bool waiting_sample_display_row = false;
    bool waiting_timeline_count = false;
    bool waiting_config = false;
    bool waiting_commandline = false;
    bool waiting_record_spawn_delay = false;
    bool waiting_man_query = false;
    bool waiting_cwd = false;
    bool waiting_symbol = false;

    bool sample_pe_file_given = false;

    std::wstring waiting_duration_arg;
    std::wstring output_filename, output_csv_filename;

    if (raw_args.empty())
    {
        print_help_header();
        print_help_prompt();
    }

    for (const auto& a : raw_args)
    {
        if (waiting_metric_config)
        {
            waiting_metric_config = false;
            load_config_metrics(a, pmu_cfg);
            continue;
        }

        if (a == L"-C")
        {
            waiting_metric_config = true;
            continue;
        }

        if (waiting_events_config)
        {
            waiting_events_config = false;
            load_config_events(a, extra_events);
            continue;
        }

        if (a == L"-E")
        {
            waiting_events_config = true;
            continue;
        }
    }

    if (builtin_metrics.size())
    {
        for (const auto& [key, value] : builtin_metrics)
        {
            if (metrics.find(key) == metrics.end())
                metrics[key] = value;
        }
    }

    for (const auto& a : raw_args)
    {
        if (waiting_commandline)
        {
            if (sample_pe_file.empty())
            {
                sample_pe_file = a;
                record_commandline = a;
            }
            else
                record_commandline += L" " + a;
            continue;
        }

        if (waiting_metric_config)
        {
            waiting_metric_config = false;
            continue;
        }

        if (waiting_events_config)
        {
            waiting_events_config = false;
            continue;
        }

        if (waiting_events)
        {
            if (do_sample || do_record)
            {
                m_sampling_flags.clear();   // Prepare to collect SPE filters
                if (parse_events_str_for_feat_spe(a, m_sampling_flags))   // Check if we are sampling with SPE
                {
                    if (pmu_cfg.has_spe == false)
                    {
                        m_out.GetErrorOutputStream() << L"SPE is not supported by your hardware: " << a << std::endl;
                        throw fatal_exception("ERROR_SPE_NOT_SUPP");
                    }

                    m_sampling_with_spe = true;

                    for (const auto& [key, value] : m_sampling_flags)
                    {
                        if (spe_device::is_filter_name(key) == false)
                        {
                            m_out.GetErrorOutputStream() << L"SPE filter '" << key <<  L"' unknown, use: "
                                << WStringJoin(spe_device::m_filter_names, L", ")
                                << std::endl;
                            throw fatal_exception("ERROR_SPE_FILTER_ERR");
                        }
                    }

                    /* When the user requests SPE we always include hidden events for the PMU. This 
                    *  events help diagnose the run. We call `parse_events_str` directly so all data 
                    *  structures required by the driver will be properly setup. 
                    */
                    parse_events_str(L"SAMPLE_POP,SAMPLE_FEED,SAMPLE_FILTRATE,SAMPLE_COLLISION", events, groups, L"", pmu_cfg);
                }
                else    // Software sampling (no SPE)
                {
                    parse_events_str_for_sample(a, ioctl_events_sample, sampling_inverval);
                    // After events are parsed check if we can fulfil this request as sampling does not have multiplexing
                    if (ioctl_events_sample.size() > pmu_cfg.total_gpc_num)
                    {
                        m_out.GetErrorOutputStream() << L"number of events requested exceeds the number of hardware PMU counters("
                            << pmu_cfg.total_gpc_num << ")" << std::endl;
                        throw fatal_exception("ERROR_EVENTS_SIZE");
                    }

                    if (ioctl_events_sample.size() > pmu_cfg.gpc_nums[EVT_CORE])
                    {
                        m_out.GetErrorOutputStream() << L"number of events requested exceeds the number of free hardware PMU counters("
                            << pmu_cfg.gpc_nums[EVT_CORE] << ") out of a total of (" << pmu_cfg.total_gpc_num << ")" << std::endl;
                        throw fatal_exception("ERROR_EVENTS_SIZE");
                    }
                }
            }
            else
                parse_events_str(a, events, groups, L"", pmu_cfg);
            waiting_events = false;
            continue;
        }

        if (waiting_cwd)
        {
            waiting_cwd = false;
            m_cwd = a;
            continue;
        }

        if (waiting_output_filename)
        {
            waiting_output_filename = false;
            output_filename = a;
            continue;
        }

        if (waiting_output_csv_filename)
        {
            waiting_output_csv_filename = false;
            output_csv_filename = a;
            continue;
        }

        if (waiting_config)
        {
            waiting_config = false;
            if (drvconfig::set(a) == false)
                m_out.GetErrorOutputStream() << L"error: can't set '" << a << "' config" << std::endl;
            continue;
        }

        if (waiting_metrics)
        {
            std::vector<std::wstring> list_of_defined_metrics;
            std::wistringstream metric_stream(a);
            std::wstring metric_token;

            while (std::getline(metric_stream, metric_token, L','))
            {
                if (groups_of_metrics.count(metric_token) > 0)
                {
                    // Expand group of metrics with its metrics
                    const std::vector<std::wstring>& m = groups_of_metrics.at(metric_token);
                    std::copy(m.begin(), m.end(), std::back_inserter(list_of_defined_metrics));
                }
                else
                {
                    // Add metric to the list
                    list_of_defined_metrics.push_back(metric_token);
                }
            }

            for (const auto &metric : list_of_defined_metrics)
            {
                if (metrics.find(metric) == metrics.end() &&
                    groups_of_metrics.count(metric) == 0)
                {
                    m_out.GetErrorOutputStream() << L"metric '" << metric << "' not supported" << std::endl;
                    if (metrics.size())
                    {
                        m_out.GetErrorOutputStream() << L"supported metrics:" << std::endl;
                        for (const auto& [key, value] : metrics)
                            m_out.GetErrorOutputStream() << L"  " << key << std::endl;
                    }
                    else
                    {
                        m_out.GetErrorOutputStream() << L"no metric registered" << std::endl;
                    }

                    throw fatal_exception("ERROR_METRIC");
                }

                metric_desc desc = metrics[metric];
                for (const auto& x : desc.events)
                    events[x.first].insert(events[x.first].end(), x.second.begin(), x.second.end());
                for (const auto& y : desc.groups)
                    groups[y.first].insert(groups[y.first].end(), y.second.begin(), y.second.end());

                if (metric == L"l3_cache")
                    report_l3_cache_metric = true;
                else if (metric == L"ddr_bw")
                    report_ddr_bw_metric = true;
            }

            waiting_metrics = false;
            continue;
        }

        if (waiting_image_name)
        {
            sample_image_name = a;
            waiting_image_name = false;
            continue;
        }

        if (waiting_pe_file)
        {
            if (std::filesystem::exists(a) == false)
            {
                m_out.GetErrorOutputStream() << L"PE file '" << a << L"' doesn't exist"
                    << std::endl;
                throw fatal_exception("ERROR_PE_FILE_PATH");
            }

            sample_pe_file = a;
            sample_pe_file_given = true;
            waiting_pe_file = false;
            continue;
        }

        if (waiting_pdb_file)
        {
            if (std::filesystem::exists(a) == false)
            {
                m_out.GetErrorOutputStream() << L"PDB file '" << a << L"' doesn't exist"
                    << std::endl;
                throw fatal_exception("ERROR_PDB_FILE_PATH");
            }

            sample_pdb_file = a;
            waiting_pdb_file = false;
            continue;
        }

        if (waiting_core_idx)
        {
            if (TokenizeWideStringOfInts(a.c_str(), L',', cores_idx) == false)
            {
                m_out.GetErrorOutputStream() << L"option -c format not supported, use comma separated list of integers, or range of integers"
                    << std::endl;
                throw fatal_exception("ERROR_CORES");
            }

            if (cores_idx.size() > MAX_PMU_CTL_CORES_COUNT)
            {
                m_out.GetErrorOutputStream() << L"you can specify up to " << int(MAX_PMU_CTL_CORES_COUNT)
                    << L"cores with -c <cpu_list> option"
                    << std::endl;
                throw fatal_exception("ERROR_CORES");
            }

            waiting_core_idx = false;
            continue;
        }

        if (waiting_timeline_count)
        {
            count_timeline = _wtoi(a.c_str());
            waiting_timeline_count = false;
            continue;
        }

        if (waiting_record_spawn_delay)
        {
            uint32_t val = _wtoi(a.c_str());
            assert(val <= UINT32_MAX);
            record_spawn_delay = val;
            waiting_record_spawn_delay = false;
            continue;
        }

        if (waiting_dmc_idx)
        {
            int val = _wtoi(a.c_str());
            assert(val <= UCHAR_MAX);
            dmc_idx = (uint8_t)val;
            waiting_dmc_idx = false;
            continue;
        }

        if (waiting_duration)
        {
            count_duration = convert_timeout_arg_to_seconds(a, waiting_duration_arg);
            waiting_duration = false;
            continue;
        }

        if (waiting_interval)
        {
            count_interval = convert_timeout_arg_to_seconds(a, waiting_duration_arg);
            waiting_interval = false;
            continue;
        }

        if (waiting_sample_display_row)
        {
            sample_display_row = _wtoi(a.c_str());
            waiting_sample_display_row = false;
            continue;
        }

        if (waiting_man_query)
        {
            man_query_args = a;
            waiting_man_query = false;
            continue;
        }

        if (waiting_symbol)
        {
            symbol_arg = a;
            waiting_symbol = false;
            continue;
        }

        // For compatibility with Linux perf
        if (a == L"list" || a == L"-l")
        {
            do_list = true;
            continue;
        }

        if (a == L"-e")
        {
            waiting_events = true;
            continue;
        }

        if (a == L"-C") // Skip this one as it was handled before any other args
        {
            waiting_metric_config = true;
            continue;
        }

        if (a == L"-E") // Skip this one as it was handled before any other args
        {
            waiting_events_config = true;
            continue;
        }

        if (a == L"-m")
        {
            waiting_metrics = true;
            continue;
        }

        if (a == L"stat")
        {
            do_count = true;
            continue;
        }

        if (a == L"record")
        {
            do_record = true;
            continue;
        }

        if (a == L"sample")
        {
            do_sample = true;
            continue;
        }

        if (a == L"detect")
        {
            do_detect = true;
            continue;
        }
        if (a == L"man")
        {
            do_man = true;
            waiting_man_query = true;
            continue;
        }

        if (a == L"--image_name")
        {
            waiting_image_name = true;
            continue;
        }

        if (a == L"--disassemble")
        {
            do_disassembly = true;
            do_annotate = true;
            continue;
        }

        if (a == L"--force-lock")
        {
            do_force_lock = true;
            continue;
        }

        if (a == L"--export_perf_data")
        {
            do_export_perf_data = true;
            continue;
        }

        if (a == L"--record_spawn_delay")
        {
            waiting_record_spawn_delay = true;
            continue;
        }

        if (a == L"--pe_file")
        {
            waiting_pe_file = true;
            continue;
        }

        if (a == L"--pdb_file")
        {
            waiting_pdb_file = true;
            continue;
        }

        if (a == L"--timeout" || a == L"sleep")
        {
            waiting_duration = true;
            waiting_duration_arg = a;
            continue;
        }

        if (a == L"--cwd" || a == L"--output-prefix")
        {
            waiting_cwd = true;
            continue;
        }

        if (a == L"--output" || a == L"-o")
        {
            waiting_output_filename = true;
            continue;
        }

        if (a == L"--output-csv")
        {
            waiting_output_csv_filename = true;
            continue;
        }

        if (a == L"--config")
        {
            waiting_config = true;
            continue;
        }

        if (a == L"-k")
        {
            do_kernel = true;
            continue;
        }

        if (a == L"-t")
        {
            do_timeline = true;
            if (count_interval == -1.0)
                count_interval = 60;
            if (count_duration == -1.0)
                count_duration = 1;
            continue;
        }

        if (a == L"-n")
        {
            waiting_timeline_count = true;
            continue;
        }

        if (a == L"--sample-display-row")
        {
            waiting_sample_display_row = true;
            continue;
        }

        if (a == L"--sample-display-long")
        {
            sample_display_short = false;
            continue;
        }

        if (a == L"-i")
        {
            waiting_duration_arg = a;
            waiting_interval = true;
            continue;
        }

        if (a == L"-c" || a == L"--cpu")
        {
            waiting_core_idx = true;
            continue;
        }

        if (a == L"--dmc")
        {
            waiting_dmc_idx = true;
            continue;
        }

        if (a == L"-v" || a == L"--verbose")
        {
            do_verbose = true;
            continue;
        }

        if (a == L"--annotate")
        {
            do_annotate = true;
            continue;
        }

        if (a == L"--version")
        {
            do_version = true;
            continue;
        }

        if (a == L"-h" || a == L"--help")
        {
            do_help = true;
            continue;
        }

        if (a == L"-q")
        {
            m_out.m_isQuiet = true;
            continue;
        }

        if (a == L"--json")
        {
            if (m_outputType != TableType::ALL)
            {
                m_outputType = TableType::JSON;
                m_out.m_isQuiet = true;
            }
            continue;
        }

        if (a == L"test")
        {
            do_test = true;
            continue;
        }

        if (a == L"-s" || a == L"--symbol")
        {
            do_symbol = true;
            waiting_symbol = true;
            continue;
        }

        /* This will finish command line argument parsing. After "--" we will see process to spawn in form of:
        *
        *  wperf ... -- PROCESS_NAME ARG ARG ARG ARG ...
        *
        *  From now on we will parse process spawn name and verbatim arguments
        */
        if (a == L"--")
        {
            if (!(do_record || do_count))
                m_out.GetErrorOutputStream() << L"warning: only `stat` and `record` support process spawn!" << std::endl;

            waiting_commandline = true;
            /* We will reload here PE file name to spawn. `waiting_commandline` will expect this
            *  to be empty to detect beggining of the scan.
            */
            sample_pe_file.clear();
            record_commandline.clear();
            continue;
        }

        m_out.GetErrorOutputStream() << L"warning: unexpected arg '" << a << L"' ignored" << std::endl;
    }

    std::wstring output_filename_full_path = output_filename;
    std::wstring output_filename_csv_full_path = output_csv_filename;

    if (m_cwd.size())
    {
        output_filename_full_path = GetFullFilePath(m_cwd, output_filename);
        output_filename_csv_full_path = GetFullFilePath(m_cwd, output_csv_filename);
    }

    // Support custom outpus for --output
    if (output_filename.size())
    {
        if (do_timeline)
        {
            if (m_outputType == TableType::JSON)    //  -t ... --json
            {
                m_outputType = TableType::ALL;
                m_out.m_filename = output_filename_full_path;           // -t ... --json --output filename.json
                m_out.m_shouldWriteToFile = true;
                timeline_output_file = output_filename_csv_full_path;   // -t ... --json --output filename.json --output-csv filename.csv
            }
            else //  -t ... --output filename.csv / --output-csv filename.csv
            {
                if (output_csv_filename.size())
                    timeline_output_file = output_filename_csv_full_path;   // -t ... --output filename.csv --output-csv filename.csv, --output-csv has higher priority
                else
                    timeline_output_file = output_filename_full_path;       // -t ... --output filename.csv
            }
        }
        else // Output to JSON
        {
            m_outputType = TableType::ALL;
            m_out.m_filename = output_filename_full_path;
            m_out.m_shouldWriteToFile = true;
        }
    }

    // --output-csv has higher priority
    if (output_csv_filename.size() && do_timeline)
        timeline_output_file = output_filename_csv_full_path;   // -t ... --output-csv filename.csv

    if (do_sample && cores_idx.size() > 1)
    {
        m_out.GetErrorOutputStream() << L"sampling: you can specify 1 core with -c option"
            << std::endl;
        throw fatal_exception("ERROR_CORES");
    }
}

bool user_request::has_events()
{
    return !!ioctl_events.size();
}

void user_request::show_events()
{
    m_out.GetOutputStream() << L"events to be counted:" << std::endl;

    for (const auto& a : ioctl_events)
    {
        m_out.GetOutputStream() << L"  " << std::setw(4) << a.second.size() << std::setw(18) << pmu_events::get_evt_class_name(a.first) << L" events:";
        for (const auto& b : a.second)
            m_out.GetOutputStream() << L" " << IntToHexWideString(b.index);
        m_out.GetOutputStream() << std::endl;
    }
}

void user_request::check_events(enum evt_class evt, int max)
{
    if (ioctl_events.count(evt) &&
        ioctl_events[evt].size() > MAX_MANAGED_CORE_EVENTS)
    {
        m_out.GetErrorOutputStream() << L"error: too many " << pmu_events::get_evt_class_name(evt) << L" (including padding) events: "
            << ioctl_events[evt].size() << L" > " << max << std::endl;
        throw fatal_exception("ERROR_MAX_EVENTS");
    }
}

// Please note that 'config_name' here is a file name or just command line with extra events
void user_request::load_config_events(std::wstring config_name,
    std::map<enum evt_class, std::vector<struct extra_event>>& extra_events)
{
    // If config_name is a openable file: parse file content
    if (std::filesystem::exists(config_name))
    {
        std::wifstream  config_file(config_name);

        if (!config_file.is_open())
        {
            m_out.GetErrorOutputStream() << L"open config file '" << config_name << "' failed" << std::endl;
            throw fatal_exception("ERROR_CONFIG_FILE");
        }

        for (std::wstring line; std::getline(config_file, line); )
        {
            parse_events_extra(line, extra_events);
        }
    }
    else
        parse_events_extra(config_name, extra_events);  // config_name is "e:v,e:v,e:v" string
}

void user_request::load_config_metrics(std::wstring config_name, const struct pmu_device_cfg& pmu_cfg)
{
    std::wifstream config_file(config_name);

    if (!config_file.is_open())
    {
        m_out.GetErrorOutputStream() << L"open config file '" << config_name << "' failed" << std::endl;
        throw fatal_exception("ERROR_CONFIG_FILE");
    }

    std::wstring line;
    while (std::getline(config_file, line))
    {
        std::wstring trimed_line = trim(line);
        if (trimed_line.front() == L'#')
            continue;

        std::wistringstream config_stream(trimed_line);
        std::wstring component;
        int i = 0;
        std::wstring key;
        while (std::getline(config_stream, component, L':'))
        {
            if (i == 0)
            {
                key = trim(component);
            }
            else if (i == 1)
            {
                metric_desc mdesc;
                mdesc.raw_str = trim(component);
                try
                {
                    parse_events_str(mdesc.raw_str, mdesc.events, mdesc.groups, key, pmu_cfg);
                    metrics[key] = mdesc;
                }
                catch (const fatal_exception&)
                {
                    m_out.GetErrorOutputStream() << L"warning: Metric " << key << " is unable to be used due to lack of hardware resources." << std::endl;
                }
            }
            else
            {
                m_out.GetErrorOutputStream() << L"unrecognized config component '"
                    << component << L"'" << std::endl;
                throw fatal_exception("ERROR_CONFIG_COMPONENT");
            }
            i++;
        }
    }

    config_file.close();
}

std::wstring user_request::trim(const std::wstring& str,
    const std::wstring& whitespace)
{
    const auto pos_begin = str.find_first_not_of(whitespace);
    if (pos_begin == std::wstring::npos)
        return L"";

    const auto pos_end = str.find_last_not_of(whitespace);
    const auto len = pos_end - pos_begin + 1;

    return str.substr(pos_begin, len);
}


bool user_request::check_timeout_arg(std::wstring number_and_suffix, const std::unordered_map<std::wstring, double>& unit_map)
{
    std::wstring accept_units;

    for (const auto& pair : unit_map)
    {
        if (!accept_units.empty()) {
            accept_units += L"|";
        }
        accept_units += pair.first;
    }

    std::wstring regex_string = L"^(0|([1-9][0-9]*))(\\.[0-9]{1,2})?(" + accept_units + L")?$";
    std::wregex r(regex_string);

    std::wsmatch match;
    if (std::regex_search(number_and_suffix, match, r)) {
        return true;
    }
    else {
        return false;
    }

}

double user_request::convert_timeout_arg_to_seconds(std::wstring number_and_suffix, const std::wstring& cmd_arg)
{
    std::unordered_map<std::wstring, double> unit_map = { {L"s", 1}, { L"m", 60 }, {L"ms", 0.001}, {L"h", 3600}, {L"d" , 86400} };

    bool valid_input = check_timeout_arg(number_and_suffix, unit_map);

    if (!valid_input) {
        m_out.GetErrorOutputStream() << L"input: \"" << number_and_suffix << L"\" to argument '" << cmd_arg <<  "' is invalid" << std::endl;
        throw fatal_exception("ERROR_TIMEOUT_COMPONENT");
    }
    //logic to split number and suffix
    int i = 0;
    for (; i < number_and_suffix.size(); i++)
    {
        if (!std::isdigit(number_and_suffix[i]) && (number_and_suffix[i] != L'.')) {
            break;
        }
    }

    std::wstring number_wstring = number_and_suffix.substr(0, i);

    double number;
    try {
        number = std::stod(number_wstring);
    }
    catch (...) {
        m_out.GetErrorOutputStream() << L"input: \"" << number_and_suffix << L"\" to argument '" << cmd_arg << "' is invalid" << std::endl;
        throw fatal_exception("ERROR_TIMEOUT_COMPONENT");
    }

    std::wstring suffix = number_and_suffix.substr(i);

    //default to seconds if unit is not provided
    if (suffix.empty()) {
        return number;
    }

    //check if the unit exists in the map
    auto it = unit_map.find(suffix);
    if (it == unit_map.end())
    {
        m_out.GetErrorOutputStream() << L"input unit \"" << suffix << L"\" not recognised as unit in argument '" << cmd_arg << "'" << std::endl;
        throw fatal_exception("ERROR_TIMEOUT_COMPONENT");
    }
    //Note: This exception should never be reached, as it should be caught in the regex construction of check_timeout_arg
    //However, if the unit map/regex construction is changeed in the future, this serves as a good safety net

    return ConvertNumberWithUnit(number, suffix, unit_map);
}

bool user_request::check_symbol_arg(const std::wstring& symbol, const std::wstring& arg,
    const wchar_t prefix_delim, const wchar_t suffix_delim)
{
    std::wstring lower_symbol = WStringToLower(symbol);
    std::wstring lower_arg = WStringToLower(arg);

    if (WStringStartsWith(arg, std::wstring(1, prefix_delim)) && WStringEndsWith(arg, std::wstring(1, suffix_delim)))
    {
        // both delimiters are present, treat as if neither are there
        return (lower_symbol == WStringToLower(arg.substr(1, arg.size() - 2)));
    }
    else if (WStringStartsWith(arg, std::wstring(1, prefix_delim)))
    {
        // symbol exists at beginning
        return CaseInsensitiveWStringStartsWith(symbol, arg.substr(1));
    }
    else if (WStringEndsWith(arg, std::wstring(1, suffix_delim)))
    {
        // symbol exists at end
        return CaseInsensitiveWStringEndsWith(symbol, arg.substr(0, arg.size() - 1));
    }
    else
    {
        // symbol matches - case insensitive
        return (lower_symbol == lower_arg);
    }
}

