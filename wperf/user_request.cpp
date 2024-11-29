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

//
// This file will be modified with wperf's pre-build step
//
#include "wperf-common\gitver.h"


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

void user_request::init(ArgParser::arg_parser& parsed_args, const struct pmu_device_cfg& pmu_cfg,
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

    parse_raw_args(parsed_args, pmu_cfg, events, groups, builtin_metrics, groups_of_metrics, extra_events);

    // Deduce image name and PDB file name from PE file name
    if (sample_pe_file.size())
    {
        /*
        * if `image_name` is not provided by user with `--image_name` we can
        * deduce image / base module name based on PE file name provided with
        * `--pe_file`. In most cases these are the same. In case image name is
        * different than PE name users should provide `image_name` to remove
        * ambiguity.
        */
        if (sample_image_name.empty())
        {
            sample_image_name = sample_pe_file;
            if (do_verbose)
                m_out.GetOutputStream() << L"deduced image name '" << sample_image_name << L"'" << std::endl;
        }

        /*
        * We can also deduce PE file's corresponding PDB file (with debug information)
        * based on PE file name (and path). Assumptions is that PDB files and PE files
        * are located in the same directory and have the same name (excluding extension).
        * E.g. `c:\path\to\image.exe` ->`c:\path\to\image.pdb`.
        */
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

void user_request::parse_raw_args(ArgParser::arg_parser& parsed_args, const struct pmu_device_cfg& pmu_cfg,
    std::map<enum evt_class, std::deque<struct evt_noted>>& events,
    std::map<enum evt_class, std::vector<struct evt_noted>>& groups,
    std::map<std::wstring, metric_desc>& builtin_metrics,
    const std::map <std::wstring, std::vector<std::wstring>>& groups_of_metrics,
    std::map<enum evt_class, std::vector<struct extra_event>>& extra_events)
{   


    bool sample_pe_file_given = false;

    std::wstring waiting_duration_arg;
    std::wstring output_filename, output_csv_filename;

    switch (parsed_args.m_command)
    {
        case ArgParser::COMMAND_CLASS::STAT:
			do_count = true;
            break;
        case ArgParser::COMMAND_CLASS::SAMPLE:
			do_sample = true;
            break;
        case ArgParser::COMMAND_CLASS::RECORD:
			do_record = true;
            break;
        case ArgParser::COMMAND_CLASS::TEST:
			do_test = true;
            break;
        case ArgParser::COMMAND_CLASS::DETECT:
            do_detect = true;
            break;
        case ArgParser::COMMAND_CLASS::HELP:
			do_help = true;
            break;
        case ArgParser::COMMAND_CLASS::VERSION:
			do_version = true;
            break;
        case ArgParser::COMMAND_CLASS::LIST:
			do_list = true;
            break;
        case ArgParser::COMMAND_CLASS::MAN:
			do_man = true;
            break;
        default:
            break;
    }

    if (parsed_args.metric_config_arg.is_set())
    {
        load_config_metrics(parsed_args.metric_config_arg.get_values().front(), pmu_cfg);
    }
    if (parsed_args.event_config_arg.is_set())
    {
        load_config_events(parsed_args.event_config_arg.get_values().front(), extra_events);
    }
    
    if (builtin_metrics.size())
    {
        for (const auto& [key, value] : builtin_metrics)
        {
            if (metrics.find(key) == metrics.end())
                metrics[key] = value;
        }
    }
    if (parsed_args.extra_args_arg.is_set())
    {
        if (sample_pe_file.empty())
            sample_pe_file = parsed_args.extra_args_arg.get_values().front();
        
        record_commandline += WStringJoin(parsed_args.extra_args_arg.get_values(), L" ");
        if (!(do_record || do_count))
            m_out.GetErrorOutputStream() << L"warning: only `stat` and `record` support process spawn!" << std::endl;

        sample_pe_file.clear();
        record_commandline.clear();
    }
    if (parsed_args.output_prefix_arg.is_set())
    {
        m_cwd = parsed_args.output_prefix_arg.get_values().front();
    }
    if (parsed_args.output_filename_arg.is_set())
    {
        output_filename = parsed_args.output_filename_arg.get_values().front();
    }

    if (parsed_args.output_csv_filename_arg.is_set())
    {
        output_csv_filename = parsed_args.output_csv_filename_arg.get_values().front();
    }

    if (parsed_args.config_arg.is_set())
    {
        wstring config_values = parsed_args.config_arg.get_values().front();
        if (drvconfig::set(config_values) == false)
            m_out.GetErrorOutputStream() << L"error: can't set '" << config_values << "' config" << std::endl;
    }
    if (parsed_args.image_name_arg.is_set())
    {
        sample_image_name = parsed_args.image_name_arg.get_values().front();
    }

    if (parsed_args.pe_file_arg.is_set())
    {
		wstring parsed_pe_file = parsed_args.pe_file_arg.get_values().front();
        if (std::filesystem::exists(parsed_pe_file) == false)
        {
            m_out.GetErrorOutputStream() << L"PE file '" << parsed_pe_file << L"' doesn't exist"
                << std::endl;
            throw fatal_exception("ERROR_PE_FILE_PATH");
        }
        sample_pe_file = parsed_pe_file;
        sample_pe_file_given = true;
    }
  
    if (parsed_args.pdb_file_arg.is_set())
    {
        wstring parsed_pdb_file = parsed_args.pdb_file_arg.get_values().front();
        if (std::filesystem::exists(parsed_pdb_file) == false)
        {
            m_out.GetErrorOutputStream() << L"PDB file '" << parsed_pdb_file << L"' doesn't exist"
                << std::endl;
            throw fatal_exception("ERROR_PDB_FILE_PATH");
        }
        sample_pdb_file = parsed_pdb_file;
    }


    if (parsed_args.cores_arg.is_set())
    {
        wstring core_idx_str = WStringJoin(parsed_args.cores_arg.get_values(), L",");

        if (TokenizeWideStringOfInts(core_idx_str.c_str(), L',', cores_idx) == false)
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
    }

    if (parsed_args.iteration_arg.is_set())
    {
        count_timeline = _wtoi(parsed_args.iteration_arg.get_values().front().c_str());
    }

    if (parsed_args.record_spawn_delay_arg.is_set())
    {
        uint32_t val = _wtoi(parsed_args.record_spawn_delay_arg.get_values().front().c_str());
        assert(val <= UINT32_MAX);
        record_spawn_delay = val;
    }

    if (parsed_args.dmc_arg.is_set())
    {

        int val = _wtoi(parsed_args.output_filename_arg.get_values().front().c_str());
        assert(val <= UCHAR_MAX);
        dmc_idx = (uint8_t)val;
    }

    if (parsed_args.timeout_arg.is_set())
    {
        count_duration = convert_timeout_arg_to_seconds(parsed_args.output_filename_arg.get_values().front(), parsed_args.timeout_arg.get_name());
    }
    
    if (parsed_args.interval_arg.is_set())
    {
        count_interval = convert_timeout_arg_to_seconds(parsed_args.interval_arg.get_values().front(), parsed_args.interval_arg.get_name());
    }

    if (parsed_args.sample_display_row_arg.is_set())
    {
        sample_display_row = _wtoi(parsed_args.sample_display_row_arg.get_values().front().c_str());
    }

    if (parsed_args.man_command.is_set())
    {
        man_query_args = parsed_args.man_command.get_values().front();
    }
    if (parsed_args.symbol_arg.is_set())
    {
        symbol_arg = parsed_args.symbol_arg.get_values().front();
        do_symbol = true;

    }
	do_disassembly = parsed_args.disassembly_opt.is_set();
	do_annotate = parsed_args.annotate_opt.is_set() || parsed_args.disassembly_opt.is_set();
	do_force_lock = parsed_args.force_lock_opt.is_set();
	do_kernel = parsed_args.kernel_opt.is_set();
	if (parsed_args.timeline_opt.is_set())
	{
		do_timeline = true;
        if (count_interval == -1.0)
            count_interval = 60;
        if (count_duration == -1.0)
            count_duration = 1;
	}
    sample_display_short = !parsed_args.sample_display_long_opt.is_set();
	do_verbose = parsed_args.verbose_opt.is_set();
	m_out.m_isQuiet = parsed_args.quite_opt.is_set();
    if (parsed_args.json_opt.is_set() && m_outputType != TableType::ALL)
    {
        m_outputType = TableType::JSON;
        m_out.m_isQuiet = true;
    }
	do_export_perf_data = parsed_args.export_perf_data_opt.is_set();


        if (parsed_args.events_arg.is_set())
        {
			wstring raw_events_string = WStringJoin(parsed_args.events_arg.get_values(), L",");
            if (do_sample || do_record)
            {
                m_sampling_flags.clear();   // Prepare to collect SPE filters
                if (parse_events_str_for_feat_spe(raw_events_string, m_sampling_flags))   // Check if we are sampling with SPE
                {
                    if (pmu_cfg.has_spe == false)
                    {
                        m_out.GetErrorOutputStream() << L"SPE is not supported by your hardware: " << raw_events_string << std::endl;
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

                        if (value > spe_device::max_filter_val(key))
                        {
                            m_out.GetErrorOutputStream() << L"SPE filter '" << key << L"' value out of range, use: 0-"
                                << spe_device::max_filter_val(key)
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
                    parse_events_str_for_sample(raw_events_string, ioctl_events_sample, sampling_inverval);
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
                parse_events_str(raw_events_string, events, groups, L"", pmu_cfg);
        }

        if (parsed_args.metrics_arg.is_set())
        {
            wstring raw_metrics_string = WStringJoin(parsed_args.metrics_arg.get_values(), L",");
            std::vector<std::wstring> list_of_defined_metrics;
            std::wistringstream metric_stream(raw_metrics_string);
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
        ioctl_events[evt].size() > max)
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

