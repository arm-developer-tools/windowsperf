// BSD 3-Clause License
//
// Copyright (c) 2022, Arm Limited
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
#include <assert.h>
#include "user_request.h"
#include "exception.h"
#include "padding.h"
#include "wperf-common/public.h"


void user_request::print_help_usage()
{
    const wchar_t* wsHelp = LR"(
usage: wperf [options]

    Options:
    list                  List supported events and metrics.
    stat                  Count events.If - e is not specified, then count default events.
    test                  Configuration information about driver and application confituration.
    sample                Sample events. If -e is not specified, cycle counter will be the default sample source
    -e e1, e2...          Specify events to count.Event eN could be a symbolic name or in raw number.
                          Symbolic name should be what's listed by 'perf list', raw number should be rXXXX,
                          XXXX is hex value of the number without '0x' prefix.
                          when doing sampling, support -e e1:sample_freq1,e2:sample_freq2...
    -m m1, m2...          Specify metrics to count. 'imix', 'icache', 'dcache', 'itlb', 'dtlb' supported.
    --timeout SEC         Specify counting duration(in s).The accuracy is 0.1s.
    sleep N               Like -d, for compatibility with Linux perf.
    -i N                  Specify counting interval(in s).To be used with -t.
    -t                    Enable timeline mode.It specifies -i 60 -d 1 implicitly.
                          Means counting 1 second after every 60 second, and the result
                          is in.csv file in the same folder where wperf is invoked.
                          You can use -i and -d to change counting duration and interval.
    -n N                  How many times count in timeline mode (disabled by default).
    -image_name           Specify the image name you want to sample.
    -pe_file              Specify the PE file.
    -pdb_file             Specify the PDB file.
    -sample-display-long  Display decorated symbol names.
    -sample-display-row   Set how many samples you want to see in the summary (50 by default).
    -C config_file        Provide customized config file which describes metrics etc.
    -E config_file        Provide customized config file which describes custom events.
    -E event_list         Provide custom events from command line, e.g. '-E name1:0x1234,name2:0xABCD'
    -c core_idx           Profile on the specified core. Skip -c to count on all cores.
                          In sampling user must specify exactly one core with -c.
    -c cpu_list           Profile on the specified cores, 'cpu_list' is comma separated list e.g. '-c 0,1,2,3'.
    -dmc dmc_idx          Profile on the specified DDR controller. Skip -dmc to count on all DMCs.
    -k                    Count kernel mode as well (disabled by default).
    -h / --help           Show tool help.
    --output              Enable JSON output to file.
    -q                    Quiet mode, no output is produced.
    -json                 Define output type as JSON.
    -l                    Alias of 'list'.
    -verbose              Enable verbose output.
    -v                    Alias of '-verbose'.
    -version              Show tool version.
)";

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


user_request::user_request() : do_list{ false }, do_count(false), do_kernel(false), do_timeline(false),
    do_sample(false), do_annotate(false), do_version(false), do_verbose(false), do_test(false),
    do_help(false), dmc_idx(_UI8_MAX), count_duration(-1.0),
    sample_image_name(L""), sample_pe_file(L""), sample_pdb_file(L""),
    sample_display_row(50), sample_display_short(true), count_timeline(0),
    count_interval(-1.0), report_l3_cache_metric(false), report_ddr_bw_metric(false) {}

void user_request::init(wstr_vec& raw_args, const struct pmu_device_cfg& pmu_cfg,
    std::map<std::wstring, metric_desc>& builtin_metrics,
    std::map<enum evt_class, std::vector<struct extra_event>>& extra_events)
{
    std::map<enum evt_class, std::deque<struct evt_noted>> events;
    std::map<enum evt_class, std::vector<struct evt_noted>> groups;

    // Fill cores_idx with {0, ... core_num}
    cores_idx.clear();
    cores_idx.resize(pmu_cfg.core_num);
    std::iota(cores_idx.begin(), cores_idx.end(), (UINT8)0);

    parse_raw_args(raw_args, pmu_cfg, events, groups, builtin_metrics, extra_events);

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
    }
    else if (do_sample)
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
    bool waiting_filename = false;
    bool waiting_image_name = false;
    bool waiting_pe_file = false;
    bool waiting_pdb_file = false;
    bool waiting_sample_display_row = false;
    bool waiting_timeline_count = false;

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
            if (do_sample)
                parse_events_str_for_sample(a, ioctl_events_sample, sampling_inverval);
            else
                parse_events_str(a, events, groups, L"", pmu_cfg);
            waiting_events = false;
            continue;
        }

        if (waiting_filename)
        {
            waiting_filename = false;
            m_out.m_filename = a;
            m_out.m_shouldWriteToFile = true;
            continue;
        }

        if (waiting_metrics)
        {
            std::wistringstream metric_stream(a);
            std::wstring metric;

            while (std::getline(metric_stream, metric, L','))
            {
                if (metrics.find(metric) == metrics.end())
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
            sample_pe_file = a;
            waiting_pe_file = false;
            continue;
        }

        if (waiting_pdb_file)
        {
            sample_pdb_file = a;
            waiting_pdb_file = false;
            continue;
        }

        if (waiting_core_idx)
        {
            if (TokenizeWideStringOfInts(a.c_str(), L',', cores_idx) == false)
            {
                m_out.GetErrorOutputStream() << L"option -c format not supported, use comma separated list of integers"
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
            count_duration = _wtof(a.c_str());
            waiting_duration = false;
            continue;
        }

        if (waiting_interval)
        {
            count_interval = _wtof(a.c_str());
            waiting_interval = false;
            continue;
        }

        if (waiting_sample_display_row)
        {
            sample_display_row = _wtoi(a.c_str());
            waiting_sample_display_row = false;
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

        if (a == L"sample")
        {
            do_sample = true;
            continue;
        }

        if (a == L"-image_name")
        {
            waiting_image_name = true;
            continue;
        }

        if (a == L"-pe_file")
        {
            waiting_pe_file = true;
            continue;
        }

        if (a == L"-pdb_file")
        {
            waiting_pdb_file = true;
            continue;
        }

        if (a == L"--timeout" || a == L"sleep")
        {
            waiting_duration = true;
            continue;
        }

        if (a == L"--output")
        {
            waiting_filename = true;
            m_outputType = TableType::ALL;
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

        if (a == L"-sample-display-row")
        {
            waiting_sample_display_row = true;
            continue;
        }

        if (a == L"-sample-display-long")
        {
            sample_display_short = false;
            continue;
        }

        if (a == L"-i")
        {
            waiting_interval = true;
            continue;
        }

        if (a == L"-c")
        {
            waiting_core_idx = true;
            continue;
        }

        if (a == L"-dmc")
        {
            waiting_dmc_idx = true;
            continue;
        }

        if (a == L"-v" || a == L"-verbose")
        {
            do_verbose = true;
            continue;
        }

        if (a == L"-annotate")
        {
            do_annotate = true;
            continue;
        }

        if (a == L"-version")
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

        if (a == L"-json")
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

        m_out.GetOutputStream() << L"warning: unexpected arg '" << a << L"' ignored\n";
    }

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
                parse_events_str(mdesc.raw_str, mdesc.events, mdesc.groups, key, pmu_cfg);
                metrics[key] = mdesc;
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
