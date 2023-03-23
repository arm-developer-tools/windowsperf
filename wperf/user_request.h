#pragma once
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

    void init(wstr_vec& raw_args, const struct pmu_device_cfg& pmu_cfg, std::map<std::wstring, metric_desc>& builtin_metrics);
    void parse_raw_args(wstr_vec& raw_args, const struct pmu_device_cfg& pmu_cfg,
        std::map<enum evt_class, std::deque<struct evt_noted>>& events,
        std::map<enum evt_class, std::vector<struct evt_noted>>& groups,
        std::map<std::wstring, metric_desc>& builtin_metrics);
    void set_event_padding(const struct pmu_device_cfg& pmu_cfg,
        std::map<enum evt_class, std::deque<struct evt_noted>>& events,
        std::map<enum evt_class, std::vector<struct evt_noted>>& groups);

    bool has_events();
    void show_events();
    void push_ioctl_normal_event(enum evt_class e_class, struct evt_noted event);
    void push_ioctl_padding_event(enum evt_class e_class, uint16_t event);
    void push_ioctl_grouped_event(enum evt_class e_class, struct evt_noted event, uint16_t group_num);
    void load_config(std::wstring config_name, const struct pmu_device_cfg& pmu_cfg);

    static void print_help();

    bool do_list;
    bool do_count;
    bool do_kernel;
    bool do_timeline;
    bool do_sample;
    bool do_version;
    bool do_verbose;
    bool do_help;
    bool do_test;
    bool report_l3_cache_metric;
    bool report_ddr_bw_metric;
    std::vector<uint8_t> cores_idx;
    uint8_t dmc_idx;
    double count_duration;
    double count_interval;
    std::wstring sample_image_name;
    std::wstring sample_pe_file;
    std::wstring sample_pdb_file;
    uint32_t sample_display_row;
    bool sample_display_short;
    std::map<enum evt_class, std::vector<struct evt_noted>> ioctl_events;
    std::vector<struct evt_sample_src> ioctl_events_sample;
    std::map<std::wstring, metric_desc> metrics;

private:
    bool all_cores_p() {
        return cores_idx.size() > 1;
    }

    std::wstring trim(const std::wstring& str, const std::wstring& whitespace = L" \t");
    void padding_ioctl_events(enum evt_class e_class, uint8_t gpc_num, std::deque<struct evt_noted>& padding_vector);
    void padding_ioctl_events(enum evt_class e_class, uint8_t gpc_num);
};