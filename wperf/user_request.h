#pragma once
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

#include <numeric>
#include "parsers.h"
#include "pmu_device.h"
#include "utils.h"
#include "events.h"
#include "output.h"

typedef std::vector<std::wstring> wstr_vec;

class user_request
{
public:
    user_request();

    void init(wstr_vec& raw_args, const struct pmu_device_cfg& pmu_cfg,
        std::map<std::wstring, metric_desc>& builtin_metrics,
        const std::map <std::wstring, std::vector<std::wstring>>& groups_of_metrics,
        std::map<enum evt_class, std::vector<struct extra_event>>& extra_events);

    void parse_raw_args(wstr_vec& raw_args, const struct pmu_device_cfg& pmu_cfg,
        std::map<enum evt_class, std::deque<struct evt_noted>>& events,
        std::map<enum evt_class, std::vector<struct evt_noted>>& groups,
        std::map<std::wstring, metric_desc>& builtin_metrics,
        const std::map <std::wstring, std::vector<std::wstring>>& groups_of_metrics,
        std::map<enum evt_class, std::vector<struct extra_event>>& extra_events);

    bool has_events();
    void show_events();
    void check_events(enum evt_class evt, int max);
    void load_config_events(std::wstring config_name,
        std::map<enum evt_class, std::vector<struct extra_event>>& extra_events);
    void load_config_metrics(std::wstring config_name, const struct pmu_device_cfg& pmu_cfg);

    static void print_help();
    static void print_help_header();
    static void print_help_prompt();
    static void print_help_usage();
    static bool is_cli_option_in_args(const wstr_vec& raw_args, std::wstring opt);    // Return true if `opt` is in CLI options
    static bool is_force_lock(const wstr_vec& raw_args);    // Return true if `--force-lock` is in CLI options
    static bool is_help(const wstr_vec& raw_args);          // Return true if `--help` is in CLI options
    static bool check_timeout_arg(std::wstring number_and_suffix, const std::unordered_map<std::wstring, double>& unit_map);
    static double convert_timeout_arg_to_seconds(std::wstring number_and_suffix, const std::wstring& cmd_arg);
    static bool check_symbol_arg(const std::wstring& symbol, const std::wstring& arg,
        const wchar_t prefix_delim = PARSER_SYMBOL_PREFIX, const wchar_t suffix_delim = PARSER_SYMBOL_SUFFIX);

    bool do_list;
    bool do_count;
    bool do_kernel;
    bool do_timeline;
    bool do_sample;
    bool do_record;
    bool do_version;
    bool do_verbose;
    bool do_help;
    bool do_test;
    bool do_annotate;
    bool do_disassembly;
    bool do_man;
    bool do_symbol;
    bool do_detect = false;
    bool do_force_lock = false;     // Force lock acquire of the driver
    bool do_export_perf_data;
    bool do_cwd = false;            // Set current working dir for storing output files
    bool report_l3_cache_metric;
    bool report_ddr_bw_metric;
    std::vector<uint8_t> cores_idx;
    uint8_t dmc_idx;
    double count_duration;
    double count_interval;
    int count_timeline;
    uint32_t record_spawn_delay = 1000;
    std::wstring man_query_args;
    std::wstring symbol_arg;
    std::wstring sample_image_name;
    std::wstring sample_pe_file;
    std::wstring sample_pdb_file;
    std::wstring record_commandline;        // <sample_pe_file> <arg> <arg> <arg> ...
    std::wstring timeline_output_file; 
    std::wstring m_cwd;                     // Current working dir for storing output files
    uint32_t sample_display_row;
    bool sample_display_short;
    std::map<enum evt_class, std::vector<struct evt_noted>> ioctl_events;
    std::vector<struct evt_sample_src> ioctl_events_sample;
    std::map<std::wstring, metric_desc> metrics;
    std::map<uint32_t, uint32_t> sampling_inverval;     //!< [event_index] -> event_sampling_interval
    bool m_sampling_with_spe = false;                   // SPE: User requested sampling with SPE
    std::map<std::wstring, bool> m_sampling_flags;      // SPE: sampling flags

private:
    bool all_cores_p() const {
        return cores_idx.size() > 1;
    }

    static const wchar_t PARSER_SYMBOL_PREFIX = L'^';
    static const wchar_t PARSER_SYMBOL_SUFFIX = L'$';

    std::wstring trim(const std::wstring& str, const std::wstring& whitespace = L" \t");
};
