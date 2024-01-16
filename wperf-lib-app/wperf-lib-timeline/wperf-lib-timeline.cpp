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

#include <cwchar>
#include <iostream>
#include <sstream>
#include <string>
#include "wperf-lib.h"

void usage(const wchar_t *name)
{
    printf("%S CORE N I PE_FILE PARAM\n"
           "\n"
           "<CORE>     - CPU number to count on (and spawn micro-benchmark process)\n"
           "<N>        - how many times count in timeline\n"
           "<I>        - interval between counts (in seconds)\n"
           "<PE_FILE>  - PE file to execute\n"
           "<PARAM>    - parameters for PE file\n",
           name);
}

struct Args
{
    uint8_t core;
    int N;
    double I;
    std::wstring metric;
    std::wstring pe_file;
    std::wstring record_commandline;
};

int parse_args(struct Args *args, int argc, const wchar_t *argv[])
{
    if (argc < 6)
        return -1;

    args->core = (uint8_t)wcstol(argv[1], nullptr, 0);
    args->N = wcstol(argv[2], nullptr, 0);
    args->I = wcstol(argv[3], nullptr, 0);
    args->metric = std::wstring(argv[4]);
    args->pe_file = std::wstring(argv[5]);

    std::wstringstream wss;
    for (int i = 5; i < argc; i++)
    {
        wss << argv[i] << " ";
    }
    args->record_commandline = wss.str();

    printf("core:               %d\n", args->core);
    printf("N:                  %d\n", args->N);
    printf("I:                  %f\n", args->I);
    printf("metric:             %S\n", args->metric.c_str());
    printf("PE file:            %S\n", args->pe_file.c_str());
    printf("record commandline: %S\n", args->record_commandline.c_str());
    return 0;
}

int wmain(int argc, const wchar_t *argv[])
{
    struct Args args = {0};
    if (parse_args(&args, argc, argv) != 0)
    {
        usage(argv[0]);
        return 1;
    }

    if (!wperf_init())
    {
        printf("failed to init wperf\n");
        return 1;
    }
    wperf_set_verbose(true);

    uint8_t cores[] = {args.core};
    const wchar_t *metric_events[] = {args.metric.c_str()};
    STAT_CONF stat_conf =
    {
        sizeof(cores)/sizeof(cores[0]), // num_cores
        cores, // cores
        0, // num_events
        NULL, // events
        {0/*num_groups*/, NULL/*num_group_events*/, NULL/*events*/}, // group_events
        sizeof(metric_events)/sizeof(metric_events[0]), // num_metrics
        metric_events, // metric_events
        1, // duration
        false, // kernel_mode
        100, // period
        true, // timeline
        args.N, // count_timeline
        args.I, // counting_interval
        args.pe_file.c_str(), // pe_file
        args.record_commandline.c_str(), // record_commandline
        1000, // record_spawn_delay
    };
    printf("checking wperf_stat with timeline...\n");
    if (wperf_stat(&stat_conf, NULL))
    {
        STAT_INFO stat_info;
        while (wperf_stat(&stat_conf, &stat_info))
        {
            printf("wperf_stat: core_idx=%u, event_idx=%u, counter_value=%llu, evt_note=", stat_info.core_idx, stat_info.event_idx, stat_info.counter_value);
            switch (stat_info.evt_note.type)
            {
            case NORMAL_EVT_NOTE:
                printf("NORMAL_EVT_NOTE\n");
                break;
            case GROUP_EVT_NOTE:
                printf("GROUP_EVT_NOTE, group_id=%u\n", stat_info.evt_note.note.group_note.group_id);
                break;
            case METRIC_EVT_NOTE:
                printf("METRIC_EVT_NOTE, group_id=%u, metric_name=%ls\n", stat_info.evt_note.note.metric_note.group_id, stat_info.evt_note.note.metric_note.name);
                break;
            default:
                printf("Unrecognized event note type\n");
            }
        }
    }

    if (!wperf_close())
        return 1;

    return 0;
}
