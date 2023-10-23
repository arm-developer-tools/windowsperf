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

    if (!wperf_init() && !wperf_set_verbose(true))
    {
        printf("failed to init wperf\n");
        return 1;
    }

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
