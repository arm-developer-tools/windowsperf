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

#include <DriverSpecs.h>
_Analysis_mode_(_Analysis_code_type_user_code_)

#define INITGUID

#include <windows.h>
#include <strsafe.h>
#include <cfgmgr32.h>
#include <set>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <numeric>
#include "wperf.h"
#include "debug.h"
#include "wperf-common\public.h"
#include "wperf-common\macros.h"
#include "wperf-common\iorequest.h"
#include "utils.h"
#include "output.h"
#include "exception.h"
#include "pe_file.h"
#include "process_api.h"

using namespace WPerfOutput;

//
// Port start
//

#include <algorithm>
#include <exception>
//#include <format>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <map>
#include <deque>
#include <cwctype>
#include <ctime>
#include <devpkey.h>

#include <tchar.h>
#include <stdio.h>
#include <psapi.h>

BOOLEAN G_PerformAsyncIo;
BOOLEAN G_LimitedLoops;
ULONG G_AsyncIoLoopsNum;
WCHAR G_DevicePath[MAX_DEVPATH_LENGTH];

#pragma warning(push)
#pragma warning(disable:4100)
BOOL DeviceAsyncIoControl(
    _In_    HANDLE  hDevice,
    _In_    LPVOID  lpBuffer,
    _In_    DWORD   nNumberOfBytesToWrite,
    _Out_   LPVOID  lpOutBuffer,
    _In_    DWORD   nOutBufferSize,
    _Out_   LPDWORD lpBytesReturned
)
{
    ULONG numberOfBytesWritten = 0;

    if (lpBuffer != NULL)
    {
        if (!WriteFile(hDevice, lpBuffer, nNumberOfBytesToWrite, &numberOfBytesWritten, NULL))
        {
            WindowsPerfDbgPrint("Error: WriteFile failed: GetLastError=%d\n", GetLastError());
            return FALSE;
        }
    }

    if (lpOutBuffer != NULL)
    {
        if (!ReadFile(hDevice, lpOutBuffer, nOutBufferSize, lpBytesReturned, NULL))
        {
            WindowsPerfDbgPrint("Error: ReadFile failed: GetLastError=%d\n", GetLastError());
            return FALSE;
        }
    }

    return TRUE;
}
#pragma warning(pop)

#pragma comment (lib, "cfgmgr32.lib")
#pragma comment (lib, "User32.lib")
#pragma comment (lib, "ole32.lib")
#pragma comment (lib, "oleaut32.lib")

static std::map<uint8_t, wchar_t*>arm64_vendor_names =
{
    {0x41, L"Arm Limited"},
    {0x42, L"Broadcomm Corporation"},
    {0x43, L"Cavium Inc"},
    {0x44, L"Digital Equipment Corporation"},
    {0x46, L"Fujitsu Ltd"},
    {0x49, L"Infineon Technologies AG"},
    {0x4D, L"Motorola or Freescale Semiconductor Inc"},
    {0x4E, L"NVIDIA Corporation"},
    {0x50, L"Applied Micro Circuits Corporation"},
    {0x51, L"Qualcomm Inc"},
    {0x56, L"Marvell International Ltd"},
    {0x69, L"Intel Corporation"},
    {0xC0, L"Ampere Computing"}
};

enum evt_type
{
    EVT_NORMAL,
    EVT_GROUPED,
    EVT_METRIC_NORMAL,
    EVT_METRIC_GROUPED,
    EVT_PADDING,
    EVT_HDR,                //!< Used to make beginning of a new group. Not real event
};

static const wchar_t* evt_class_name[EVT_CLASS_NUM] =
{
    L"core",
    L"dsu",
    L"dmc_clk",
    L"dmc_clkdiv2",
};

static const wchar_t* evt_name_prefix[EVT_CLASS_NUM] =
{
    L"",
    L"/dsu/",
    L"/dmc_clk/",
    L"/dmc_clkdiv2/",
};

static uint8_t gpc_nums[EVT_CLASS_NUM];
static uint8_t fpc_nums[EVT_CLASS_NUM];

struct evt_noted
{
    uint16_t index;
    enum evt_type type;
    std::wstring note;
};

struct metric_desc
{
    std::map<enum evt_class, std::deque<struct evt_noted>> events;
    std::map<enum evt_class, std::vector<struct evt_noted>> groups;
    std::wstring raw_str;
};

static wchar_t* get_dmc_clk_event_name(uint16_t index)
{
    switch (index)
    {
#define WPERF_DMC_CLK_EVENTS(a,b,c) case b: return L##c;
#include "wperf-common\dmc-clk-events.def"
#undef WPERF_DMC_CLK_EVENTS
    default: return L"unknown event";
    }
}

static const wchar_t* get_dmc_clkdiv2_event_name(uint16_t index)
{
    switch (index)
    {
#define WPERF_DMC_CLKDIV2_EVENTS(a,b,c) case b: return L##c;
#include "wperf-common\dmc-clkdiv2-events.def"
#undef WPERF_DMC_CLKDIV2_EVENTS
    default: return L"unknown event";
    }
}

static const wchar_t* get_core_event_name(uint16_t index)
{
    switch (index)
    {
#define WPERF_ARMV8_ARCH_EVENTS(a,b,c) case b: return L##c;
#include "wperf-common\armv8-arch-events.def"
#undef WPERF_ARMV8_ARCH_EVENTS
    case 0xFFFF: return L"cycle";
    default: return L"unknown event";
    }
}

static const wchar_t* get_event_name(uint16_t index, enum evt_class e_class = EVT_CORE)
{
    if (e_class == EVT_DMC_CLK)
        return get_dmc_clk_event_name(index);

    if (e_class == EVT_DMC_CLKDIV2)
        return get_dmc_clkdiv2_event_name(index);

    return get_core_event_name(index);
}

static int get_core_event_index(std::wstring name)
{
#define WPERF_ARMV8_ARCH_EVENTS(a,b,c) if (name == L##c) return b;
#include "wperf-common\armv8-arch-events.def"
#undef WPERF_ARMV8_ARCH_EVENTS
    return -1;
}

static int get_dmc_clk_event_index(std::wstring name)
{
#define WPERF_DMC_CLK_EVENTS(a,b,c) if (name == L##c) return b;
#include "wperf-common\dmc-clk-events.def"
#undef WPERF_DMC_CLK_EVENTS
    return -1;
}

static int get_dmc_clkdiv2_event_index(std::wstring name)
{
#define WPERF_DMC_CLKDIV2_EVENTS(a,b,c) if (name == L##c) return b;
#include "wperf-common\dmc-clkdiv2-events.def"
#undef WPERF_DMC_CLKDIV2_EVENTS
    return -1;
}

static int get_event_index(std::wstring name, enum evt_class e_class = EVT_CORE)
{
    if (e_class == EVT_DMC_CLK)
        return get_dmc_clk_event_index(name);

    if (e_class == EVT_DMC_CLKDIV2)
        return get_dmc_clkdiv2_event_index(name);

    return get_core_event_index(name);
}

enum
{
#define WPERF_ARMV8_ARCH_EVENTS(a,b,c) PMU_EVENT_##a = b,
#include "wperf-common\armv8-arch-events.def"
#undef WPERF_ARMV8_ARCH_EVENTS
};

enum
{
#define WPERF_DMC_CLKDIV2_EVENTS(a,b,c) DMC_EVENT_##a = b,
#include "wperf-common\dmc-clkdiv2-events.def"
#undef WPERF_DMC_CLKDIV2_EVENTS
};

typedef std::vector<std::wstring> wstr_vec;

static bool sort_ioctl_events_sample(const struct evt_sample_src& a, const struct evt_sample_src& b)
{
    return a.index < b.index;
}

class user_request
{
public:
    user_request() : do_list{ false }, do_count(false), do_kernel(false), do_timeline(false),
        do_sample(false), do_version(false), do_verbose(false), do_test(false),
        do_help(false), dmc_idx(_UI8_MAX), count_duration(-1.0),
        sample_image_name(L""), sample_pe_file(L""), sample_pdb_file(L""),
        sample_display_row(50), sample_display_short(true),
        count_interval(-1.0), report_l3_cache_metric(false), report_ddr_bw_metric(false) {}

    void init(wstr_vec& raw_args, uint8_t core_num, std::map<std::wstring, metric_desc>& builtin_metrics)
    {
        bool waiting_events = false;
        bool waiting_metrics = false;
        bool waiting_core_idx = false;
        bool waiting_dmc_idx = false;
        bool waiting_duration = false;
        bool waiting_interval = false;
        bool waiting_config = false;
        bool waiting_filename = false;
        bool waiting_image_name = false;
        bool waiting_pe_file = false;
        bool waiting_pdb_file = false;
        bool waiting_sample_display_row = false;
        std::map<enum evt_class, std::deque<struct evt_noted>> events;
        std::map<enum evt_class, std::vector<struct evt_noted>> groups;

        // Fill cores_idx with {0, ... core_num}
        cores_idx.clear();
        cores_idx.resize(core_num);
        std::iota(cores_idx.begin(), cores_idx.end(), (UINT8)0);

        for (auto a : raw_args)
        {
            if (waiting_config)
            {
                waiting_config = false;
                load_config(a);
                continue;
            }

            if (a == L"-C")
            {
                waiting_config = true;
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

        for (auto a : raw_args)
        {
            if (waiting_config)
            {
                waiting_config = false;
                continue;
            }

            if (waiting_events)
            {
                if (do_sample)
                    parse_events_str_for_sample(a);
                else
                    parse_events_str(a, events, groups, L"");
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
                    for (auto x : desc.events)
                        events[x.first].insert(events[x.first].end(), x.second.begin(), x.second.end());
                    for (auto y : desc.groups)
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

            if (a == L"-C")
            {
                waiting_config = true;
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

            if (a == L"-d" || a == L"sleep")
            {
                waiting_duration = true;
                continue;
            }

            if (a == L"--output")
            {
                waiting_filename = true;
                m_outputType = TableOutputL::ALL;
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

            if (a == L"-version")
            {
                do_version = true;
                continue;
            }

            if (a == L"-h")
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
                if(m_outputType != TableOutputL::ALL)
                {
                    m_outputType = TableOutputL::JSON;
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

        for (uint32_t core_idx : cores_idx)
            if (core_idx >= core_num) {
                m_out.GetErrorOutputStream() << L"core index " << core_idx << L" not allowed. Use 0-" << (core_num - 1)
                    << L", see option -c <n>" << std::endl;
                throw fatal_exception("ERROR_CORES");
            }

        if (groups.size())
        {
            for (auto a : groups)
            {
                auto total_size = a.second.size();
                enum evt_class e_class = a.first;

                // We start with group_start === 1 because at "a" index [0] there is a EVT_HDR event.
                // This event is not "an event". It is a placeholder which stores following group event size.
                // Please note that we will read EVT_HDR.index for event group size here.
                //
                //  a.second vector contains:
                //
                //      EVT_HDR(m), EVT_m1, EVT_m2, ..., EVT_HDR(n), EVT_n1, EVT_n2, ...
                //
                for (uint16_t group_start = 1, group_size = a.second[0].index, group_num = 0; group_start < total_size; group_num++)
                {
                    for (uint16_t elem_idx = group_start; elem_idx < (group_start + group_size); elem_idx++)
                        push_ioctl_grouped_event(e_class, a.second[elem_idx], group_num);

                    padding_ioctl_events(e_class, events[e_class]);

                    // Make sure we are not reading beyond last element in a.second vector.
                    // Here if there's another event we want to read next EVT_HDR to know next group size.
                    const uint16_t next_evt_hdr_idx = group_start + group_size;
                    if (next_evt_hdr_idx >= total_size)
                        break;
                    uint16_t next_evt_group_size = a.second[next_evt_hdr_idx].index;
                    group_start += group_size + 1;
                    group_size = next_evt_group_size;
                }
            }
        }

        if (events.size())
        {
            for (auto a : events)
            {
                enum evt_class e_class = a.first;

                for (auto e : a.second)
                    push_ioctl_normal_event(e_class, e);

                if (groups.find(e_class) != groups.end())
                    padding_ioctl_events(e_class);
            }
        }

        std::sort(ioctl_events_sample.begin(), ioctl_events_sample.end(), sort_ioctl_events_sample);
    }

    bool has_events()
    {
        return !!ioctl_events.size();
    }

    void show_events()
    {
        m_out.GetOutputStream() << L"events to be counted:" << std::endl;

        for (auto a : ioctl_events)
        {
            m_out.GetOutputStream() << L"  " << std::setw(18) << evt_class_name[a.first] << L":";
            for (auto b : a.second)
                m_out.GetOutputStream() << L" " << IntToHexWideString(b.index);
            m_out.GetOutputStream() << std::endl;
        }
    }

    void push_ioctl_normal_event(enum evt_class e_class, struct evt_noted event)
    {
        ioctl_events[e_class].push_back(event);
    }

    void push_ioctl_padding_event(enum evt_class e_class, uint16_t event)
    {
        ioctl_events[e_class].push_back({ event, EVT_PADDING, L"p" });
    }

    void push_ioctl_grouped_event(enum evt_class e_class, struct evt_noted event, uint16_t group_num)
    {
        if (event.note == L"")
            event.note = L"g" + std::to_wstring(group_num);
        else
            event.note = L"g" + std::to_wstring(group_num) + L"," + event.note;
        ioctl_events[e_class].push_back(event);
    }

    void load_config(std::wstring config_name)
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
                    parse_events_str(mdesc.raw_str, mdesc.events, mdesc.groups, key);
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

    void parse_events_str_for_sample(std::wstring events_str)
    {
        std::wistringstream event_stream(events_str);
        std::wstring event;

        while(std::getline(event_stream, event, L','))
        {
            uint32_t raw_event, interval = 0x4000000;
            size_t delim_pos = event.find(L":");
            std::wstring str1;

            if (delim_pos == std::string::npos)
            {
                str1 = event;
            }
            else
            {
                str1 = event.substr(0, delim_pos);
                interval = std::stoi(event.substr(delim_pos + 1, std::string::npos), NULL, 0);
            }

            if (std::iswdigit(str1[0]))
            {
                raw_event = static_cast<uint32_t>(std::stoi(str1, nullptr, 0));
            }
            else if (str1[0] == L'r' &&
                     str1.length() == 5 &&
                     std::iswxdigit(str1[1]) &&
                     std::iswxdigit(str1[2]) &&
                     std::iswxdigit(str1[3]) &&
                     std::iswxdigit(str1[4]))
            {
                raw_event = static_cast<uint32_t>(std::stoi(str1.substr(1, std::string::npos), NULL, 16));
            }
            else
            {
                int idx = get_event_index(str1);

                if (idx < 0)
                {
                    std::wcout << L"unknown event name: " << str1 << std::endl;
                    exit(-1);
                }

                raw_event = static_cast<uint32_t>(idx);
            }

            // convert to fixed cycle event to save one GPC
            if (raw_event == 0x11)
                raw_event = CYCLE_EVT_IDX;

            struct evt_sample_src _evt = {raw_event, interval};
            ioctl_events_sample.push_back(_evt);
        }
    }

    static void parse_events_str(std::wstring events_str, std::map<enum evt_class, std::deque<struct evt_noted>>& events,
        std::map<enum evt_class, std::vector<struct evt_noted>>& groups, std::wstring note)
    {
        std::wistringstream event_stream(events_str);
        std::wstring event;
        uint16_t group_size = 0;

        while (std::getline(event_stream, event, L','))
        {
            bool push_group = false, push_group_last = false;
            enum evt_class e_class = EVT_CORE;
            const wchar_t* chars_try = event.c_str();
            uint16_t raw_event;

            if (wcsncmp(chars_try, L"/dsu/", 5) == 0)
            {
                e_class = EVT_DSU;
                event.erase(0, 5);
            }
            else if (wcsncmp(chars_try, L"/dmc_clk/", 9) == 0)
            {
                e_class = EVT_DMC_CLK;
                event.erase(0, 9);
            }
            else if (wcsncmp(chars_try, L"/dmc_clkdiv2/", 13) == 0)
            {
                e_class = EVT_DMC_CLKDIV2;
                event.erase(0, 13);
            }

            const wchar_t* chars = event.c_str();

            if (std::iswdigit(chars[0]))
            {
                raw_event = static_cast<uint16_t>(wcstol(chars, NULL, 0));

                if (group_size)
                {
                    push_group = true;
                    group_size++;
                }
            }
            else if (chars[0] == L'r' &&
                event.length() == 5 &&
                std::iswxdigit(chars[1]) &&
                std::iswxdigit(chars[2]) &&
                std::iswxdigit(chars[3]) &&
                std::iswxdigit(chars[4]))
            {
                raw_event = static_cast<uint16_t>(wcstol(chars + 1, NULL, 16));

                if (group_size)
                {
                    push_group = true;
                    group_size++;
                }
            }
            else
            {
                if (event[0] == L'{')
                {
                    if (group_size)
                    {
                        m_out.GetErrorOutputStream() << L"nested group is not supported " << event << std::endl;
                        throw fatal_exception("ERROR_UNSUPPORTED");
                    }

                    std::wstring strip_str = event.substr(1);
                    if (strip_str.back() == L'}')
                    {
                        strip_str.pop_back();
                    }
                    else
                    {
                        push_group = true;
                        group_size++;
                    }

                    chars = strip_str.c_str();
                    if (std::iswdigit(chars[0]))
                    {
                        raw_event = static_cast<uint16_t>(wcstol(chars, NULL, 0));
                    }
                    else
                    {
                        int idx = get_event_index(strip_str, e_class);

                        if (idx < 0)
                        {
                            m_out.GetErrorOutputStream() << L"unknown event name: " << strip_str << std::endl;
                            throw fatal_exception("ERROR_EVENT");
                        }

                        raw_event = static_cast<uint16_t>(idx);
                    }
                }
                else if (event.back() == L'}')
                {
                    if (!group_size)
                    {
                        m_out.GetErrorOutputStream() << L"missing '{' for event group " << event << std::endl;
                        throw fatal_exception("ERROR_EVENT");
                    }

                    push_group = true;
                    push_group_last = true;

                    event.pop_back();

                    chars = event.c_str();
                    if (std::iswdigit(chars[0]))
                    {
                        raw_event = static_cast<uint16_t>(wcstol(chars, NULL, 0));
                    }
                    else
                    {
                        int idx = get_event_index(event, e_class);

                        if (idx < 0)
                        {
                            m_out.GetErrorOutputStream() << L"unknown event name: " << event << std::endl;
                            throw fatal_exception("ERROR_EVENT");
                        }

                        raw_event = static_cast<uint16_t>(idx);
                    }

                    group_size++;
                }
                else
                {
                    int idx = get_event_index(event, e_class);

                    if (idx < 0)
                    {
                        m_out.GetErrorOutputStream() << L"unknown event name: " << event << std::endl;
                        throw fatal_exception("ERROR_EVENT");
                    }

                    raw_event = static_cast<uint16_t>(idx);
                    if (group_size)
                    {
                        push_group = true;
                        group_size++;
                    }
                }
            }

            if (push_group)
            {
                groups[e_class].push_back({ raw_event, note == L"" ? EVT_GROUPED : EVT_METRIC_GROUPED, note });
                if (push_group_last)
                {
                    if (group_size > gpc_nums[e_class])
                    {
                        m_out.GetErrorOutputStream() << L"event group size(" << group_size
                            << ") exceeds number of hardware PMU counter("
                            << gpc_nums[e_class] << ")" << std::endl;
                        throw fatal_exception("ERROR_GROUP");
                    }

                    // Insert EVT_HDR in front of group we've just added. This entity
                    // will store information about following group of events.
                    auto it = groups[e_class].end();
                    groups[e_class].insert(std::prev(it, group_size), { group_size, EVT_HDR, note });
                    group_size = 0;
                }
            }
            else
            {
                if (note == L"")
                    events[e_class].push_back({ raw_event, EVT_NORMAL, L"e" });
                else
                    events[e_class].push_back({ raw_event, EVT_METRIC_NORMAL, L"e," + note });
            }
        }
    }
private:
    bool all_cores_p() {
        return cores_idx.size() > 1;
    }

    std::wstring trim(const std::wstring& str,
        const std::wstring& whitespace = L" \t")
    {
        const auto pos_begin = str.find_first_not_of(whitespace);
        if (pos_begin == std::wstring::npos)
            return L"";

        const auto pos_end = str.find_last_not_of(whitespace);
        const auto len = pos_end - pos_begin + 1;

        return str.substr(pos_begin, len);
    }

    void padding_ioctl_events(enum evt_class e_class, std::deque<struct evt_noted>& padding_vector)
    {
        auto event_num = ioctl_events[e_class].size();
        uint8_t gpc_num = gpc_nums[e_class];

        if (!(event_num % gpc_num))
            return;

        auto event_num_after_padding = (event_num + gpc_num - 1) / gpc_num * gpc_num;
        for (auto idx = 0; idx < (event_num_after_padding - event_num); idx++)
        {
            if (padding_vector.size())
            {
                struct evt_noted e = padding_vector.front();
                padding_vector.pop_front();
                push_ioctl_normal_event(e_class, e);
            }
            else
            {
                push_ioctl_padding_event(e_class, PMU_EVENT_INST_SPEC);
            }
        }
    }

    void padding_ioctl_events(enum evt_class e_class)
    {
        auto event_num = ioctl_events[e_class].size();
        uint8_t gpc_num = gpc_nums[e_class];

        if (!(event_num % gpc_num))
            return;

        auto event_num_after_padding = (event_num + gpc_num - 1) / gpc_num * gpc_num;
        for (auto idx = 0; idx < (event_num_after_padding - event_num); idx++)
            push_ioctl_padding_event(e_class, PMU_EVENT_INST_SPEC);
    }
};

class pmu_device
{
public:
    pmu_device() : handle(NULL), count_kernel(false), has_dsu(false), dsu_cluster_num(0),
	    dsu_cluster_size(0), has_dmc(false), dmc_num(0), enc_bits(0), core_num(0),
		dmc_idx(0), pmu_ver(0), timeline_mode(false), vendor_name(0), do_verbose(false)
    {
        for (int e = EVT_CLASS_FIRST; e < EVT_CLASS_NUM; e++)
            multiplexings[e] = false;
    }

    void init(HANDLE hDevice)
    {
        handle = hDevice;

        struct hw_cfg hw_cfg;
        query_hw_cfg(hw_cfg);

        assert(hw_cfg.core_num <= UCHAR_MAX);
        core_num = (UINT8)hw_cfg.core_num;
        fpc_nums[EVT_CORE] = hw_cfg.fpc_num;
        uint8_t gpc_num = hw_cfg.gpc_num;
        gpc_nums[EVT_CORE] = gpc_num;
        pmu_ver = hw_cfg.pmu_ver;

        vendor_name = get_vendor_name(hw_cfg.vendor_id);
        core_outs = std::make_unique<ReadOut[]>(core_num);
        memset(core_outs.get(), 0, sizeof(ReadOut) * core_num);

        // Only support metrics based on Arm's default core implementation
        if ((hw_cfg.vendor_id == 0x41 || hw_cfg.vendor_id == 0x51) && gpc_num >= 5)
        {

            if (gpc_num == 5)
            {
                set_builtin_metrics(L"imix", L"{inst_spec,dp_spec,vfp_spec,ase_spec,ldst_spec}");
            }
            else
            {
                set_builtin_metrics(L"imix", L"{inst_spec,dp_spec,vfp_spec,ase_spec,ld_spec,st_spec}");
            }

            set_builtin_metrics(L"icache", L"{l1i_cache,l1i_cache_refill,l2i_cache,l2i_cache_refill,inst_retired}");
            set_builtin_metrics(L"dcache", L"{l1d_cache,l1d_cache_refill,l2d_cache,l2d_cache_refill,inst_retired}");
            set_builtin_metrics(L"itlb",   L"{l1i_tlb,l1i_tlb_refill,l2i_tlb,l2i_tlb_refill,inst_retired}");
            set_builtin_metrics(L"dtlb",   L"{l1d_tlb,l1d_tlb_refill,l2d_tlb,l2d_tlb_refill,inst_retired}");
        }

        // Detect unCore PMU from Arm Ltd - System Cache
        has_dsu = detect_armh_dsu();
        if (has_dsu)
        {
            struct dsu_ctl_hdr ctl;
            struct dsu_cfg cfg;
            DWORD res_len;

            ctl.action = DSU_CTL_INIT;
            assert(dsu_cluster_num <= USHRT_MAX);
            assert(dsu_cluster_size <= USHRT_MAX);
            ctl.cluster_num = (uint16_t)dsu_cluster_num;
            ctl.cluster_size = (uint16_t)dsu_cluster_size;
            BOOL status = DeviceAsyncIoControl(handle, &ctl, sizeof(dsu_ctl_hdr), &cfg, sizeof(dsu_cfg), &res_len);
            if (!status)
            {
                m_out.GetErrorOutputStream() << L"DSU_CTL_INIT failed" << std::endl;
                throw fatal_exception("ERROR_PMU_INIT");
            }

            if (res_len != sizeof(struct dsu_cfg))
            {
                m_out.GetErrorOutputStream() << L"DSU_CTL_INIT returned unexpected length of data" << std::endl;
                throw fatal_exception("ERROR_PMU_INIT");
            }

            gpc_nums[EVT_DSU] = cfg.gpc_num;
            fpc_nums[EVT_DSU] = cfg.fpc_num;

            dsu_outs = std::make_unique<DSUReadOut[]>(dsu_cluster_num);
            memset(dsu_outs.get(), 0, sizeof(DSUReadOut)* dsu_cluster_num);

            set_builtin_metrics(L"l3_cache", L"/dsu/l3d_cache,/dsu/l3d_cache_refill");
        }

        // unCore PMU - DDR controller
        has_dmc = detect_armh_dma();
        if (has_dmc)
        {
            size_t len = sizeof(dmc_ctl_hdr) + dmc_regions.size() * sizeof(uint64_t) * 2;
            std::unique_ptr<uint8_t[]> buf = std::make_unique<uint8_t[]>(len);

            struct dmc_ctl_hdr* ctl = (struct dmc_ctl_hdr*)buf.get();
            struct dmc_cfg cfg;
            DWORD res_len;

            for (int i = 0; i < dmc_regions.size(); i++)
            {
                ctl->addr[i * 2] = dmc_regions[i].first;
                ctl->addr[i * 2 + 1] = dmc_regions[i].second;
            }

            ctl->action = DMC_CTL_INIT;
            assert(dmc_regions.size() <= UCHAR_MAX);
            ctl->dmc_num = (uint8_t)dmc_regions.size();
            BOOL status = DeviceAsyncIoControl(handle, ctl, (DWORD)len, &cfg, (DWORD)sizeof(dmc_cfg), &res_len);
            if (!status)
            {
                m_out.GetErrorOutputStream() << L"DMC_CTL_INIT failed" << std::endl;
                throw fatal_exception("ERROR_PMU_INIT");
            }

            gpc_nums[EVT_DMC_CLK] = cfg.clk_gpc_num;
            fpc_nums[EVT_DMC_CLK] = 0;
            gpc_nums[EVT_DMC_CLKDIV2] = cfg.clkdiv2_gpc_num;
            fpc_nums[EVT_DMC_CLKDIV2] = 0;

            dmc_outs = std::make_unique<DMCReadOut[]>(ctl->dmc_num);
            memset(dmc_outs.get(), 0, sizeof(DMCReadOut)* ctl->dmc_num);

            set_builtin_metrics(L"ddr_bw", L"/dmc_clkdiv2/rdwr");
        }
    }

    // post_init members
    void post_init(std::vector<uint8_t> cores_idx_init, uint32_t dmc_idx_init, bool timeline_mode_init, uint32_t enable_bits)
    {
        // Initliaze core numbers, please note we are sorting cores ascending
        // because we may relay in ascending order for some simple algorithms.
        // For example in wperf-driver::deviceControl() we only init one core
        // per DSU cluster.
        cores_idx = cores_idx_init;
        std::sort(cores_idx.begin(), cores_idx.end());  // Keep this sorting!
        // We want to keep only unique cores
        cores_idx.erase(unique(cores_idx.begin(), cores_idx.end()), cores_idx.end());

        dmc_idx = (uint8_t)dmc_idx_init;
        timeline_mode = timeline_mode_init;
        enc_bits = enable_bits;

        if (has_dsu)
            // Gather all DSU numbers for specified core in set of unique DSU numbers
            for (uint32_t i : cores_idx)
                dsu_cores.insert(i / dsu_cluster_size);

        if (!timeline_mode_init)
            return;

        time_t rawtime;
        struct tm timeinfo;

        time(&rawtime);
        localtime_s(&timeinfo, &rawtime);

        for (int e = EVT_CLASS_FIRST; e < EVT_CLASS_NUM; e++)
        {
            if (e == EVT_CORE && !(enc_bits & CTL_FLAG_CORE))
                continue;

            if (e == EVT_DSU && !(enc_bits & CTL_FLAG_DSU))
                continue;

            if ((e == EVT_DMC_CLK || e == EVT_DMC_CLKDIV2) && !(enc_bits & CTL_FLAG_DMC))
                continue;

            char buf[256];
            size_t length = strftime(buf, sizeof(buf), "%Y_%m_%d_%H_%M_%S.", &timeinfo);
            std::string filename = std::string(buf, buf + strlen(buf));
            if (all_cores_p())
                snprintf(buf, sizeof(buf), "wperf_system_side_");
            else
                snprintf(buf, sizeof(buf), "wperf_core_%d_", cores_idx[0]);
            std::string prefix(buf);

            std::string suffix = MultiByteFromWideString(evt_class_name[e]);

            filename = prefix + filename + suffix + std::string(".csv");
            if (!length)
            {
                std::cerr << "timeline output file name: " << filename << std::endl;
                throw fatal_exception("open timeline output file failed");
            }

            timeline_outfiles[e].open(filename);
        }
    }

    ~pmu_device()
    {
        if (timeline_mode)
        {
            for (int e = EVT_CLASS_FIRST; e < EVT_CLASS_NUM; e++)
                if (enc_bits & (1 << e))
                    timeline_outfiles[e].close();
        }

        CloseHandle(handle);
    }

    void set_sample_src(std::vector<struct evt_sample_src> &sample_sources, bool sample_kernel)
    {
        PMUSampleSetSrcHdr *ctl;
        DWORD res_len;
        size_t sz;

        if (sample_sources.size())
        {
            sz = sizeof(PMUSampleSetSrcHdr) + sample_sources.size() * sizeof(SampleSrcDesc);
            ctl = reinterpret_cast<PMUSampleSetSrcHdr *>(new uint8_t[sz]);
            for (int i = 0; i < sample_sources.size(); i++)
            {
                SampleSrcDesc *dst = ctl->sources + i;
                struct evt_sample_src src = sample_sources[i];
                dst->event_src = src.index;
                dst->interval = src.interval;
                dst->filter_bits = sample_kernel ? 0 : FILTER_BIT_EXCL_EL1;
            }
        }
        else
        {
            sz = sizeof(PMUSampleSetSrcHdr) + sizeof(SampleSrcDesc);
            ctl = reinterpret_cast<PMUSampleSetSrcHdr *>(new uint8_t[sz]);
            ctl->sources[0].event_src = CYCLE_EVT_IDX;
            ctl->sources[0].interval = 0x8000000;
            ctl->sources[0].filter_bits = sample_kernel ? 0 : FILTER_BIT_EXCL_EL1;
        }

        ctl->action = PMU_CTL_SAMPLE_SET_SRC;
        ctl->core_idx = cores_idx[0];   // Only one core for sampling!
        BOOL status = DeviceAsyncIoControl(handle, ctl, (DWORD)sz, NULL, 0, &res_len);
        delete[] ctl;
        if (!status)
            throw fatal_exception("PMU_CTL_SAMPLE_SET_SRC failed");
    }

    // Return false if sample buffer was empty
    bool get_sample(std::vector<FrameChain> &sample_info)
    {
        struct PMUCtlGetSampleHdr hdr;
        hdr.action = PMU_CTL_SAMPLE_GET;
        hdr.core_idx = cores_idx[0];
        DWORD res_len;

        PMUSamplePayload framesPayload = {0};

        BOOL status = DeviceAsyncIoControl(handle, &hdr, sizeof(struct PMUCtlGetSampleHdr), &framesPayload, sizeof(PMUSamplePayload), &res_len);
        if (!status)
            throw fatal_exception("PMU_CTL_SAMPLE_GET failed");

        if (framesPayload.size != SAMPLE_CHAIN_BUFFER_SIZE)
            return false;

        FrameChain *frames = (FrameChain *)framesPayload.payload;
        for (int i = 0; i < (res_len / sizeof(FrameChain)); i++)
            sample_info.push_back(frames[i]);

        return true;
    }

    void start_sample()
    {
        struct pmu_ctl_hdr ctl;
        DWORD res_len;

        ctl.action = PMU_CTL_SAMPLE_START;
        ctl.cores_idx.cores_count = 1;
        ctl.cores_idx.cores_no[0] = cores_idx[0];
        BOOL status = DeviceAsyncIoControl(handle, &ctl, sizeof(struct pmu_ctl_hdr), NULL, 0, &res_len);
        if (!status)
            throw fatal_exception("PMU_CTL_SAMPLE_START failed");
    }

    struct pmu_sample_summary
    {
        uint64_t sample_generated;
        uint64_t sample_dropped;
    };

    void stop_sample()
    {
        struct pmu_ctl_hdr ctl;
        struct pmu_sample_summary summary;
        DWORD res_len;

        ctl.action = PMU_CTL_SAMPLE_STOP;
        ctl.cores_idx.cores_count = 1;
        ctl.cores_idx.cores_no[0] = cores_idx[0];
        BOOL status = DeviceAsyncIoControl(handle, &ctl, sizeof(struct pmu_ctl_hdr), &summary, sizeof(struct pmu_sample_summary), &res_len);
        if (!status)
            throw fatal_exception("PMU_CTL_SAMPLE_STOP failed");

        if (do_verbose)
        {
            std::wcout << L"=================" << std::endl;
            std::wcout << L"sample generated: " << std::dec << summary.sample_generated << std::endl;
            std::wcout << L"sample dropped  : " << std::dec << summary.sample_dropped << std::endl;
        }
    }

    void set_builtin_metrics(std::wstring key, std::wstring raw_str)
    {
        metric_desc mdesc;
        mdesc.raw_str = raw_str;
        user_request::parse_events_str(mdesc.raw_str, mdesc.events, mdesc.groups, key);
        builtin_metrics[key] = mdesc;
        mdesc.events.clear();
        mdesc.groups.clear();
    }

    void start(uint32_t flags = CTL_FLAG_CORE)
    {
        struct pmu_ctl_hdr ctl;
        DWORD res_len;

        ctl.action = PMU_CTL_START;
        ctl.cores_idx.cores_count = cores_idx.size();
        std::copy(cores_idx.begin(), cores_idx.end(), ctl.cores_idx.cores_no);

        ctl.dmc_idx = dmc_idx;
        ctl.flags = flags;
        BOOL status = DeviceAsyncIoControl(handle, &ctl, sizeof(struct pmu_ctl_hdr), NULL, 0, &res_len);
        if (!status)
            throw fatal_exception("PMU_CTL_START failed");
    }

    void stop(uint32_t flags = CTL_FLAG_CORE)
    {
        struct pmu_ctl_hdr ctl;
        DWORD res_len;

        ctl.action = PMU_CTL_STOP;
        ctl.cores_idx.cores_count = cores_idx.size();
        std::copy(cores_idx.begin(), cores_idx.end(), ctl.cores_idx.cores_no);
        ctl.dmc_idx = dmc_idx;
        ctl.flags = flags;

        BOOL status = DeviceAsyncIoControl(handle, &ctl, sizeof(struct pmu_ctl_hdr), NULL, 0, &res_len);
        if (!status)
            throw fatal_exception("PMU_CTL_STOP failed");
    }

    void reset(uint32_t flags = CTL_FLAG_CORE)
    {
        struct pmu_ctl_hdr ctl;
        DWORD res_len;

        ctl.action = PMU_CTL_RESET;
        ctl.cores_idx.cores_count = cores_idx.size();
        std::copy(cores_idx.begin(), cores_idx.end(), ctl.cores_idx.cores_no);
        ctl.dmc_idx = dmc_idx;
        ctl.flags = flags;
        BOOL status = DeviceAsyncIoControl(handle, &ctl, sizeof(struct pmu_ctl_hdr), NULL, 0, &res_len);
        if (!status)
            throw fatal_exception("PMU_CTL_RESET failed");
    }

    void events_assign(uint32_t core_idx, std::map<enum evt_class, std::vector<struct evt_noted>> events, bool include_kernel)
    {
        size_t acc_sz = 0;

        for (auto a : events)
        {
            enum evt_class e_class = a.first;
            size_t e_num = a.second.size();

            acc_sz += sizeof(struct evt_hdr) + e_num * sizeof(uint16_t);
            multiplexings[e_class] = !!(e_num > gpc_nums[e_class]);
        }

        if (!acc_sz)
            return;

        acc_sz += sizeof(struct pmu_ctl_evt_assign_hdr);

        std::unique_ptr<uint8_t[]> buf = std::make_unique<uint8_t[]>(acc_sz);

        DWORD res_len;
        struct pmu_ctl_evt_assign_hdr* ctl =
            reinterpret_cast<struct pmu_ctl_evt_assign_hdr*>(buf.get());

        ctl->action = PMU_CTL_ASSIGN_EVENTS;
        ctl->core_idx = core_idx;
        ctl->dmc_idx = dmc_idx;
        count_kernel = include_kernel;
        ctl->filter_bits = count_kernel ? 0 : FILTER_BIT_EXCL_EL1;
        uint16_t* ctl2 =
            reinterpret_cast<uint16_t*>(buf.get() + sizeof(struct pmu_ctl_evt_assign_hdr));

        for (auto a : events)
        {
            enum evt_class e_class = a.first;
            struct evt_hdr* hdr = reinterpret_cast<struct evt_hdr*>(ctl2);

            hdr->evt_class = e_class;
            hdr->num = (UINT16)a.second.size();
            uint16_t* payload = reinterpret_cast<uint16_t*>(hdr + 1);

            for (auto b : a.second)
                *payload++ = b.index;

            ctl2 = payload;
        }

        BOOL status = DeviceAsyncIoControl(handle, buf.get(), (DWORD)acc_sz, NULL, 0, &res_len);
        if (!status)
            throw fatal_exception("PMU_CTL_ASSIGN_EVENTS failed");

        if (!timeline_mode)
            return;

        bool multiplexing = multiplexings[EVT_CORE];

        for (auto a : events)
        {
            enum evt_class e_class = a.first;
            std::wofstream& timeline_outfile = timeline_outfiles[e_class];

            timeline_outfile << L"\"performance counter stats";
            if (multiplexing)
                timeline_outfile << L", multiplexed";
            else
                timeline_outfile << L", no multiplexing";

            if (e_class == EVT_CORE)
            {
                if (count_kernel)
                    timeline_outfile << L", kernel mode included";
                else
                    timeline_outfile << L", kernel mode excluded";
            }

            timeline_outfile << L", on " << vendor_name << L" " << evt_class_name[e_class] << " implementation\",";

            std::vector<struct evt_noted>& unit_events = events[e_class];
            uint32_t real_event_num = 0;

            for (auto e : unit_events)
            {
                if (e.type == EVT_PADDING)
                    continue;

                real_event_num++;
            }

            uint32_t iter_base, iter_end, comma_num, comma_num2;
            if (e_class == EVT_DMC_CLK || e_class == EVT_DMC_CLKDIV2)
            {
                if (dmc_idx == ALL_DMC_CHANNEL)
                {
                    comma_num = real_event_num * dmc_num - 1;
                    comma_num2 = real_event_num - 1;
                    iter_base = 0;
                    iter_end = dmc_num;
                }
                else
                {
                    comma_num = real_event_num - 1;
                    comma_num2 = comma_num;
                    iter_base = dmc_idx;
                    iter_end = dmc_idx + 1;
                }
            }
            else if (core_idx == ALL_CORE)
            {
                uint32_t block_num = e_class == EVT_DSU ? dsu_cluster_num : core_num;

                comma_num = multiplexing ? ((real_event_num + 1) * 2 * block_num - 1)
                    : ((real_event_num + 1) * block_num - 1);
                comma_num2 = multiplexing ? ((real_event_num + 1) * 2 - 1) : real_event_num;
                iter_base = 0;
                iter_end = core_num;
            }
            else
            {
                comma_num = multiplexing ? ((real_event_num + 1) * 2 - 1) : real_event_num;
                comma_num2 = comma_num;
                iter_base = core_idx;
                iter_end = core_idx + 1;
            }

            for (uint32_t idx = 0; idx < comma_num; idx++)
                timeline_outfile << L",";
            timeline_outfile << std::endl;

            uint32_t i_inc = e_class == EVT_DSU ? dsu_cluster_size : 1;
            for (uint32_t idx = iter_base; idx < iter_end; idx += i_inc)
            {
                timeline_outfile << evt_class_name[e_class] << L" " << idx / i_inc << L",";
                for (uint32_t i = 0; i < comma_num2; i++)
                    timeline_outfile << L",";
            }
            timeline_outfile << std::endl;

            uint32_t event_num = (uint32_t)unit_events.size();
            for (uint32_t i = iter_base; i < iter_end; i += i_inc)
            {
                if (e_class == EVT_DMC_CLK || e_class == EVT_DMC_CLKDIV2)
                {
                    for (uint32_t idx = 0; idx < event_num; idx++)
                    {
                        if (unit_events[idx].type == EVT_PADDING)
                            continue;
                        timeline_outfile << get_event_name(unit_events[idx].index, e_class) << L",";
                    }
                }
                else if (multiplexing)
                {
                    timeline_outfile << L"cycle,sched_times,";
                    for (uint32_t idx = 0; idx < event_num; idx++)
                    {
                        if (unit_events[idx].type == EVT_PADDING)
                            continue;
                        timeline_outfile << get_event_name(unit_events[idx].index) << L",sched_times,";
                    }
                }
                else
                {
                    timeline_outfile << L"cycle,";
                    for (uint32_t idx = 0; idx < event_num; idx++)
                    {
                        if (unit_events[idx].type == EVT_PADDING)
                            continue;
                        timeline_outfile << get_event_name(unit_events[idx].index) << L",";
                    }
                }
            }
            timeline_outfile << std::endl;
        }
    }

    void core_events_read_nth(uint8_t core_no)
    {
        struct pmu_ctl_hdr ctl;
        DWORD res_len;

        ctl.action = PMU_CTL_READ_COUNTING;
        ctl.cores_idx.cores_count = 1;
        ctl.cores_idx.cores_no[0] = core_no;

        LPVOID out_buf = core_outs.get() + core_no;
        size_t out_buf_len = sizeof(ReadOut);
        BOOL status = DeviceAsyncIoControl(handle, &ctl, (DWORD)sizeof(struct pmu_ctl_hdr), out_buf, (DWORD)out_buf_len, &res_len);
        if (!status)
            throw fatal_exception("PMU_CTL_READ_COUNTING failed");
    }

    void core_events_read()
    {
        for (uint8_t core_no : cores_idx)
        {
            core_events_read_nth(core_no);
        }
    }

    void dsu_events_read_nth(uint8_t core_no)
    {
        struct pmu_ctl_hdr ctl;
        DWORD res_len;

        ctl.action = DSU_CTL_READ_COUNTING;
        ctl.cores_idx.cores_count = 1;
        ctl.cores_idx.cores_no[0] = core_no;

        LPVOID out_buf = dsu_outs.get() + (core_no / dsu_cluster_size);
        size_t out_buf_len = sizeof(DSUReadOut);
        BOOL status = DeviceAsyncIoControl(handle, &ctl, (DWORD)sizeof(struct pmu_ctl_hdr), out_buf, (DWORD)out_buf_len, &res_len);
        if (!status)
            throw fatal_exception("DSU_CTL_READ_COUNTING failed");
    }

    void dsu_events_read(void)
    {
        for (uint8_t core_no : cores_idx)
        {
            dsu_events_read_nth(core_no);
        }
    }

    void dmc_events_read(void)
    {
        struct pmu_ctl_hdr ctl;
        DWORD res_len;

        ctl.action = DMC_CTL_READ_COUNTING;
        ctl.dmc_idx = dmc_idx;

        LPVOID out_buf = dmc_idx == ALL_DMC_CHANNEL ? dmc_outs.get() : dmc_outs.get() + dmc_idx;
        size_t out_buf_len = dmc_idx == ALL_DMC_CHANNEL ? (sizeof(DMCReadOut) * dmc_regions.size()) : sizeof(DMCReadOut);
        BOOL status = DeviceAsyncIoControl(handle, &ctl, (DWORD)sizeof(struct pmu_ctl_hdr), out_buf, (DWORD)out_buf_len, &res_len);
        if (!status)
            throw fatal_exception("DMC_CTL_READ_COUNTING failed");
    }

    void version_query(version_info& driver_ver)
    {
        struct pmu_ctl_ver_hdr ctl;
        DWORD res_len;

        ctl.action = PMU_CTL_QUERY_VERSION;
        ctl.version.major = MAJOR;
        ctl.version.minor = MINOR;
        ctl.version.patch = PATCH;

        BOOL status = DeviceAsyncIoControl(
            handle, &ctl, (DWORD)sizeof(struct pmu_ctl_ver_hdr), &driver_ver,
            (DWORD)sizeof(struct version_info), &res_len);

        if (!status)
            throw fatal_exception("PMU_CTL_QUERY_VERSION failed");
    }

    void events_query(std::map<enum evt_class, std::vector<uint16_t>>& events_out)
    {
        events_out.clear();

        enum pmu_ctl_action action = PMU_CTL_QUERY_SUPP_EVENTS;
        DWORD res_len;

        // Armv8's architecture defined + vendor defined events should be within 2K at the moment.
        auto buf_size = 2048 * sizeof(uint16_t);
        std::unique_ptr<uint8_t[]> buf = std::make_unique<uint8_t[]>(buf_size);
        BOOL status = DeviceAsyncIoControl(handle, &action, (DWORD)sizeof(enum pmu_ctl_action), buf.get(), (DWORD)buf_size, &res_len);
        if (!status)
            throw fatal_exception("PMU_CTL_QUERY_SUPP_EVENTS failed");

        DWORD consumed_sz = 0;
        for (; consumed_sz < res_len;)
        {
            struct evt_hdr* hdr = reinterpret_cast<struct evt_hdr*>(buf.get() + consumed_sz);
            enum evt_class evt_class = hdr->evt_class;
            uint16_t evt_num = hdr->num;
            uint16_t* events = reinterpret_cast<uint16_t*>(hdr + 1);

            for (int idx = 0; idx < evt_num; idx++)
                events_out[evt_class].push_back(events[idx]);

            consumed_sz += sizeof(struct evt_hdr) + evt_num * sizeof(uint16_t);
        }
    }

    void print_core_stat(std::vector<struct evt_noted>& events)
    {
        bool multiplexing = multiplexings[EVT_CORE];
        bool print_note = false;

        for (auto a : events)
        {
            if (a.type != EVT_NORMAL)
            {
                print_note = true;
                break;
            }
        }

        struct agg_entry
        {
            uint32_t event_idx;
            uint64_t counter_value;
            uint64_t scaled_value;
        };

        uint32_t core_base = cores_idx[0];
        std::unique_ptr<agg_entry[]> overall;

        if (all_cores_p())
        {
            // TODO: This will work only if we are counting EXACTLY same events for N cores
            overall = std::make_unique<agg_entry[]>(core_outs[core_base].evt_num);
            memset(overall.get(), 0, sizeof(agg_entry)* core_outs[core_base].evt_num);

            for (uint32_t i = 0; i < core_outs[core_base].evt_num; i++)
                overall[i].event_idx = core_outs[core_base].evts[i].event_idx;
        }

        for (uint32_t i : cores_idx)
        {
            if (!timeline_mode)
            {
                m_out.GetOutputStream() << std::endl
                           << L"Performance counter stats for core " << std::dec << i
                           << (multiplexing ? L", multiplexed" : L", no multiplexing")
                           << (count_kernel ? L", kernel mode included" : L", kernel mode excluded")
                           << L", on " << vendor_name << L" core implementation:"
                           << std::endl;

                m_out.GetOutputStream() << L"note: 'e' - normal event, 'gN' - grouped event with group number N, "
                              L"metric name will be appended if 'e' or 'g' comes from it"
                              << std::endl
                              << std::endl;
            }

            UINT32 evt_num = core_outs[i].evt_num;
            struct pmu_event_usr* evts = core_outs[i].evts;
            uint64_t round = core_outs[i].round;

            std::vector<std::wstring> col_counter_value, col_event_name, col_event_idx,
                                      col_multiplexed, col_scaled_value, col_event_note;

            for (size_t j = 0; j < evt_num; j++)
            {
                if (j >= 1 && (events[j - 1].type == EVT_PADDING))
                    continue;

                struct pmu_event_usr* evt = &evts[j];

                if (multiplexing)
                {
                    if (timeline_mode)
                    {
                        timeline_outfiles[EVT_CORE] << evt->value << L"," << evt->scheduled << L",";
                    }
                    else
                    {
                        if (evt->event_idx == CYCLE_EVT_IDX) {
                            col_counter_value.push_back(std::to_wstring(evt->value));
                            col_event_name.push_back(get_event_name((uint16_t)evt->event_idx));
                            col_event_idx.push_back(L"fixed");
                            col_event_note.push_back(L"e");
                            col_multiplexed.push_back(std::to_wstring(evt->scheduled) + L"/" + std::to_wstring(round));
                            col_scaled_value.push_back(std::to_wstring((uint64_t)((double)evt->value / ((double)evt->scheduled / (double)round))));
                        }
                        else {
                            col_counter_value.push_back(std::to_wstring(evt->value));
                            col_event_name.push_back(get_event_name((uint16_t)evt->event_idx));
                            col_event_idx.push_back(IntToHexWideString(evt->event_idx, 2));
                            col_event_note.push_back(events[j - 1].note);
                            col_multiplexed.push_back(std::to_wstring(evt->scheduled) + L"/" + std::to_wstring(round));
                            col_scaled_value.push_back(std::to_wstring((uint64_t)((double)evt->value / ((double)evt->scheduled / (double)round))));
                        }
                    }

                    if (overall)
                    {
                        overall[j].counter_value += evt->value;
                        overall[j].scaled_value += (uint64_t)((double)evt->value / ((double)evt->scheduled / (double)round));
                    }
                }
                else
                {
                    if (timeline_mode)
                    {
                        timeline_outfiles[EVT_CORE] << evt->value << L",";
                    }
                    else
                    {
                        if (evt->event_idx == CYCLE_EVT_IDX) {
                            col_counter_value.push_back(std::to_wstring(evt->value));
                            col_event_name.push_back(get_event_name((uint16_t)evt->event_idx));
                            col_event_idx.push_back(L"fixed");
                            col_event_note.push_back(L"e");
                        }
                        else {
                            col_counter_value.push_back(std::to_wstring(evt->value));
                            col_event_name.push_back(get_event_name((uint16_t)evt->event_idx));
                            col_event_idx.push_back(IntToHexWideString(evt->event_idx, 2));
                            col_event_note.push_back(events[j - 1].note);
                        }
                    }

                    if (overall)
                        overall[j].counter_value += evt->value;
                }
            }

            if (!timeline_mode) {
                TableOutputL table(m_outputType);

                if (multiplexing)
                {
                    table.PresetHeaders<PerformanceCounterOutputTraitsL<true>>();
                    table.SetAlignment(0, ColumnAlignL::RIGHT);
                    table.SetAlignment(4, ColumnAlignL::RIGHT);
                    table.SetAlignment(5, ColumnAlignL::RIGHT);
                    table.Insert(col_counter_value, col_event_name, col_event_idx, col_event_note, col_multiplexed, col_scaled_value);
                }
                else
                {
                    table.PresetHeaders<PerformanceCounterOutputTraitsL<false>>();
                    table.SetAlignment(0, ColumnAlignL::RIGHT);
                    table.Insert(col_counter_value, col_event_name, col_event_idx, col_event_note);
                }

                m_out.Print(table);
                table.m_core = GlobalStringType(std::to_wstring(i));
                m_globalJSON.m_multiplexing = multiplexing;
                m_globalJSON.m_kernel = count_kernel;
                m_globalJSON.m_corePerformanceTables.push_back(table);
            }
        }

        if (timeline_mode)
            timeline_outfiles[EVT_CORE] << std::endl;

        if (!overall)
            return;

        if (timeline_mode)
            return;

        m_out.GetOutputStream() << std::endl
                << L"System-wide Overall:" << std::endl;

        std::vector<std::wstring> col_counter_value, col_event_idx, col_event_name,
                                  col_scaled_value, col_event_note;

        UINT32 evt_num = core_outs[core_base].evt_num;

        for (size_t j = 0; j < evt_num; j++)
        {
            if (j >= 1 && (events[j - 1].type == EVT_PADDING))
                continue;

            struct agg_entry* entry = overall.get() + j;
            if (multiplexing)
            {
                if (entry->event_idx == CYCLE_EVT_IDX) {
                    col_counter_value.push_back(std::to_wstring(entry->counter_value));
                    col_event_name.push_back(get_event_name((uint16_t)entry->event_idx));
                    col_event_idx.push_back(L"fixed");
                    col_event_note.push_back(L"e");
                    col_scaled_value.push_back(std::to_wstring(entry->scaled_value));
                }
                else {
                    col_counter_value.push_back(std::to_wstring(entry->counter_value));
                    col_event_name.push_back(get_event_name((uint16_t)entry->event_idx));
                    col_event_idx.push_back(IntToHexWideString(entry->event_idx));
                    col_event_note.push_back(events[j - 1].note);
                    col_scaled_value.push_back(std::to_wstring(entry->scaled_value));
                }
            }
            else
            {
                if (entry->event_idx == CYCLE_EVT_IDX) {
                    col_counter_value.push_back(std::to_wstring(entry->counter_value));
                    col_event_name.push_back(get_event_name((uint16_t)entry->event_idx));
                    col_event_idx.push_back(L"fixed");
                    col_event_note.push_back(L"e");
                }
                else {
                    col_counter_value.push_back(std::to_wstring(entry->counter_value));
                    col_event_name.push_back(get_event_name((uint16_t)entry->event_idx));
                    col_event_idx.push_back(IntToHexWideString(entry->event_idx));
                    col_event_note.push_back(events[j - 1].note);
                }
            }
        }

        // Print System-wide Overall
        {
            TableOutputL table(m_outputType);
            if (multiplexing)
            {
                table.PresetHeaders<SystemwidePerformanceCounterOutputTraitsL<true>>();
                table.SetAlignment(0, ColumnAlignL::RIGHT);
                table.SetAlignment(4, ColumnAlignL::RIGHT);
                table.Insert(col_counter_value, col_event_name, col_event_idx, col_event_note, col_scaled_value);
            }
            else {
                table.PresetHeaders<SystemwidePerformanceCounterOutputTraitsL<false>>();
                table.SetAlignment(0, ColumnAlignL::RIGHT);
                table.Insert(col_counter_value, col_event_name, col_event_idx, col_event_note);
            }

            m_out.Print(table);
            m_globalJSON.m_coreOverall = table;
        }
    }

    void print_dsu_stat(std::vector<struct evt_noted>& events, bool report_l3_metric)
    {
        bool multiplexing = multiplexings[EVT_DSU];
        bool print_note = false;

        for (auto a : events)
        {
            if (a.type != EVT_NORMAL)
            {
                print_note = true;
                break;
            }
        }

        struct agg_entry
        {
            uint32_t event_idx;
            uint64_t counter_value;
            uint64_t scaled_value;
        };

        std::unique_ptr<agg_entry[]> overall;

        if (all_cores_p())
        {
            // TODO: because now all DSU cores count same events we use first core
            //       to gather events info.
            uint32_t dsu_core_0 = *(dsu_cores.begin());

            overall = std::make_unique<agg_entry[]>(dsu_outs[dsu_core_0].evt_num);
            memset(overall.get(), 0, sizeof(agg_entry)* dsu_outs[dsu_core_0].evt_num);

            for (uint32_t i = 0; i < dsu_outs[dsu_core_0].evt_num; i++)
                overall[i].event_idx = dsu_outs[dsu_core_0].evts[i].event_idx;
        }

        for (uint32_t dsu_core : dsu_cores)
        {
            if (!timeline_mode)
            {
                m_out.GetOutputStream() << std::endl
                           << L"Performance counter stats for DSU cluster "
                           << dsu_core
                           << (multiplexing ? L", multiplexed" : L", no multiplexing")
                           << L", on " << vendor_name
                           << L" core implementation:"
                           << std::endl;

                m_out.GetOutputStream() << L"note: 'e' - normal event, 'gN' - grouped event with group number N, "
                              L"metric name will be appended if 'e' or 'g' comes from it"
                              << std::endl
                              << std::endl;
            }

            UINT32 evt_num = dsu_outs[dsu_core].evt_num;
            struct pmu_event_usr* evts = dsu_outs[dsu_core].evts;
            uint64_t round = dsu_outs[dsu_core].round;

            std::vector<std::wstring> col_counter_value, col_event_idx,  col_event_name,
                                      col_multiplexed, col_scaled_value, col_event_note;

            for (size_t j = 0; j < evt_num; j++)
            {
                if (j >= 1 && (events[j - 1].type == EVT_PADDING))
                    continue;

                struct pmu_event_usr* evt = &evts[j];

                if (multiplexing)
                {
                    if (timeline_mode)
                    {
                        timeline_outfiles[EVT_DSU] << evt->value << L"," << evt->scheduled << L",";
                    }
                    else
                    {
                        if (evt->event_idx == CYCLE_EVT_IDX) {

                            col_counter_value.push_back(std::to_wstring(evt->value));
                            col_event_name.push_back(get_event_name((uint16_t)evt->event_idx));
                            col_event_idx.push_back(L"fixed");
                            col_event_note.push_back(L"e");
                            col_multiplexed.push_back(std::to_wstring(evt->scheduled) + L"/" + std::to_wstring(round));
                            col_scaled_value.push_back(std::to_wstring((uint64_t)((double)evt->value / ((double)evt->scheduled / (double)round))));
                        }
                        else {
                            col_counter_value.push_back(std::to_wstring(evt->value));
                            col_event_name.push_back(get_event_name((uint16_t)evt->event_idx));
                            col_event_idx.push_back(IntToHexWideString(evt->event_idx));
                            col_event_note.push_back(events[j - 1].note);
                            col_multiplexed.push_back(std::to_wstring(evt->scheduled) + L"/" + std::to_wstring(round));
                            col_scaled_value.push_back(std::to_wstring((uint64_t)((double)evt->value / ((double)evt->scheduled / (double)round))));
                        }
                    }

                    if (overall)
                    {
                        overall[j].counter_value += evt->value;
                        overall[j].scaled_value += (uint64_t)((double)evt->value / ((double)evt->scheduled / (double)round));
                    }
                }
                else
                {
                    if (timeline_mode)
                    {
                        timeline_outfiles[EVT_DSU] << evt->value << L",";
                    }
                    else
                    {
                        if (evt->event_idx == CYCLE_EVT_IDX) {

                            col_counter_value.push_back(std::to_wstring(evt->value));
                            col_event_name.push_back(get_event_name((uint16_t)evt->event_idx));
                            col_event_idx.push_back(L"fixed");
                            col_event_note.push_back(L"e");
                        }
                        else {
                            col_counter_value.push_back(std::to_wstring(evt->value));
                            col_event_name.push_back(get_event_name((uint16_t)evt->event_idx));
                            col_event_idx.push_back(IntToHexWideString(evt->event_idx));
                            col_event_note.push_back(events[j - 1].note);
                        }
                    }

                    if (overall)
                        overall[j].counter_value += evt->value;
                }
            }

            // Print performance counter stats for DSU cluster
            {
                TableOutputL table(m_outputType);

                if (multiplexing)
                {
                    table.PresetHeaders<PerformanceCounterOutputTraitsL<true>>();
                    table.SetAlignment(0, ColumnAlignL::RIGHT);
                    table.SetAlignment(4, ColumnAlignL::RIGHT);
                    table.SetAlignment(5, ColumnAlignL::RIGHT);
                    table.Insert(col_counter_value, col_event_name, col_event_idx, col_event_note, col_multiplexed, col_scaled_value);
                }
                else
                {
                    table.PresetHeaders<PerformanceCounterOutputTraitsL<false>>();
                    table.SetAlignment(0, ColumnAlignL::RIGHT);
                    table.Insert(col_counter_value, col_event_name, col_event_idx, col_event_note);
                }

                table.m_core = GlobalStringType(std::to_wstring(dsu_core));
                m_globalJSON.m_DSUPerformanceTables.push_back(table);

                m_out.Print(table);

            }
        }

        if (timeline_mode)
            timeline_outfiles[EVT_DSU] << std::endl;

        if (!overall)
        {
            if (!timeline_mode && report_l3_metric)
            {
                m_out.GetOutputStream() << std::endl
                           << L"L3 cache metrics:" << std::endl;

                std::vector<std::wstring> col_cluster, col_cores, col_read_bandwith, col_miss_rate;

                for (uint32_t dsu_core : dsu_cores)
                {
                    UINT32 evt_num = dsu_outs[dsu_core].evt_num;
                    struct pmu_event_usr* evts = dsu_outs[dsu_core].evts;
                    uint64_t l3_cache_access_num = 0, l3_cache_refill_num = 0;

                    for (size_t j = FIXED_COUNTERS_NO; j < evt_num; j++)
                    {
                        if (j >= 1 && (events[j - 1].type == EVT_PADDING))
                            continue;

                        if (events[j - 1].index == PMU_EVENT_L3D_CACHE)
                        {
                            l3_cache_access_num = evts[j].value;
                        }
                        else if (events[j - 1].index == PMU_EVENT_L3D_CACHE_REFILL)
                        {
                            l3_cache_refill_num = evts[j].value;
                        }
                    }

                    double miss_rate_pct = (l3_cache_access_num != 0 )
                        ? ((double)(l3_cache_refill_num)) / ((double)(l3_cache_access_num)) * 100
                        : 100.0;

                    std::wstring core_list;
                    for (uint8_t core_idx : cores_idx)
                        if ((core_idx / dsu_cluster_size) == (uint8_t)dsu_core)
                            if (core_list.empty())
                                core_list = L"" + std::to_wstring(core_idx);
                            else
                                core_list += L", " + std::to_wstring(core_idx);

                    col_cluster.push_back(std::to_wstring(dsu_core));
                    col_cores.push_back(core_list);
                    col_read_bandwith.push_back(DoubleToWideString(((double)(l3_cache_access_num * 64)) / 1024.0 / 1024.0) + L"MB");
                    col_miss_rate.push_back(DoubleToWideString(miss_rate_pct) + L"%");
                }

                {
                    TableOutputL table(m_outputType);
                    table.PresetHeaders<L3CacheMetricOutputTraitsL>();
                    table.SetAlignment(0, ColumnAlignL::RIGHT);
                    table.SetAlignment(1, ColumnAlignL::RIGHT);
                    table.SetAlignment(2, ColumnAlignL::RIGHT);
                    table.SetAlignment(3, ColumnAlignL::RIGHT);
                    table.Insert(col_cluster, col_cores, col_read_bandwith, col_miss_rate);
                    m_globalJSON.m_DSUL3metric = table;
                    m_out.Print(table);
                }
            }

            return;
        }

        if (timeline_mode)
            return;

        m_out.GetOutputStream() << std::endl;

        m_out.GetOutputStream() << L"System-wide Overall:" << std::endl;

        uint32_t dsu_core_0 = *(dsu_cores.begin());
        UINT32 evt_num = dsu_outs[dsu_core_0].evt_num;

        std::vector<std::wstring> col_counter_value, col_event_name, col_event_idx,
                                  col_event_note, col_scaled_value;

        for (size_t j = 0; j < evt_num; j++)
        {
            if (j >= 1 && (events[j - 1].type == EVT_PADDING))
                continue;

            struct agg_entry* entry = overall.get() + j;
            if (multiplexing)
            {
                if (entry->event_idx == CYCLE_EVT_IDX) {
                    col_counter_value.push_back(std::to_wstring(entry->counter_value));
                    col_event_name.push_back(get_event_name((uint16_t)entry->event_idx));
                    col_event_idx.push_back(L"fixed");
                    col_event_note.push_back(L"e");
                    col_scaled_value.push_back(std::to_wstring(entry->scaled_value));
                }
                else {
                    col_counter_value.push_back(std::to_wstring(entry->counter_value));
                    col_event_name.push_back(get_event_name((uint16_t)entry->event_idx));
                    col_event_idx.push_back(IntToHexWideString(entry->event_idx));
                    col_event_note.push_back(events[j - 1].note);
                    col_scaled_value.push_back(std::to_wstring(entry->scaled_value));
                }
            }
            else
            {
                if (entry->event_idx == CYCLE_EVT_IDX) {
                    col_counter_value.push_back(std::to_wstring(entry->counter_value));
                    col_event_name.push_back(get_event_name((uint16_t)entry->event_idx));
                    col_event_idx.push_back(L"fixed");
                    col_event_note.push_back(L"e");
                }
                else {
                    col_counter_value.push_back(std::to_wstring(entry->counter_value));
                    col_event_name.push_back(get_event_name((uint16_t)entry->event_idx));
                    col_event_idx.push_back(IntToHexWideString(entry->event_idx));
                    col_event_note.push_back(events[j - 1].note);
                }
            }
        }

        {
            TableOutputL table(m_outputType);
            if (multiplexing)
            {
                table.PresetHeaders<SystemwidePerformanceCounterOutputTraitsL<true>>();
                table.SetAlignment(0, ColumnAlignL::RIGHT);
                table.SetAlignment(4, ColumnAlignL::RIGHT);
                table.Insert(col_counter_value, col_event_name, col_event_idx, col_event_note, col_scaled_value);
            }
            else {
                table.PresetHeaders<SystemwidePerformanceCounterOutputTraitsL<false>>();
                table.SetAlignment(0, ColumnAlignL::RIGHT);
                table.Insert(col_counter_value, col_event_name, col_event_idx, col_event_note);
            }

            m_globalJSON.m_DSUOverall = table;
            m_out.Print(table);
        }

        if (report_l3_metric)
        {
            m_out.GetOutputStream() << std::endl
                       << L"L3 cache metrics:" << std::endl;

            std::vector<std::wstring> col_cluster, col_cores, col_read_bandwith, col_miss_rate;

            for (uint32_t dsu_core : dsu_cores)
            {
                UINT32 evt_num2 = dsu_outs[dsu_core].evt_num;
                struct pmu_event_usr* evts = dsu_outs[dsu_core].evts;
                uint64_t l3_cache_access_num = 0, l3_cache_refill_num = 0;

                for (size_t j = FIXED_COUNTERS_NO; j < evt_num2; j++)
                {
                    if (j >= 1 && (events[j - 1].type == EVT_PADDING))
                        continue;

                    if (events[j - 1].index == PMU_EVENT_L3D_CACHE)
                    {
                        l3_cache_access_num = evts[j].value;
                    }
                    else if (events[j - 1].index == PMU_EVENT_L3D_CACHE_REFILL)
                    {
                        l3_cache_refill_num = evts[j].value;
                    }
                }

                double miss_rate_pct = (l3_cache_access_num != 0 )
                    ? ((double)(l3_cache_refill_num)) / ((double)(l3_cache_access_num)) * 100
                    : 100.0;

                std::wstring core_list;
                for (uint8_t core_idx : cores_idx)
                    if ((core_idx / dsu_cluster_size) == (uint8_t)dsu_core)
                        if (core_list.empty())
                            core_list = L"" + std::to_wstring(core_idx);
                        else
                            core_list += L", " + std::to_wstring(core_idx);

                col_cluster.push_back(std::to_wstring(dsu_core));
                col_cores.push_back(core_list);
                col_read_bandwith.push_back(DoubleToWideString(((double)(l3_cache_access_num * 64)) / 1024.0 / 1024.0) + L"MB");
                col_miss_rate.push_back(DoubleToWideString(miss_rate_pct) + L"%");
            }

            uint32_t dsu_core = *dsu_cores.begin();
            UINT32 evt_num2 = dsu_outs[dsu_core].evt_num;
            uint64_t acc_l3_cache_access_num = 0, acc_l3_cache_refill_num = 0;

            for (size_t j = 0; j < evt_num2; j++)
            {
                if (j >= 1 && (events[j - 1].type == EVT_PADDING))
                    continue;

                struct agg_entry* entry = overall.get() + j;
                if (entry->event_idx == PMU_EVENT_L3D_CACHE)
                {
                    acc_l3_cache_access_num = entry->counter_value;
                }
                else if (entry->event_idx == PMU_EVENT_L3D_CACHE_REFILL)
                {
                    acc_l3_cache_refill_num = entry->counter_value;
                }
            }

            col_cluster.push_back(L"all");
            col_cores.push_back(L"all");
            col_read_bandwith.push_back(DoubleToWideString(((double)(acc_l3_cache_access_num * 64)) / 1024.0 / 1024.0) + L"MB");
            col_miss_rate.push_back(DoubleToWideString(((double)(acc_l3_cache_refill_num)) / ((double)(acc_l3_cache_access_num)) * 100) + L"%");

            {
                TableOutputL table(m_outputType);
                table.PresetHeaders<L3CacheMetricOutputTraitsL>();
                table.SetAlignment(0, ColumnAlignL::RIGHT);
                table.SetAlignment(1, ColumnAlignL::RIGHT);
                table.SetAlignment(2, ColumnAlignL::RIGHT);
                table.SetAlignment(3, ColumnAlignL::RIGHT);
                table.Insert(col_cluster, col_cores, col_read_bandwith, col_miss_rate);
                m_globalJSON.m_DSUL3metric = table;
                m_out.Print(table);
            }
        }
    }

    void print_dmc_stat(std::vector<struct evt_noted>& clk_events, std::vector<struct evt_noted>& clkdiv2_events, bool report_ddr_bw_metric)
    {
        struct agg_entry
        {
            uint32_t event_idx;
            uint64_t counter_value;
        };

        std::unique_ptr<agg_entry[]> overall_clk, overall_clkdiv2;
        size_t clkdiv2_events_num = clkdiv2_events.size();
        size_t clk_events_num = clk_events.size();
        uint8_t ch_base = 0, ch_end = 0;

        if (dmc_idx == ALL_DMC_CHANNEL)
        {
            ch_base = 0;
            ch_end = (uint8_t)dmc_regions.size();
        }
        else
        {
            ch_base = dmc_idx;
            ch_end = dmc_idx + 1;
        }

        if (clk_events_num)
        {
            overall_clk = std::make_unique<agg_entry[]>(clk_events_num);
            memset(overall_clk.get(), 0, sizeof(agg_entry) * clk_events_num);

            for (uint32_t i = 0; i < clk_events_num; i++)
                overall_clk[i].event_idx = dmc_outs[ch_base].clk_events[i].event_idx;
        }

        if (clkdiv2_events_num)
        {
            overall_clkdiv2 = std::make_unique<agg_entry[]>(clkdiv2_events_num);
            memset(overall_clkdiv2.get(), 0, sizeof(agg_entry) * clkdiv2_events_num);

            for (uint32_t i = 0; i < clkdiv2_events_num; i++)
                overall_clkdiv2[i].event_idx = dmc_outs[ch_base].clkdiv2_events[i].event_idx;
        }

        std::vector<std::wstring> col_pmu_id, col_counter_value, col_event_name,
                                  col_event_idx, col_event_note;

        for (uint32_t i = ch_base; i < ch_end; i++)
        {
            int32_t evt_num = dmc_outs[i].clk_events_num;

            struct pmu_event_usr* evts = dmc_outs[i].clk_events;
            for (int j = 0; j < evt_num; j++)
            {
                struct pmu_event_usr* evt = &evts[j];

                if (timeline_mode)
                {
                    timeline_outfiles[EVT_DMC_CLK] << evt->value << L",";
                }
                else
                {
                    col_pmu_id.push_back(L"dmc " + std::to_wstring(i));
                    col_counter_value.push_back(std::to_wstring(evt->value));
                    col_event_name.push_back(get_event_name((uint16_t)evt->event_idx, EVT_DMC_CLK));
                    col_event_idx.push_back(IntToHexWideString(evt->event_idx));
                    col_event_note.push_back(clk_events[j].note);
                }

                if (overall_clk)
                    overall_clk[j].counter_value += evt->value;
            }

            evt_num = dmc_outs[i].clkdiv2_events_num;
            evts = dmc_outs[i].clkdiv2_events;
            for (int j = 0; j < evt_num; j++)
            {
                struct pmu_event_usr* evt = &evts[j];

                if (timeline_mode)
                {
                    timeline_outfiles[EVT_DMC_CLKDIV2] << evt->value << L",";
                }
                else
                {
                    col_pmu_id.push_back(L"dmc " + std::to_wstring(i));
                    col_counter_value.push_back(std::to_wstring(evt->value));
                    col_event_name.push_back(get_event_name((uint16_t)evt->event_idx, EVT_DMC_CLKDIV2));
                    col_event_idx.push_back(IntToHexWideString(evt->event_idx));
                    col_event_note.push_back(clkdiv2_events[j].note);
                }

                if (overall_clkdiv2)
                    overall_clkdiv2[j].counter_value += evt->value;
            }
        }

        for (size_t j = 0; j < clk_events_num; j++)
        {
            struct agg_entry* entry = overall_clk.get() + j;
            col_pmu_id.push_back(L"overall");
            col_counter_value.push_back(std::to_wstring(entry->counter_value));
            col_event_name.push_back(get_event_name((uint16_t)entry->event_idx, EVT_DMC_CLK));
            col_event_idx.push_back(IntToHexWideString(entry->event_idx));
            col_event_note.push_back(clk_events[j].note);
        }

        for (size_t j = 0; j < clkdiv2_events_num; j++)
        {
            struct agg_entry* entry = overall_clkdiv2.get() + j;
            col_pmu_id.push_back(L"overall");
            col_counter_value.push_back(std::to_wstring(entry->counter_value));
            col_event_name.push_back(get_event_name((uint16_t)entry->event_idx, EVT_DMC_CLKDIV2));
            col_event_idx.push_back(IntToHexWideString(entry->event_idx));
            col_event_note.push_back(clkdiv2_events[j].note);
        }

        if (!timeline_mode)
        {
            m_out.GetOutputStream() << std::endl;

            TableOutputL table(m_outputType);
            table.PresetHeaders<PMUPerformanceCounterOutputTraitsL>();
            table.SetAlignment(1, ColumnAlignL::RIGHT);
            table.Insert(col_pmu_id, col_counter_value, col_event_name, col_event_idx, col_event_note);

            m_globalJSON.m_pmu = table;
            m_out.Print(table);

        }

        if (timeline_mode)
        {
            if (clk_events_num)
                timeline_outfiles[EVT_DMC_CLK] << std::endl;
            if (clkdiv2_events_num)
                timeline_outfiles[EVT_DMC_CLKDIV2] << std::endl;
        }

        if (!overall_clk && !overall_clkdiv2)
        {
            if (!timeline_mode && report_ddr_bw_metric)
            {
                std::vector<std::wstring> col_channel, col_rw_bandwidth;

                m_out.GetOutputStream() << std::endl
                           << L"ddr metrics:" << std::endl;

                for (uint32_t i = ch_base; i < ch_end; i++)
                {
                    int32_t evt_num = dmc_outs[i].clkdiv2_events_num;
                    struct pmu_event_usr* evts = dmc_outs[i].clkdiv2_events;
                    uint64_t ddr_rd_num = 0;

                    for (int j = 0; j < evt_num; j++)
                    {
                        if (evts[j].event_idx == DMC_EVENT_RDWR)
                            ddr_rd_num = evts[j].value;
                    }

                    col_channel.push_back(std::to_wstring(i));
                    col_rw_bandwidth.push_back(DoubleToWideString(((double)(ddr_rd_num * 128)) / 1000.0 / 1000.0) + L"MB");
                }

                TableOutputL table(m_outputType);
                table.PresetHeaders<DDRMetricOutputTraitsL>();
                table.SetAlignment(0, ColumnAlignL::RIGHT);
                table.SetAlignment(1, ColumnAlignL::RIGHT);
                table.Insert(col_channel, col_rw_bandwidth);

                m_globalJSON.m_DMCDDDR = table;
                m_out.Print(table);

            }
            return;
        }

        if (timeline_mode)
            return;

        if (report_ddr_bw_metric)
        {
            m_out.GetOutputStream() << std::endl
                       << L"ddr metrics:" << std::endl;

            std::vector<std::wstring> col_channel, col_rw_bandwidth;

            for (uint32_t i = ch_base; i < ch_end; i++)
            {
                int32_t evt_num = dmc_outs[i].clkdiv2_events_num;
                struct pmu_event_usr* evts = dmc_outs[i].clkdiv2_events;
                uint64_t ddr_rd_num = 0;

                for (int j = 0; j < evt_num; j++)
                {
                    if (evts[j].event_idx == DMC_EVENT_RDWR)
                        ddr_rd_num = evts[j].value;
                }

                col_channel.push_back(std::to_wstring(i));
                col_rw_bandwidth.push_back(DoubleToWideString(((double)(ddr_rd_num * 128)) / 1000.0 / 1000.0) + L"MB");
            }

            uint64_t evt_num = dmc_outs[ch_base].clkdiv2_events_num;
            uint64_t ddr_rd_num = 0;

            for (int j = 0; j < evt_num; j++)
            {
                struct agg_entry* entry = overall_clkdiv2.get() + j;
                if (entry->event_idx == DMC_EVENT_RDWR)
                    ddr_rd_num = entry->counter_value;
            }

            col_channel.push_back(L"all");
            col_rw_bandwidth.push_back(DoubleToWideString(((double)(ddr_rd_num * 128)) / 1000.0 / 1000.0) + L"MB");

            TableOutputL table(m_outputType);
            table.PresetHeaders<DDRMetricOutputTraitsL>();
            table.SetAlignment(0, ColumnAlignL::RIGHT);
            table.SetAlignment(1, ColumnAlignL::RIGHT);
            table.Insert(col_channel, col_rw_bandwidth);

            m_globalJSON.m_DMCDDDR = table;
            m_out.Print(table);

        }
    }

    void do_list(const std::map<std::wstring, metric_desc> &metrics)
    {
        // Print pre-defined events
        {
            // Query for available events
            std::map<enum evt_class, std::vector<uint16_t>> events;
            events_query(events);
            std::vector<std::wstring> col_alias_name, col_raw_index, col_event_type;

            m_out.GetOutputStream() << std::endl
                       << L"List of pre-defined events (to be used in -e)"
                       << std::endl << std::endl;

            for (const auto &a : events)
            {
                const wchar_t* prefix = evt_name_prefix[a.first];
                for (auto b : a.second) {
                    col_alias_name.push_back(std::wstring(prefix) + std::wstring(get_event_name(b, a.first)));
                    col_raw_index.push_back(IntToHexWideString(b, 2));
                    col_event_type.push_back(L"[" + std::wstring(evt_class_name[a.first]) + std::wstring(L" PMU event") + L"]");
                }
            }

            TableOutputL table(m_outputType);
            table.PresetHeaders<PredefinedEventsOutputTraitsL>();
            table.SetAlignment(1, ColumnAlignL::RIGHT);
            table.Insert(col_alias_name, col_raw_index, col_event_type);

            m_globalListJSON.m_Events = table;
            m_out.Print(table);

        }

        // Print supported metrics
        {
            std::vector<std::wstring> col_metric, col_events;

            m_out.GetOutputStream() << std::endl
                       << L"List of supported metrics (to be used in -m)"
                       << std::endl << std::endl;

            for (const auto& [key, value] : metrics) {
                col_metric.push_back(key);
                col_events.push_back(value.raw_str);
            }

            TableOutputL table(m_outputType);
            table.PresetHeaders<MetricOutputTraitsL>();
            table.Insert(col_metric, col_events);

            m_globalListJSON.m_Metrics = table;
            m_out.Print(table);
        }

        m_out.Print(m_globalListJSON);
    }

    void do_test(uint32_t enable_bits,
                 std::map<enum evt_class, std::vector<struct evt_noted>>& ioctl_events)
    {
        std::vector<std::wstring> col_test_name, col_test_result;

        // Tests for request.ioctl_events
        col_test_name.push_back(L"request.ioctl_events [EVT_CORE]");
        col_test_result.push_back(enable_bits & CTL_FLAG_CORE ? L"True" : L"False");
        col_test_name.push_back(L"request.ioctl_events [EVT_DSU]");
        col_test_result.push_back(enable_bits & CTL_FLAG_DSU ? L"True" : L"False");
        col_test_name.push_back(L"request.ioctl_events [EVT_DMC_CLK/EVT_DMC_CLKDIV2]");
        col_test_result.push_back(enable_bits & CTL_FLAG_DMC ? L"True" : L"False");

        // Test for pmu_device.vendor_name
        col_test_name.push_back(L"pmu_device.vendor_name");
        col_test_result.push_back(vendor_name);

        // Tests for pmu_device.events_query(events)
        std::map<enum evt_class, std::vector<uint16_t>> events;
        events_query(events);
        col_test_name.push_back(L"pmu_device.events_query(events) [EVT_CORE]");
        if (events.count(EVT_CORE))
            col_test_result.push_back(std::to_wstring(events[EVT_CORE].size()));
        else
            col_test_result.push_back(L"0");
        col_test_name.push_back(L"pmu_device.events_query(events) [EVT_DSU]");
        if (events.count(EVT_DSU))
            col_test_result.push_back(std::to_wstring(events[EVT_DSU].size()));
        else
            col_test_result.push_back(L"0");
        col_test_name.push_back(L"pmu_device.events_query(events) [EVT_DMC_CLK]");
        if (events.count(EVT_DMC_CLK))
            col_test_result.push_back(std::to_wstring(events[EVT_DMC_CLK].size()));
        else
            col_test_result.push_back(L"0");
        col_test_name.push_back(L"pmu_device.events_query(events) [EVT_DMC_CLKDIV2]");
        if (events.count(EVT_DMC_CLKDIV2))
            col_test_result.push_back(std::to_wstring(events[EVT_DMC_CLKDIV2].size()));
        else
            col_test_result.push_back(L"0");

        // Tests for PMU_CTL_QUERY_HW_CFG
        struct hw_cfg hw_cfg;
        query_hw_cfg(hw_cfg);
        col_test_name.push_back(L"PMU_CTL_QUERY_HW_CFG [arch_id]");
        col_test_result.push_back(IntToHexWideString(hw_cfg.arch_id));
        col_test_name.push_back(L"PMU_CTL_QUERY_HW_CFG [core_num]");
        col_test_result.push_back(IntToHexWideString(hw_cfg.core_num));
        col_test_name.push_back(L"PMU_CTL_QUERY_HW_CFG [fpc_num]");
        col_test_result.push_back(IntToHexWideString(hw_cfg.fpc_num));
        col_test_name.push_back(L"PMU_CTL_QUERY_HW_CFG [gpc_num]");
        col_test_result.push_back(IntToHexWideString(hw_cfg.gpc_num));
        col_test_name.push_back(L"PMU_CTL_QUERY_HW_CFG [part_id]");
        col_test_result.push_back(IntToHexWideString(hw_cfg.part_id));
        col_test_name.push_back(L"PMU_CTL_QUERY_HW_CFG [pmu_ver]");
        col_test_result.push_back(IntToHexWideString(hw_cfg.pmu_ver));
        col_test_name.push_back(L"PMU_CTL_QUERY_HW_CFG [rev_id]");
        col_test_result.push_back(IntToHexWideString(hw_cfg.rev_id));
        col_test_name.push_back(L"PMU_CTL_QUERY_HW_CFG [variant_id]");
        col_test_result.push_back(IntToHexWideString(hw_cfg.variant_id));
        col_test_name.push_back(L"PMU_CTL_QUERY_HW_CFG [vendor_id]");
        col_test_result.push_back(IntToHexWideString(hw_cfg.vendor_id));

        // Tests for event scheduling
        col_test_name.push_back(L"gpc_nums[EVT_CORE]");
        col_test_result.push_back(std::to_wstring(gpc_nums[EVT_CORE]));
        col_test_name.push_back(L"gpc_nums[EVT_DSU]");
        col_test_result.push_back(std::to_wstring(gpc_nums[EVT_DSU]));
        col_test_name.push_back(L"gpc_nums[EVT_DMC_CLK]");
        col_test_result.push_back(std::to_wstring(gpc_nums[EVT_DMC_CLK]));
        col_test_name.push_back(L"gpc_nums[EVT_DMC_CLKDIV2]");
        col_test_result.push_back(std::to_wstring(gpc_nums[EVT_DMC_CLKDIV2]));
        std::wstring evt_indexes, evt_notes;
        for (auto e : ioctl_events[EVT_CORE])
        {
            evt_indexes += std::to_wstring(e.index) + L",";
            evt_notes += e.note + L",";
        }
        if (!evt_indexes.empty() && evt_indexes.back() == L',')
        {
            evt_indexes.pop_back();
        }
        if (!evt_notes.empty() && evt_notes.back() == L',')
        {
            evt_notes.pop_back();
        }
        col_test_name.push_back(L"ioctl_events[EVT_CORE].index");
        col_test_result.push_back(evt_indexes);
        col_test_name.push_back(L"ioctl_events[EVT_CORE].note");
        col_test_result.push_back(evt_notes);
        evt_indexes.clear();
        evt_notes.clear();
        for (auto e : ioctl_events[EVT_DSU])
        {
            evt_indexes += std::to_wstring(e.index) + L",";
            evt_notes += e.note + L",";
        }
        if (!evt_indexes.empty() && evt_indexes.back() == L',')
        {
            evt_indexes.pop_back();
        }
        if (!evt_notes.empty() && evt_notes.back() == L',')
        {
            evt_notes.pop_back();
        }
        col_test_name.push_back(L"ioctl_events[EVT_DSU].index");
        col_test_result.push_back(evt_indexes);
        col_test_name.push_back(L"ioctl_events[EVT_DSU].note");
        col_test_result.push_back(evt_notes);
        evt_indexes.clear();
        evt_notes.clear();
        for (auto e : ioctl_events[EVT_DMC_CLK])
        {
            evt_indexes += std::to_wstring(e.index) + L",";
            evt_notes += e.note + L",";
        }
        if (!evt_indexes.empty() && evt_indexes.back() == L',')
        {
            evt_indexes.pop_back();
        }
        if (!evt_notes.empty() && evt_notes.back() == L',')
        {
            evt_notes.pop_back();
        }
        col_test_name.push_back(L"ioctl_events[EVT_DMC_CLK].index");
        col_test_result.push_back(evt_indexes);
        col_test_name.push_back(L"ioctl_events[EVT_DMC_CLK].note");
        col_test_result.push_back(evt_notes);
        evt_indexes.clear();
        evt_notes.clear();
        for (auto e : ioctl_events[EVT_DMC_CLKDIV2])
        {
            evt_indexes += std::to_wstring(e.index) + L",";
            evt_notes += e.note + L",";
        }
        if (!evt_indexes.empty() && evt_indexes.back() == L',')
        {
            evt_indexes.pop_back();
        }
        if (!evt_notes.empty() && evt_notes.back() == L',')
        {
            evt_notes.pop_back();
        }
        col_test_name.push_back(L"ioctl_events[EVT_DMC_CLKDIV2].index");
        col_test_result.push_back(evt_indexes);
        col_test_name.push_back(L"ioctl_events[EVT_DMC_CLKDIV2].note");
        col_test_result.push_back(evt_notes);

        TableOutputL table(m_outputType);
        table.PresetHeaders<TestOutputTraitsL>();
        table.Insert(col_test_name, col_test_result);
        m_out.Print(table, true);
    }

    uint8_t core_num;
    std::map<std::wstring, metric_desc> builtin_metrics;
    bool has_dsu;
    bool has_dmc;
    uint32_t dsu_cluster_num;
    uint32_t dsu_cluster_size;
    uint32_t dmc_num;
    bool do_verbose;

private:
    /// <summary>
    /// This two dimentional array stores names of cores (DeviceDesc) for all (DSU clusers) x (cluster core no)
    ///
    /// You can access them like this:
    ///
    ///     [dsu_cluster_num][core_num_idx] -> WSTRING(DeviceDesc)
    ///
    /// Where (example for 80 core ARMv8 CPU:
    ///
    ///     dsu_cluster_num - number of DSU clusters, e.g. 0-39 (40 clusters)
    ///     core_num_idx    - index of core in DSU clusters, e.g. 0-1 (2 cores per cluster)
    ///
    /// You can print it like this:
    ///
    /// int i = 0;
    /// for (const auto& a : m_dsu_cluster_makeup)
    /// {
    ///     int j = 0;
    ///     for (const auto& b : a)
    ///     {
    ///         std::wcout << L"[" << i << L"]" << L"[" << j << L"]" << b << std::endl;
    ///         j++;
    ///     }
    ///     i++;
    /// }
    ///
    /// </summary>
    std::vector <std::vector < std::wstring >> m_dsu_cluster_makeup;

    bool all_cores_p() {
        return cores_idx.size() > 1;
    }

    bool detect_armh_dsu()
    {
        ULONG DeviceListLength = 0;
        CONFIGRET cr = CR_SUCCESS;
        DEVPROPTYPE PropertyType;
        PWSTR DeviceList = NULL;
        WCHAR DeviceDesc[512];
        PWSTR CurrentDevice;
        ULONG PropertySize;
        DEVINST Devinst;

        cr = CM_Get_Device_ID_List_Size(&DeviceListLength, NULL, CM_GETIDLIST_FILTER_PRESENT);
        if (cr != CR_SUCCESS)
        {
			warning(L"warning: detect uncore: failed CM_Get_Device_ID_List_Size, cancel unCore support");
            goto fail0;
        }

        DeviceList = (PWSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
            DeviceListLength * sizeof(WCHAR));
        if (!DeviceList)
        {
			warning(L"detect uncore: failed HeapAlloc for DeviceList, cancel unCore support");
            goto fail0;
        }

        cr = CM_Get_Device_ID_ListW(L"ACPI\\ARMHD500", DeviceList, DeviceListLength,
            CM_GETIDLIST_FILTER_PRESENT | CM_GETIDLIST_FILTER_ENUMERATOR);
        if (cr != CR_SUCCESS)
        {
			warning(L"warning: detect uncore: failed CM_Get_Device_ID_ListW, cancel unCore support");
            goto fail0;
        }

        dsu_cluster_num = 0;
        for (CurrentDevice = DeviceList; *CurrentDevice; CurrentDevice += wcslen(CurrentDevice) + 1, dsu_cluster_num++)
        {
            cr = CM_Locate_DevNodeW(&Devinst, CurrentDevice, CM_LOCATE_DEVNODE_NORMAL);
            if (cr != CR_SUCCESS)
            {
				warning(L"warning: detect uncore: failed CM_Locate_DevNodeW, cancel unCore support");
                goto fail0;
            }

            PropertySize = sizeof(DeviceDesc);
            cr = CM_Get_DevNode_PropertyW(Devinst, &DEVPKEY_Device_Siblings, &PropertyType,
                (PBYTE)DeviceDesc, &PropertySize, 0);
            if (cr != CR_SUCCESS)
            {
				warning(L"warning: detect uncore: failed CM_Get_DevNode_PropertyW, cancel unCore support");
                goto fail0;
            }

            std::vector < std::wstring > siblings;
            uint32_t core_num_idx = 0;
            for (PWSTR sibling = DeviceDesc; *sibling; sibling += wcslen(sibling) + 1, core_num_idx++)
                siblings.push_back(std::wstring(sibling));

            m_dsu_cluster_makeup.push_back(siblings);

            if (dsu_cluster_size)
            {
                if (core_num_idx != dsu_cluster_size)
                {
					warning(L"detect uncore: failed CM_Get_DevNode_PropertyW, cancel unCore support");
                    goto fail0;
                }
            }
            else
            {
                dsu_cluster_size = core_num_idx;
            }
        }

        if (!dsu_cluster_num)
        {
            warning(L"detect uncore: failed finding cluster, cancel unCore support");
            goto fail0;
        }

        if (!dsu_cluster_size)
        {
            warning(L"warning: detect uncore: failed finding core inside cluster, cancel unCore support");
            goto fail0;
        }

        return true;
    fail0:

        if (DeviceList)
            HeapFree(GetProcessHeap(), 0, DeviceList);

        return false;
    }

    bool detect_armh_dma()
    {
        ULONG DeviceListLength = 0;
        CONFIGRET cr = CR_SUCCESS;
        PWSTR DeviceList = NULL;
        PWSTR CurrentDevice;
        DEVINST Devinst;

        cr = CM_Get_Device_ID_List_Size(&DeviceListLength, NULL, CM_GETIDLIST_FILTER_PRESENT);
        if (cr != CR_SUCCESS)
            goto fail1;

        DeviceList = (PWSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
            DeviceListLength * sizeof(WCHAR));
        if (!DeviceList)
            goto fail1;

        cr = CM_Get_Device_ID_ListW(L"ACPI\\ARMHD620", DeviceList, DeviceListLength,
            CM_GETIDLIST_FILTER_PRESENT | CM_GETIDLIST_FILTER_ENUMERATOR);
        if (cr != CR_SUCCESS)
            goto fail1;

        for (CurrentDevice = DeviceList; *CurrentDevice; CurrentDevice += wcslen(CurrentDevice) + 1, dmc_num++)
        {
            cr = CM_Locate_DevNodeW(&Devinst, CurrentDevice, CM_LOCATE_DEVNODE_NORMAL);
            if (cr != CR_SUCCESS)
                goto fail1;

            LOG_CONF config;
            cr = CM_Get_First_Log_Conf(&config, Devinst, BOOT_LOG_CONF);
            if (cr != CR_SUCCESS)
                goto fail1;

            int log_conf_num = 0;
            do
            {
                RES_DES prev_resdes = (RES_DES)config;
                RES_DES resdes = 0;
                ULONG data_size;

                while (CM_Get_Next_Res_Des(&resdes, prev_resdes, ResType_Mem, NULL, 0) == CR_SUCCESS)
                {
                    if (prev_resdes != config)
                        CM_Free_Res_Des_Handle(prev_resdes);

                    prev_resdes = resdes;
                    if (CM_Get_Res_Des_Data_Size(&data_size, resdes, 0) != CR_SUCCESS)
                        continue;

                    std::unique_ptr<BYTE[]> resdes_data = std::make_unique<BYTE[]>(data_size);

                    if (CM_Get_Res_Des_Data(resdes, resdes_data.get(), data_size, 0) != CR_SUCCESS)
                        continue;

                    PMEM_RESOURCE mem_data = (PMEM_RESOURCE)resdes_data.get();
                    if (mem_data->MEM_Header.MD_Alloc_End - mem_data->MEM_Header.MD_Alloc_Base + 1)
                        dmc_regions.push_back(std::make_pair(mem_data->MEM_Header.MD_Alloc_Base, mem_data->MEM_Header.MD_Alloc_End));
                }

                LOG_CONF config2;
                cr = CM_Get_Next_Log_Conf(&config2, config, 0);
                CONFIGRET cr2 = CM_Free_Log_Conf_Handle(config);

                if (cr2 != CR_SUCCESS)
                    goto fail1;

                config = config2;
                log_conf_num++;

                if (log_conf_num > 1)
                    goto fail1;
            } while (cr != CR_NO_MORE_LOG_CONF);
        }

        if (!dmc_num)
            goto fail1;

        return !dmc_regions.empty();
    fail1:
        if (DeviceList)
            HeapFree(GetProcessHeap(), 0, DeviceList);

        return false;
    }

    void query_hw_cfg(struct hw_cfg& out)
    {
        struct hw_cfg buf;
        DWORD res_len;

        enum pmu_ctl_action action = PMU_CTL_QUERY_HW_CFG;
        BOOL status = DeviceAsyncIoControl(handle, &action, (DWORD)sizeof(enum pmu_ctl_action), &buf, (DWORD)sizeof(struct hw_cfg), &res_len);
        if (!status)
            throw fatal_exception("PMU_CTL_QUERY_HW_CFG failed");

        if (res_len != sizeof(struct hw_cfg))
            throw fatal_exception("PMU_CTL_QUERY_HW_CFG returned unexpected length of data");

        out = buf;
    }

    const wchar_t* get_vendor_name(uint8_t vendor_id)
    {
        if (arm64_vendor_names.count(vendor_id))
            return arm64_vendor_names[vendor_id];
        return L"Unknown Vendor";
    }

    // Use this function to print to wcerr runtime warnings in verbose mode.
    // Do not use this function for debug. Instead use WindowsPerfDbgPrint().
	void warning(const std::wstring wrn)
	{
		if (do_verbose)
			m_out.GetErrorOutputStream() << L"warning: " << wrn << std::endl;
	}

    HANDLE handle;
    uint32_t pmu_ver;
    const wchar_t* vendor_name;
    std::vector<uint8_t> cores_idx;                     // Cores
    std::set<uint32_t, std::less<uint32_t>> dsu_cores;  // DSU used by cores in 'cores_idx'
    uint8_t dmc_idx;
    std::unique_ptr<ReadOut[]> core_outs;
    std::unique_ptr<DSUReadOut[]> dsu_outs;
    std::unique_ptr<DMCReadOut[]> dmc_outs;
    bool multiplexings[EVT_CLASS_NUM];
    bool timeline_mode;
    bool count_kernel;
    uint32_t enc_bits;
    std::wofstream timeline_outfiles[EVT_CLASS_NUM];
    std::vector<std::pair<uint64_t, uint64_t>> dmc_regions;
};  // pmu_device

static bool no_ctrl_c = true;

static BOOL WINAPI ctrl_handler(DWORD dwCtrlType)
{
    switch (dwCtrlType)
    {
    case CTRL_C_EVENT:
        no_ctrl_c = false;
        m_out.GetOutputStream() << L"Ctrl-C received, quit counting...";
        return TRUE;
    case CTRL_BREAK_EVENT:
    default:
        m_out.GetErrorOutputStream() << L"unsupported dwCtrlType " << dwCtrlType << std::endl;
        return FALSE;
    }
}

static void print_help()
{
    const wchar_t* wsHelp = LR"(
usage: wperf [options]

    Options:
    list              List supported events and metrics.
    stat              Count events.If - e is not specified, then count default events.
    sample            Sample events. If -e is not specified, cycle counter will be the default sample source
    -e e1, e2...      Specify events to count.Event eN could be a symbolic name or in raw number.
                      Symbolic name should be what's listed by 'perf list', raw number should be rXXXX,
                      XXXX is hex value of the number without '0x' prefix.
                      when doing sampling, support -e e1:sample_freq1,e2:sample_freq2...
    -m m1, m2...      Specify metrics to count. 'imix', 'icache', 'dcache', 'itlb', 'dtlb' supported.
    -d N              Specify counting duration(in s).The accuracy is 0.1s.
    sleep N           Like -d, for compatibility with Linux perf.
    -i N              Specify counting interval(in s).To be used with -t.
    -t                Enable timeline mode.It specifies -i 60 -d 1 implicitly.
                      Means counting 1 second after every 60 second, and the result
                      is in.csv file in the same folder where wperf is invoked.
                      You can use -i and -d to change counting duration and interval.
    -image_name       Specify the image name you want to sample.
    -pe_file          Specify the PE file.
    -pdb_file         Specify the PDB file.
    -C config_file    Provide customized config file which describes metrics etc.
    -c core_idx       Profile on the specified core. Skip -c to count on all cores.
    -c cpu_list       Profile on the specified cores, 'cpu_list' is comma separated list e.g. '-c 0,1,2,3'.
    -dmc dmc_idx      Profile on the specified DDR controller. Skip -dmc to count on all DMCs.
    -k                Count kernel model as well (disabled by default).
    -h                Show tool help.
    --output          Enable JSON output to file.
    -q                Quiet mode, no output is produced.
    -json             Define output type as JSON.
    -l                Alias of 'list'.
    -verbose          Enable verbose output.
    -v                Alias of '-verbose'.
    -version          Show tool version.
)";

    m_out.GetOutputStream() << wsHelp << std::endl;
}

//
// Port End
//
int __cdecl
wmain(
    _In_ int argc,
    _In_reads_(argc) wchar_t* argv[]
    )
{
    HANDLE hDevice = INVALID_HANDLE_VALUE;
    auto exit_code = EXIT_SUCCESS;

    if (argc < 2)
    {
        print_help();
        return EXIT_SUCCESS;
    }

    if ( !GetDevicePath(
            (LPGUID) &GUID_DEVINTERFACE_WINDOWSPERF,
            G_DevicePath,
            sizeof(G_DevicePath)/sizeof(G_DevicePath[0])) )
    {
        WindowsPerfDbgPrint("Error: Failed to find device path. GetLastError=%d\n", GetLastError());
        return EXIT_FAILURE;
    }

    hDevice = CreateFile(G_DevicePath,
                         GENERIC_READ|GENERIC_WRITE,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         NULL,
                         OPEN_EXISTING,
                         0,
                         NULL );

    if (hDevice == INVALID_HANDLE_VALUE) {
        WindowsPerfDbgPrint("Error: Failed to open device. GetLastError=%d\n",GetLastError());
        return EXIT_FAILURE;
    }

    //
    // Port Begin
    //
    user_request request;
    pmu_device pmu_device;
    wstr_vec raw_args;

    try{
        pmu_device.init(hDevice);
    } catch(std::exception&) {
        exit_code = EXIT_FAILURE;
        goto clean_exit;
    }

    for (int i = 1; i < argc; i++)
        raw_args.push_back(argv[i]);

    try{
        request.init(raw_args, pmu_device.core_num, pmu_device.builtin_metrics);
        pmu_device.do_verbose = request.do_verbose;
    } catch(std::exception&)
    {
        exit_code = EXIT_FAILURE;
        goto clean_exit;
    }

    if (request.do_help)
    {
        print_help();
        goto clean_exit;
    }

    uint32_t enable_bits = 0;
    for (auto a : request.ioctl_events)
    {
        if (a.first == EVT_CORE)
        {
            enable_bits |= CTL_FLAG_CORE;
        }
        else if (a.first == EVT_DSU)
        {
            enable_bits |= CTL_FLAG_DSU;
        }
        else if (a.first == EVT_DMC_CLK || a.first == EVT_DMC_CLKDIV2)
        {
            enable_bits |= CTL_FLAG_DMC;
        }
        else
        {
            m_out.GetErrorOutputStream() << L"Unrecognized EVT_CLASS when mapping enable_bits: " << a.first << "\n";
            exit_code = EXIT_FAILURE;
            goto clean_exit;
        }
    }

    version_info driver_ver;
    pmu_device.version_query(driver_ver);

    if (request.do_version)
    {
        std::vector<std::wstring> col_component, col_version;
        col_component.push_back(L"wperf");
        col_version.push_back(std::to_wstring(MAJOR) + L"." +
                              std::to_wstring(MINOR) + L"." +
                              std::to_wstring(PATCH));
        col_component.push_back(L"wperf-driver");
        col_version.push_back(std::to_wstring(driver_ver.major) + L"." +
                              std::to_wstring(driver_ver.minor) + L"." +
                              std::to_wstring(driver_ver.patch));
        TableOutputL table(m_outputType);
        table.PresetHeaders<VersionOutputTraitsL>();
        table.Insert(col_component, col_version);
        m_out.Print(table, true);
        goto clean_exit;
    }

    if (driver_ver.major != MAJOR || driver_ver.minor != MINOR
        || driver_ver.patch != PATCH)
    {
        m_out.GetErrorOutputStream() << L"Version mismatch between wperf-driver and wperf.\n";
        m_out.GetErrorOutputStream() << L"wperf-driver version: " << driver_ver.major << "."
                   << driver_ver.minor << "." << driver_ver.patch << "\n";
        m_out.GetErrorOutputStream() << L"wperf version: " << MAJOR << "." << MINOR << "."
                   << PATCH << "\n";
        exit_code = EXIT_FAILURE;
        goto clean_exit;
    }

    if (request.do_test)
    {
        pmu_device.do_test(enable_bits, request.ioctl_events);
        goto clean_exit;
    }

    try
    {
        if (request.do_list)
        {
            pmu_device.do_list(request.metrics);
            goto clean_exit;
        }

        pmu_device.post_init(request.cores_idx, request.dmc_idx, request.do_timeline, enable_bits);

        if (request.do_count)
        {
            if (!request.has_events())
            {
                m_out.GetErrorOutputStream() << "no event specified\n";
                return -1;
            }
            else if (request.do_verbose)
            {
                request.show_events();
            }

            if (SetConsoleCtrlHandler(&ctrl_handler, TRUE) == FALSE)
                throw fatal_exception("SetConsoleCtrlHandler failed");

            uint32_t stop_bits = CTL_FLAG_CORE;
            if (pmu_device.has_dsu)
                stop_bits |= CTL_FLAG_DSU;
            if (pmu_device.has_dmc)
                stop_bits |= CTL_FLAG_DMC;

            pmu_device.stop(stop_bits);

            for (uint32_t core_idx : request.cores_idx)
                pmu_device.events_assign(core_idx, request.ioctl_events, request.do_kernel);

            int64_t counting_duration_iter = request.count_duration > 0 ?
                static_cast<int64_t>(request.count_duration * 10) : _I64_MAX;

            int64_t counting_interval_iter = request.count_interval > 0 ?
                static_cast<int64_t>(request.count_interval * 2) : 0;

            do
            {
                pmu_device.reset(enable_bits);

                SYSTEMTIME timestamp_a;
                GetSystemTime(&timestamp_a);

                pmu_device.start(enable_bits);

                m_out.GetOutputStream() << L"counting ... -";

                int progress_map_index = 0;
                wchar_t progress_map[] = { L'/', L'|', L'\\', L'-' };
                int64_t t_count1 = counting_duration_iter;

                while (t_count1 > 0 && no_ctrl_c)
                {
                    m_out.GetOutputStream() << L'\b' << progress_map[progress_map_index % 4];
                    t_count1--;
                    Sleep(100);
                    progress_map_index++;
                }
                m_out.GetOutputStream() << L'\b' << "done\n";

                pmu_device.stop(enable_bits);

                SYSTEMTIME timestamp_b;
                GetSystemTime(&timestamp_b);

                if (enable_bits & CTL_FLAG_CORE)
                {
                    pmu_device.core_events_read();
                    pmu_device.print_core_stat(request.ioctl_events[EVT_CORE]);
                }

                if (enable_bits & CTL_FLAG_DSU)
                {
                    pmu_device.dsu_events_read();
                    pmu_device.print_dsu_stat(request.ioctl_events[EVT_DSU], request.report_l3_cache_metric);
                }

                if (enable_bits & CTL_FLAG_DMC)
                {
                    pmu_device.dmc_events_read();
                    pmu_device.print_dmc_stat(request.ioctl_events[EVT_DMC_CLK], request.ioctl_events[EVT_DMC_CLKDIV2], request.report_ddr_bw_metric);
                }

                ULARGE_INTEGER li_a, li_b;
                FILETIME time_a, time_b;

                SystemTimeToFileTime(&timestamp_a, &time_a);
                SystemTimeToFileTime(&timestamp_b, &time_b);
                li_a.u.LowPart = time_a.dwLowDateTime;
                li_a.u.HighPart = time_a.dwHighDateTime;
                li_b.u.LowPart = time_b.dwLowDateTime;
                li_b.u.HighPart = time_b.dwHighDateTime;

                if (!request.do_timeline)
                {
                    double duration = (double)(li_b.QuadPart - li_a.QuadPart) / 10000000.0;
                    m_out.GetOutputStream() << std::endl;
                    m_out.GetOutputStream() << std::right << std::setw(20)
                        << duration << L" seconds time elapsed" << std::endl;
                    m_globalJSON.m_duration = duration;
                }
                else
                {
                    m_out.GetOutputStream() << L"sleeping ... -";
                    int64_t t_count2 = counting_interval_iter;
                    for (; t_count2 > 0 && no_ctrl_c; t_count2--)
                    {
                        m_out.GetOutputStream() << L'\b' << progress_map[t_count2 % 4];
                        Sleep(500);
                    }

                    m_out.GetOutputStream() << L'\b' << "done\n";
                }

                if (m_outputType == TableOutputL::JSON || m_outputType == TableOutputL::ALL)
                {
                    m_out.Print(m_globalJSON);
                }
            } while (request.do_timeline && no_ctrl_c);
        }
        else if (request.do_sample)
        {
            if (SetConsoleCtrlHandler(&ctrl_handler, TRUE) == FALSE)
                throw fatal_exception("SetConsoleCtrlHandler failed for sampling");

            if (request.sample_pe_file == L"")
                throw fatal_exception("PE file not specified");

            if (request.sample_pdb_file == L"")
                throw fatal_exception("PDB file not specified");

            std::vector<SectionDesc> sec_info;          // List of sections in executable
            std::vector<FuncSymDesc> sym_info;
            std::vector<std::wstring> sec_import;       // List of DLL imported by executable
            uint64_t static_entry_point, image_base;

            parse_pe_file(request.sample_pe_file, static_entry_point, image_base, sec_info, sec_import);
            parse_pdb_file(request.sample_pdb_file, sym_info, request.sample_display_short);

            uint32_t stop_bits = CTL_FLAG_CORE;

            pmu_device.stop(stop_bits);

            pmu_device.set_sample_src(request.ioctl_events_sample, request.do_kernel);

            UINT64 runtime_vaddr_delta = 0;

            std::map<std::wstring, PeFileMetaData> dll_metadata;        // [pe_name] -> PeFileMetaData
            std::map<std::wstring, ModuleMetaData> modules_metadata;    // [mod_name] -> ModuleMetaData

            if (request.sample_image_name == L"")
            {
                std::wcout << "no pid or process name specified, sample address are not de-ASLRed" << std::endl;
            }
            else
            {
                HMODULE hMods[1024];
                DWORD cbNeeded;
                DWORD pid = FindProcess(request.sample_image_name);
                HANDLE process_handle = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, 0, pid);

                if (EnumProcessModules(process_handle, hMods, sizeof(hMods), &cbNeeded))
                {
                    for (auto i = 0; i < (cbNeeded / sizeof(HMODULE)); i++)
                    {
                        TCHAR szModName[MAX_PATH];
                        TCHAR lpszBaseName[MAX_PATH];

                        std::wstring name;
                        if (GetModuleBaseNameW(process_handle, hMods[i], lpszBaseName, MAX_PATH))
                        {
                            name = lpszBaseName;
                            modules_metadata[name].mod_name = name;
                        }

                        // Get the full path to the module's file.
                        if (GetModuleFileNameEx(process_handle, hMods[i], szModName, sizeof(szModName) / sizeof(TCHAR)))
                        {
                            std::wstring mod_path = szModName;
                            modules_metadata[name].mod_path = mod_path;
                            modules_metadata[name].handle = hMods[i];
                        }
                    }
                }

                if (request.do_verbose)
                {
                    std::wcout << L"================================" << std::endl;
                    for (const auto& [key, value] : modules_metadata)
                        {
                            std::wcout << std::setw(32) << key
                                << std::setw(32) << IntToHexWideString((ULONGLONG)value.handle, 20)
                                << L"          " << value.mod_path << std::endl;
                        }
                }

                for (auto& [key, value] : modules_metadata)
                {
                    std::wstring pdb_path = gen_pdb_name(value.mod_path);
                    std::ifstream ifile(pdb_path);
                    if (ifile) {
                        PeFileMetaData pefile_metadata;
                        parse_pe_file(value.mod_path, pefile_metadata);
                        dll_metadata[value.mod_name] = pefile_metadata;

                        parse_pdb_file(pdb_path, value.sym_info, request.sample_display_short);
                        ifile.close();
                    }
                }

                if (request.do_verbose)
                {
                    std::wcout << L"================================" << std::endl;
                    for (const auto& [key, value] : dll_metadata)
                    {
                        std::wcout << std::setw(32) << key
                            << L"          " << value.pe_name << std::endl;

                        for (auto& sec : value.sec_info)
                        {
                            std::wcout << std::setw(32) << sec.name
                                << std::setw(32) << IntToHexWideString(sec.offset, 20)
                                << std::setw(32) << IntToHexWideString(sec.virtual_size)
                                << std::endl;
                        }
                    }
                }

                HMODULE module_handle = GetModule(process_handle, request.sample_image_name);

                MODULEINFO modinfo;
                bool ret = GetModuleInformation(process_handle, module_handle, &modinfo, sizeof(MODULEINFO));
                if (!ret)
                {
                    std::wcout << L"failed to query base address of '" << request.sample_image_name << L"'\n";
                }
                else
                {
                    runtime_vaddr_delta = (UINT64)modinfo.EntryPoint - (image_base + static_entry_point);
                    std::wcout << L"base address of '" << request.sample_image_name
                        << L"': 0x" << std::hex << (UINT64)modinfo.EntryPoint
                        << L", runtime delta: 0x" << runtime_vaddr_delta << std::endl;
                }

                CloseHandle(process_handle);
            }

            pmu_device.start_sample();
            std::wcout << L"sampling ...";

            std::vector<FrameChain> raw_samples;

            while (no_ctrl_c)
            {
                bool sample = pmu_device.get_sample(raw_samples);
                if (sample)
                    std::wcout << L".";
                else
                    std::wcout << L"e";
                Sleep(1000);
            }

            std::wcout << " done!" << std::endl;

            pmu_device.stop_sample();

            std::vector<SampleDesc> resolved_samples;

            // Search for symbols in image (executable)
            uint64_t sec_base = 0;
            for (auto& b : sym_info)
            {
                for (const auto &c : sec_info)
                {
                    if (c.idx == (b.sec_idx - 1))
                    {
                        sec_base = image_base + c.offset + runtime_vaddr_delta;
                        break;
                    }
                }
            }

            for (const auto &a : raw_samples)
            {
                bool found = false;
                SampleDesc sd;

                // Search in symbol table for image (executable)
                for (const auto& b : sym_info)
                {
                    if (a.pc >= (b.offset + sec_base) && a.pc < (b.offset + sec_base + b.size))
                    {
                        sd.name = b.name;
                        found = true;
                        break;
                    }
                }

                // Nothing was found in base images, let's search inside modules loaded with
                // images (such as DLLs).
                // Note: at this point:
                //  `dll_metadata` contains names of all modules loaded with image (executable)
                //  `modules_metadata` contains e.g. symbols of image modules loaded which had
                //                     PDB files present and we were able to load them.
                if (!found)
                {
                    sec_base = 0;

                    for (const auto& [key, value] : dll_metadata)
                    {
                        if (modules_metadata.count(key))
                        {
                            ModuleMetaData& mmd = modules_metadata[key];

                            for (auto& b : mmd.sym_info)
                                for (const auto& c : value.sec_info)
                                    if (c.idx == (b.sec_idx - 1))
                                    {
                                        sec_base = (UINT64)mmd.handle + c.offset;
                                        break;
                                    }

                            for (const auto& b : mmd.sym_info)
                                if (a.pc >= (b.offset + sec_base) && a.pc < (b.offset + sec_base + b.size))
                                {
                                    sd.name = b.name + L":" + key;
                                    found = true;
                                    break;
                                }
                        }
                    }
                }

                if (!found)
                    sd.name = L"unknown";

                for (uint32_t counter_idx = 0; counter_idx < 32; counter_idx++)
                {
                    if (!(a.ov_flags & (1i64 << (UINT64)counter_idx)))
                        continue;

                    bool inserted = false;
                    uint32_t event_src;
                    if (counter_idx == 31)
                        event_src = CYCLE_EVT_IDX;
                    else
                        event_src = request.ioctl_events_sample[counter_idx].index;
                    for (auto& c : resolved_samples)
                    {
                        if (c.name == sd.name && c.event_src == event_src)
                        {
                            c.freq++;
                            bool pc_found = false;
                            for (int i = 0; i < c.pc.size(); i++)
                            {
                                if (c.pc[i].first == a.pc)
                                {
                                    c.pc[i].second += 1;
                                    pc_found = true;
                                    break;
                                }
                            }

                            if (!pc_found)
                                c.pc.push_back(std::make_pair(a.pc, 1));

                            inserted = true;
                            break;
                        }
                    }

                    if (!inserted)
                    {
                        sd.freq = 1;
                        sd.event_src = event_src;
                        sd.pc.push_back(std::make_pair(a.pc, 1));
                        resolved_samples.push_back(sd);
                    }
                }
            }

            std::sort(resolved_samples.begin(), resolved_samples.end(), sort_samples);

            uint32_t prev_evt_src = 0;
            if(resolved_samples.size() > 0)
                prev_evt_src = resolved_samples[0].event_src;

            std::vector<uint64_t> total_samples;
            uint64_t acc = 0;
            for (const auto& a : resolved_samples)
            {
                if (a.event_src != prev_evt_src)
                {
                    prev_evt_src = a.event_src;
                    total_samples.push_back(acc);
                    acc = 0;
                }

                acc += a.freq;
            }
            total_samples.push_back(acc);

#if 0
            uint32_t idx = 0;
            int displayed_sample = 0;
            std::wcout << L"======================== sample results (top " << std::dec << request.sample_display_row << L") ========================\n";
            for (auto a : resolved_samples)
            {
                std::wcout << std::format(L"{:>5.2f}%  {:>8}  {:>8}  ", ((double)a.freq * 100 / (double)total_sample), a.event_src, a.freq) << a.name << std::endl;
                displayed_sample += a.freq;
                idx++;
                if (idx == request.sample_display_row)
                    break;
            }
            //std::wcout << std::format(L"{:>5.2f}%  {:>8}  ", ((double)displayed_sample * 100 / (double)total_sample), displayed_sample) << L"top " << std::dec << request.sample_display_row << L" in total" << std::endl;
#else
            int32_t group_idx = -1;
            prev_evt_src = CYCLE_EVT_IDX - 1;
            uint64_t printed_sample_num = 0, printed_sample_freq = 0;
            for (auto a : resolved_samples)
            {
                if (a.event_src != prev_evt_src)
                {
                    prev_evt_src = a.event_src;

                    if (printed_sample_num > 0 && printed_sample_num < request.sample_display_row)
                        //std::wcout << std::format(L"{:>6.2f}%  {:>8}  ", ((double)printed_sample_freq * 100 / (double)total_samples[group_idx]), printed_sample_freq) << L"top " << std::dec << printed_sample_num << L" in total" << std::endl;

                        std::wcout << DoubleToWideStringExt(((double)printed_sample_freq * 100 / (double)total_samples[group_idx]), 2, 6) << L"%"
                        << IntToDecWideString(printed_sample_freq, 10)
                        << L"  top " << std::dec << printed_sample_num << L" in total" << std::endl;

                    std::wcout << L"======================== sample source: " << get_event_name(static_cast<uint16_t>(a.event_src)) << L", top " << std::dec << request.sample_display_row << L" hot functions ========================\n";

                    printed_sample_num = 0;
                    printed_sample_freq = 0;
                    group_idx++;
                }

                if (printed_sample_num == request.sample_display_row)
                {
                    //std::wcout << std::format(L"{:>6.2f}%  {:>8}  ", ((double)printed_sample_freq * 100 / (double)total_samples[group_idx]), printed_sample_freq) << L"top " << std::dec << request.sample_display_row << L" in total" << std::endl;
                    std::wcout << DoubleToWideStringExt(((double)printed_sample_freq * 100 / (double)total_samples[group_idx]), 2, 6) << L"%"
                        << IntToDecWideString(printed_sample_freq, 10)
                        << L"  top " << std::dec << request.sample_display_row << L" in total" << std::endl;
                    printed_sample_num++;
                    continue;
                }

                if (printed_sample_num > request.sample_display_row)
                    continue;

                //std::wcout << std::format(L"{:>6.2f}%  {:>8}  ", ((double)a.freq * 100 / (double)total_samples[group_idx]), a.freq) << a.name << std::endl;
                std::wcout << DoubleToWideStringExt(((double)a.freq * 100 / (double)total_samples[group_idx]), 2, 6) << L"%"
                    << IntToDecWideString(a.freq, 10)
                    << L"  " << a.name << std::endl;

                if (request.do_verbose)
                {
                    std::sort(a.pc.begin(), a.pc.end(), sort_pcs);

                    for (int i = 0; i < 10 && i < a.pc.size(); i++)
                    {
                        std::wcout << L"                   " << IntToHexWideString(a.pc[i].first, 20) << L" " << IntToDecWideString(a.pc[i].second, 8) << std::endl;
                    }

                    printed_sample_freq += a.freq;
                    printed_sample_num++;
                }
            }

            if (printed_sample_num > 0 && printed_sample_num < request.sample_display_row)
                //std::wcout << std::format(L"{:>6.2f}%  {:>8}  ", ((double)printed_sample_freq * 100 / (double)total_samples[group_idx]), printed_sample_freq) << L"top " << std::dec << printed_sample_num << L" in total" << std::endl;
                std::wcout << DoubleToWideStringExt((double)printed_sample_freq * 100 / (double)total_samples[group_idx], 2, 6) << L"%"
                           << IntToDecWideString(printed_sample_freq, 10) << L"  top " << std::dec << printed_sample_num << L" in total" << std::endl;
#endif
        }
    }
	catch (fatal_exception& e)
	{
		std::cerr << e.what() << std::endl;
        exit_code = EXIT_FAILURE;
        goto clean_exit;
	}

    //
    // Port End
    //
clean_exit:
    if (hDevice != INVALID_HANDLE_VALUE) {
        CloseHandle(hDevice);
    }

    return exit_code;
}

BOOL
GetDevicePath(
    _In_ LPGUID InterfaceGuid,
    _Out_writes_(BufLen) PWCHAR DevicePath,
    _In_ size_t BufLen
    )
{
    CONFIGRET cr = CR_SUCCESS;
    PWSTR deviceInterfaceList = NULL;
    ULONG deviceInterfaceListLength = 0;
    PWSTR nextInterface;
    HRESULT hr = E_FAIL;
    BOOL bRet = TRUE;

    cr = CM_Get_Device_Interface_List_Size(
                &deviceInterfaceListLength,
                InterfaceGuid,
                NULL,
                CM_GET_DEVICE_INTERFACE_LIST_PRESENT);
    if (cr != CR_SUCCESS) {
        WindowsPerfDbgPrint("Error: 0x%x retrieving device interface list size.\n", cr);
        goto clean0;
    }

    if (deviceInterfaceListLength <= 1) {
        bRet = FALSE;
        WindowsPerfDbgPrint("Error: No active device interfaces found.\n"
                            "       Is the sample driver loaded?");
        goto clean0;
    }

    deviceInterfaceList = (PWSTR)malloc(deviceInterfaceListLength * sizeof(WCHAR));
    if (deviceInterfaceList == NULL) {
        WindowsPerfDbgPrint("Error: Allocating memory for device interface list.\n");
        goto clean0;
    }
    ZeroMemory(deviceInterfaceList, deviceInterfaceListLength * sizeof(WCHAR));

    cr = CM_Get_Device_Interface_List(
                InterfaceGuid,
                NULL,
                deviceInterfaceList,
                deviceInterfaceListLength,
                CM_GET_DEVICE_INTERFACE_LIST_PRESENT);
    if (cr != CR_SUCCESS) {
        WindowsPerfDbgPrint("Error: 0x%x retrieving device interface list.\n", cr);
        goto clean0;
    }

    nextInterface = deviceInterfaceList + wcslen(deviceInterfaceList) + 1;
    if (*nextInterface != UNICODE_NULL) {
        WindowsPerfDbgPrint("Warning: More than one device interface instance found. \n"
                            "         Selecting first matching device.\n\n");
    }

    hr = StringCchCopy(DevicePath, BufLen, deviceInterfaceList);
    if (FAILED(hr)) {
        bRet = FALSE;
        WindowsPerfDbgPrint("Error: StringCchCopy failed with HRESULT 0x%x", hr);
        goto clean0;
    }

clean0:
    if (deviceInterfaceList != NULL) {
        free(deviceInterfaceList);
    }
    if (CR_SUCCESS != cr) {
        bRet = FALSE;
    }

    return bRet;
}
