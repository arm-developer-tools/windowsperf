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
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "wperf.h"
#include "debug.h"
#include "prettytable.h"
#include "wperf-common\public.h"
#include "wperf-common\macros.h"
#include "wperf-common\iorequest.h"
#include "utils.h"

//
// Port start
//

#include <exception>
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
            WindowsPerfDbgPrint("WriteFile failed: Error %d\n", GetLastError());
            return FALSE;
        }
    }

    if (lpOutBuffer != NULL)
    {
        if (!ReadFile(hDevice, lpOutBuffer, nOutBufferSize, lpBytesReturned, NULL))
        {
            WindowsPerfDbgPrint("ReadFile failed: Error %d\n", GetLastError());
            return FALSE;
        }
    }

    return TRUE;
}
#pragma warning(pop)

#pragma comment (lib, "cfgmgr32.lib")

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

class fatal_exception : public std::exception
{
public:
    fatal_exception(const char* msg) : exception_msg(msg) {}
    virtual const char* what() const throw() { return exception_msg; }
private:
    const char* exception_msg;
};

class user_request
{
public:
    user_request(wstr_vec& raw_args, std::map<std::wstring, metric_desc>& builtin_metrics)
        : do_list{ false }, do_count(false), do_kernel(false), do_timeline(false),
        do_sample(false), do_version(false), do_verbose(false),
        do_help(false), core_idx(_UI32_MAX), dmc_idx(_UI8_MAX), count_duration(-1.0),
        count_interval(-1.0), report_l3_cache_metric(false), report_ddr_bw_metric(false)
    {
        bool waiting_events = false;
        bool waiting_metrics = false;
        bool waiting_core_idx = false;
        bool waiting_dmc_idx = false;
        bool waiting_duration = false;
        bool waiting_interval = false;
        bool waiting_config = false;
        std::map<enum evt_class, std::deque<struct evt_noted>> events;
        std::map<enum evt_class, std::vector<struct evt_noted>> groups;

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
                parse_events_str(a, events, groups, L"");
                waiting_events = false;
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
                        std::wcerr << L"metric '" << metric << "' not supported" << std::endl;
                        if (metrics.size())
                        {
                            std::wcerr << L"supported metrics:" << std::endl;
                            for (const auto& [key, value] : metrics)
                                std::wcerr << L"  " << key << std::endl;
                        }
                        else
                        {
                            std::wcerr << L"no metric registered" << std::endl;
                        }

                        exit(-1);
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

            if (waiting_core_idx)
            {
                core_idx = _wtoi(a.c_str());
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

            if (a == L"-d" || a == L"sleep")
            {
                waiting_duration = true;
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

            std::wcout << L"warning: unexpected arg '" << a << L"' ignored\n";
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
                //auto event_size = a.second.size();
                enum evt_class e_class = a.first;

                for (auto e : a.second)
                    push_ioctl_normal_event(e_class, e);

                if (groups.find(e_class) != groups.end())
                    padding_ioctl_events(e_class);
            }
        }
    }

    bool has_events()
    {
        return !!ioctl_events.size();
    }

    void show_events()
    {
        std::wcout << L"events to be counted:" << std::endl;

        for (auto a : ioctl_events)
        {
            std::wcout << L"  " << std::setw(18) << evt_class_name[a.first] << L":";
            for (auto b : a.second)
                std::wcout << L" " << IntToHexWideString(b.index);
            std::wcout << std::endl;
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
            std::wcerr << L"open config file '" << config_name << "' failed" << std::endl;
            exit(-1);
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
                    std::wcerr << L"unrecognized config component '"
                        << component << L"'" << std::endl;
                    exit(-1);
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
    bool report_l3_cache_metric;
    bool report_ddr_bw_metric;
    uint32_t core_idx;
    uint8_t dmc_idx;
    double count_duration;
    double count_interval;
    std::map<enum evt_class, std::vector<struct evt_noted>> ioctl_events;
    std::map<std::wstring, metric_desc> metrics;
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
                        std::wcerr << L"nested group is not supported " << event << std::endl;
                        exit(-1);
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
                            std::wcerr << L"unknown event name: " << strip_str << std::endl;
                            exit(-1);
                        }

                        raw_event = static_cast<uint16_t>(idx);
                    }
                }
                else if (event.back() == L'}')
                {
                    if (!group_size)
                    {
                        std::wcerr << L"missing '{' for event group " << event << std::endl;
                        exit(-1);
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
                            std::wcerr << L"unknown event name: " << event << std::endl;
                            exit(-1);
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
                        std::wcerr << L"unknown event name: " << event << std::endl;
                        exit(-1);
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
                        std::wcerr << L"event group size(" << group_size
                            << ") exceeds number of hardware PMU counter("
                            << gpc_nums[e_class] << ")" << std::endl;
                        exit(-1);
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
    pmu_device(HANDLE hDevice) : handle(hDevice), count_kernel(false), has_dsu(false), dsu_cluster_num(0), dsu_cluster_size(0),
        has_dmc(false), dmc_num(0), enc_bits(0)
    {
        for (int e = EVT_CLASS_FIRST; e < EVT_CLASS_NUM; e++)
            multiplexings[e] = false;

        struct hw_cfg hw_cfg;
        query_hw_cfg(hw_cfg);

        core_num = hw_cfg.core_num;
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
                std::wcerr << L"DSU_CTL_INIT failed" << std::endl;
                exit(1);
            }

            if (res_len != sizeof(struct dsu_cfg))
            {
                std::wcerr << L"DSU_CTL_INIT returned unexpected length of data" << std::endl;
                exit(1);
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
                std::wcerr << L"DMC_CTL_INIT failed" << std::endl;
                exit(1);
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

    void post_init(uint32_t core_idx_init, uint32_t dmc_idx_init, bool timeline_mode_init, uint32_t enable_bits)
    {
        // post_init members
        core_idx = core_idx_init;
        dmc_idx = (uint8_t)dmc_idx_init;
        timeline_mode = timeline_mode_init;
        enc_bits = enable_bits;

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
            if (core_idx == ALL_CORE)
                snprintf(buf, sizeof(buf), "wperf_system_side_");
            else
                snprintf(buf, sizeof(buf), "wperf_core_%d_", core_idx);
            std::string prefix(buf);

            std::string suffix = MultiByteFromWideString(evt_class_name[e]);

            filename = prefix + filename + suffix + std::string(".csv");
            if (!length)
            {
                std::cout << "timeline output file name: " << filename << std::endl;
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
        ctl.core_idx = core_idx;
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
        ctl.core_idx = core_idx;
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
        ctl.core_idx = core_idx;
        ctl.dmc_idx = dmc_idx;
        ctl.flags = flags;
        BOOL status = DeviceAsyncIoControl(handle, &ctl, sizeof(struct pmu_ctl_hdr), NULL, 0, &res_len);
        if (!status)
            throw fatal_exception("PMU_CTL_RESET failed");
    }



    void events_assign(std::map<enum evt_class, std::vector<struct evt_noted>> events, bool include_kernel)
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
        ctl->filter_bits = include_kernel ? 0 : FILTER_BIT_EXCL_EL1;
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

    void core_events_read(void)
    {
        struct pmu_ctl_hdr ctl;
        DWORD res_len;

        ctl.action = PMU_CTL_READ_COUNTING;
        ctl.core_idx = core_idx;

        LPVOID out_buf = core_idx == ALL_CORE ? core_outs.get() : core_outs.get() + core_idx;
        size_t out_buf_len = core_idx == ALL_CORE ? (sizeof(ReadOut) * core_num) : sizeof(ReadOut);
        BOOL status = DeviceAsyncIoControl(handle, &ctl, (DWORD)sizeof(struct pmu_ctl_hdr), out_buf, (DWORD)out_buf_len, &res_len);
        if (!status)
            throw fatal_exception("PMU_CTL_READ_COUNTING failed");
    }

    void dsu_events_read(void)
    {
        struct pmu_ctl_hdr ctl;
        DWORD res_len;

        ctl.action = DSU_CTL_READ_COUNTING;
        ctl.core_idx = core_idx;

        LPVOID out_buf = core_idx == ALL_CORE ? dsu_outs.get() : dsu_outs.get() + core_idx/dsu_cluster_size;
        size_t out_buf_len = core_idx == ALL_CORE ? (sizeof(DSUReadOut) * dsu_cluster_num) : sizeof(DSUReadOut);
        BOOL status = DeviceAsyncIoControl(handle, &ctl, (DWORD)sizeof(struct pmu_ctl_hdr), out_buf, (DWORD)out_buf_len, &res_len);
        if (!status)
            throw fatal_exception("DSU_CTL_READ_COUNTING failed");
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

        uint32_t core_base, core_end;
        std::unique_ptr<agg_entry[]> overall;

        if (core_idx == ALL_CORE)
        {
            core_base = 0;
            core_end = core_num;

            overall = std::make_unique<agg_entry[]>(core_outs[core_base].evt_num);
            memset(overall.get(), 0, sizeof(agg_entry)* core_outs[core_base].evt_num);

            for (uint32_t i = 0; i < core_outs[core_base].evt_num; i++)
                overall[i].event_idx = core_outs[core_base].evts[i].event_idx;
        }
        else
        {
            core_base = core_idx;
            core_end = core_idx + 1;
        }

        for (uint32_t i = core_base; i < core_end; i++)
        {
            if (!timeline_mode)
            {
                std::wcout << std::endl
                           << L"Performance counter stats for core " << std::dec << i
                           << (multiplexing ? L", multiplexed" : L", no multiplexing")
                           << (count_kernel ? L", kernel mode excluded" : L", kernel mode included")
                           << L", on " << vendor_name << L" core implementation:"
                           << std::endl;

                std::wcout << L"note: 'e' - normal event, 'gN' - grouped event with group number N, "
                              L"metric name will be appended if 'e' or 'g' comes from it"
                              << std::endl
                              << std::endl;
            }

            int32_t evt_num = core_outs[i].evt_num;
            struct pmu_event_usr* evts = core_outs[i].evts;
            uint64_t round = core_outs[i].round;

            std::vector<std::wstring> col_counter_value, col_event_name, col_event_idx,
                                      col_multiplexed, col_scaled_value, col_event_note;

            for (int j = 0; j < evt_num; j++)
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
                PrettyTable ptable;

                if (multiplexing)
                {
                    ptable.AddColumn(L"counter value", col_counter_value, PrettyTable::RIGHT);
                    ptable.AddColumn(L"event name", col_event_name);
                    ptable.AddColumn(L"event idx", col_event_idx);
                    ptable.AddColumn(L"event note", col_event_note);
                    ptable.AddColumn(L"multiplexed", col_multiplexed, PrettyTable::RIGHT);
                    ptable.AddColumn(L"scaled value", col_scaled_value, PrettyTable::RIGHT);
                }
                else
                {
                    ptable.AddColumn(L"counter value", col_counter_value, PrettyTable::RIGHT);
                    ptable.AddColumn(L"event name", col_event_name);
                    ptable.AddColumn(L"event idx", col_event_idx);
                    ptable.AddColumn(L"event note", col_event_note);
                }

                ptable.Print();
            }
        }

        if (timeline_mode)
            timeline_outfiles[EVT_CORE] << std::endl;

        if (!overall)
            return;

        if (timeline_mode)
            return;

        std::wcout << std::endl
                   << L"System-wide Overall:" << std::endl;

        std::vector<std::wstring> col_counter_value, col_event_idx, col_event_name,
                                  col_scaled_value, col_event_note;

        int32_t evt_num = core_outs[core_base].evt_num;

        for (int j = 0; j < evt_num; j++)
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
            PrettyTable ptable;
            if (multiplexing)
            {
                ptable.AddColumn(L"counter value", col_counter_value, PrettyTable::RIGHT);
                ptable.AddColumn(L"event name", col_event_name);
                ptable.AddColumn(L"event idx", col_event_idx);
                ptable.AddColumn(L"event note", col_event_note);
                ptable.AddColumn(L"scaled value", col_scaled_value, PrettyTable::RIGHT);
            }
            else {
                ptable.AddColumn(L"counter value", col_counter_value, PrettyTable::RIGHT);
                ptable.AddColumn(L"event name", col_event_name);
                ptable.AddColumn(L"event idx", col_event_idx);
                ptable.AddColumn(L"event note", col_event_note);
            }

            ptable.Print();
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

        uint32_t core_base, core_end;
        std::unique_ptr<agg_entry[]> overall;

        if (core_idx == ALL_CORE)
        {
            core_base = 0;
            core_end = core_num;

            overall = std::make_unique<agg_entry[]>(dsu_outs[core_base].evt_num);
            memset(overall.get(), 0, sizeof(agg_entry)* dsu_outs[core_base].evt_num);

            for (uint32_t i = 0; i < dsu_outs[core_base].evt_num; i++)
                overall[i].event_idx = dsu_outs[core_base].evts[i].event_idx;
        }
        else
        {
            core_base = core_idx;
            core_end = core_idx + 1;
        }

        for (uint32_t i = core_base; i < core_end; i += dsu_cluster_size)
        {
            if (!timeline_mode)
            {
                std::wcout << std::endl
                           << L"Performance counter stats for DSU cluster "
                           << (i / dsu_cluster_size)
                           << (multiplexing ? L", multiplexed" : L", no multiplexing")
                           << L", on " << vendor_name
                           << L" core implementation:"
                           << std::endl;

                std::wcout << L"note: 'e' - normal event, 'gN' - grouped event with group number N, "
                              L"metric name will be appended if 'e' or 'g' comes from it"
                              << std::endl
                              << std::endl;
            }

            int32_t evt_num = dsu_outs[i / dsu_cluster_size].evt_num;
            struct pmu_event_usr* evts = dsu_outs[i / dsu_cluster_size].evts;
            uint64_t round = dsu_outs[i / dsu_cluster_size].round;

            std::vector<std::wstring> col_counter_value, col_event_idx,  col_event_name,
                                      col_multiplexed, col_scaled_value, col_event_note;

            for (int j = 0; j < evt_num; j++)
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
                PrettyTable ptable;
                if (multiplexing)
                {
                    ptable.AddColumn(L"counter value", col_counter_value, PrettyTable::RIGHT);
                    ptable.AddColumn(L"event name", col_event_name);
                    ptable.AddColumn(L"event idx", col_event_idx);
                    ptable.AddColumn(L"event note", col_event_note);
                    ptable.AddColumn(L"multiplexed", col_multiplexed, PrettyTable::RIGHT);
                    ptable.AddColumn(L"scaled value", col_scaled_value, PrettyTable::RIGHT);
                }
                else
                {
                    ptable.AddColumn(L"counter value", col_counter_value, PrettyTable::RIGHT);
                    ptable.AddColumn(L"event name", col_event_name);
                    ptable.AddColumn(L"event idx", col_event_idx);
                    ptable.AddColumn(L"event note", col_event_note);
                }

                ptable.Print();
            }
        }

        if (timeline_mode)
            timeline_outfiles[EVT_DSU] << std::endl;

        if (!overall)
        {
            if (!timeline_mode && report_l3_metric)
            {
                std::wcout << std::endl
                           << L"L3 cache metrics:" << std::endl;

                std::vector<std::wstring> col_cluster, col_read_bandwith, col_miss_rate;

                for (uint32_t i = core_base; i < core_end; i += dsu_cluster_size)
                {
                    int32_t evt_num = dsu_outs[i / dsu_cluster_size].evt_num;
                    struct pmu_event_usr* evts = dsu_outs[i / dsu_cluster_size].evts;
                    uint64_t l3_cache_access_num = 0, l3_cache_refill_num = 0;

                    for (int j = 0; j < evt_num; j++)
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

                    col_cluster.push_back(std::to_wstring(i / dsu_cluster_size));
                    col_read_bandwith.push_back(DoubleToWideString(((double)(l3_cache_access_num * 64)) / 1024.0 / 1024.0) + L"MB");
                    col_miss_rate.push_back(DoubleToWideString(((double)(l3_cache_refill_num)) / ((double)(l3_cache_access_num)) * 100) + L"%");
                }

                {
                    PrettyTable ptable;
                    ptable.AddColumn(L"cluster", col_cluster, PrettyTable::RIGHT);
                    ptable.AddColumn(L"read_bandwidth", col_read_bandwith, PrettyTable::RIGHT);
                    ptable.AddColumn(L"miss_rate", col_miss_rate, PrettyTable::RIGHT);
                    ptable.Print();
                }
            }

            return;
        }

        if (timeline_mode)
            return;

        std::wcout << std::endl;

        std::wcout << L"System-wide Overall:" << std::endl;

        int32_t evt_num = dsu_outs[core_base / dsu_cluster_size].evt_num;

        std::vector<std::wstring> col_counter_value, col_event_name, col_event_idx,
                                  col_event_note, col_scaled_value;

        for (int j = 0; j < evt_num; j++)
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
            PrettyTable ptable;
            if (multiplexing)
            {
                ptable.AddColumn(L"counter value", col_counter_value, PrettyTable::RIGHT);
                ptable.AddColumn(L"event name", col_event_name);
                ptable.AddColumn(L"event idx", col_event_idx);
                ptable.AddColumn(L"event note", col_event_note);
                ptable.AddColumn(L"scaled value", col_scaled_value, PrettyTable::RIGHT);
            }
            else
            {
                ptable.AddColumn(L"counter value", col_counter_value, PrettyTable::RIGHT);
                ptable.AddColumn(L"event name", col_event_name);
                ptable.AddColumn(L"event idx", col_event_idx);
                ptable.AddColumn(L"event note", col_event_note, PrettyTable::RIGHT);
            }
            ptable.Print();
        }

        if (report_l3_metric)
        {
            std::wcout << std::endl
                       << L"L3 cache metrics:" << std::endl;

            std::vector<std::wstring> col_cluster, col_read_bandwith, col_miss_rate;

            for (uint32_t i = core_base; i < core_end; i += dsu_cluster_size)
            {
                int32_t evt_num2 = dsu_outs[i / dsu_cluster_size].evt_num;
                struct pmu_event_usr* evts = dsu_outs[i / dsu_cluster_size].evts;
                uint64_t l3_cache_access_num = 0, l3_cache_refill_num = 0;

                for (int j = 0; j < evt_num2; j++)
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

                col_cluster.push_back(std::to_wstring(i / dsu_cluster_size));
                col_read_bandwith.push_back(DoubleToWideString(((double)(l3_cache_access_num * 64)) / 1024.0 / 1024.0) + L"MB");
                col_miss_rate.push_back(DoubleToWideString(((double)(l3_cache_refill_num)) / ((double)(l3_cache_access_num)) * 100) + L"%");
            }

            uint64_t evt_num2 = dsu_outs[core_base / dsu_cluster_size].evt_num;
            uint64_t acc_l3_cache_access_num = 0, acc_l3_cache_refill_num = 0;
            for (int j = 0; j < evt_num2; j++)
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
            col_read_bandwith.push_back(DoubleToWideString(((double)(acc_l3_cache_access_num * 64)) / 1024.0 / 1024.0) + L"MB");
            col_miss_rate.push_back(DoubleToWideString(((double)(acc_l3_cache_refill_num)) / ((double)(acc_l3_cache_access_num)) * 100) + L"%");

            {
                PrettyTable ptable;
                ptable.AddColumn(L"cluster", col_cluster, PrettyTable::RIGHT);
                ptable.AddColumn(L"read_bandwidth", col_read_bandwith, PrettyTable::RIGHT);
                ptable.AddColumn(L"miss_rate", col_miss_rate, PrettyTable::RIGHT);
                ptable.Print();
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
        auto clkdiv2_events_num = clkdiv2_events.size();
        auto clk_events_num = clk_events.size();
        uint8_t ch_base, ch_end;

        if (dmc_idx == ALL_DMC_CHANNEL)
        {
            ch_base = 0;
            ch_end = (uint8_t)dmc_regions.size();

            //auto clk_events_num = clk_events.size();  /* hides previous declaration. */

            if (clk_events_num)
            {
                overall_clk = std::make_unique<agg_entry[]>(clk_events_num);
                memset(overall_clk.get(), 0, sizeof(agg_entry)* clk_events_num);

                for (uint32_t i = 0; i < clk_events_num; i++)
                    overall_clk[i].event_idx = dmc_outs[ch_base].clk_events[i].event_idx;
            }

            if (clkdiv2_events_num)
            {
                overall_clkdiv2 = std::make_unique<agg_entry[]>(clkdiv2_events_num);
                memset(overall_clkdiv2.get(), 0, sizeof(agg_entry)* clkdiv2_events_num);

                for (uint32_t i = 0; i < clkdiv2_events_num; i++)
                    overall_clkdiv2[i].event_idx = dmc_outs[ch_base].clkdiv2_events[i].event_idx;
            }
        }
        else
        {
            ch_base = dmc_idx;
            ch_end = dmc_idx + 1;
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

        for (int j = 0; j < clk_events_num; j++)
        {
            struct agg_entry* entry = overall_clk.get() + j;
            col_pmu_id.push_back(L"overall");
            col_counter_value.push_back(std::to_wstring(entry->counter_value));
            col_event_name.push_back(get_event_name((uint16_t)entry->event_idx, EVT_DMC_CLK));
            col_event_idx.push_back(IntToHexWideString(entry->event_idx));
            col_event_note.push_back(clk_events[j].note);
        }

        for (int j = 0; j < clkdiv2_events_num; j++)
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
            std::wcout << std::endl;

            PrettyTable ptable;
            ptable.AddColumn(L"pmu id", col_pmu_id);
            ptable.AddColumn(L"counter value", col_counter_value, PrettyTable::RIGHT);
            ptable.AddColumn(L"event name", col_event_name);
            ptable.AddColumn(L"event idx", col_event_idx);
            ptable.AddColumn(L"event note", col_event_note);
            ptable.Print();
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

                std::wcout << std::endl
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

                PrettyTable ptable;
                ptable.AddColumn(L"channel", col_channel, PrettyTable::RIGHT);
                ptable.AddColumn(L"rw_bandwidth", col_rw_bandwidth, PrettyTable::RIGHT);
                ptable.Print();
            }
            return;
        }

        if (timeline_mode)
            return;

        if (report_ddr_bw_metric)
        {
            std::wcout << std::endl
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

            PrettyTable ptable;
            ptable.AddColumn(L"channel", col_channel, PrettyTable::RIGHT);
            ptable.AddColumn(L"rw_bandwidth", col_rw_bandwidth, PrettyTable::RIGHT);
            ptable.Print();
        }
    }

    uint32_t core_num;
    std::map<std::wstring, metric_desc> builtin_metrics;
    bool has_dsu;
    bool has_dmc;
    uint32_t dsu_cluster_num;
    uint32_t dsu_cluster_size;
    uint32_t dmc_num;
private:
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
            //std::wcout << "warning: detect uncore: failed CM_Get_Device_ID_List_Size, cancel unCore support" << std::endl;
            goto fail0;
        }

        DeviceList = (PWSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
            DeviceListLength * sizeof(WCHAR));
        if (!DeviceList)
        {
            //std::wcout << "warning: detect uncore: failed HeapAlloc for DeviceList, cancel unCore support" << std::endl;
            goto fail0;
        }

        cr = CM_Get_Device_ID_ListW(L"ACPI\\ARMHD500", DeviceList, DeviceListLength,
            CM_GETIDLIST_FILTER_PRESENT | CM_GETIDLIST_FILTER_ENUMERATOR);
        if (cr != CR_SUCCESS)
        {
            //std::wcout << "warning: detect uncore: failed CM_Get_Device_ID_ListW, cancel unCore support" << std::endl;
            goto fail0;
        }

        for (CurrentDevice = DeviceList; *CurrentDevice; CurrentDevice += wcslen(CurrentDevice) + 1, dsu_cluster_num++)
        {
            cr = CM_Locate_DevNodeW(&Devinst, CurrentDevice, CM_LOCATE_DEVNODE_NORMAL);
            if (cr != CR_SUCCESS)
            {
                //std::wcout << "warning: detect uncore: failed CM_Locate_DevNodeW, cancel unCore support" << std::endl;
                goto fail0;
            }

            PropertySize = sizeof(DeviceDesc);
            cr = CM_Get_DevNode_PropertyW(Devinst, &DEVPKEY_Device_Siblings, &PropertyType,
                (PBYTE)DeviceDesc, &PropertySize, 0);
            if (cr != CR_SUCCESS)
            {
                //std::wcout << "warning: detect uncore: failed CM_Get_DevNode_PropertyW, cancel unCore support" << std::endl;
                goto fail0;
            }

            uint32_t core_num_idx = 0;
            for (PWSTR sibling = DeviceDesc; *sibling; sibling += wcslen(sibling) + 1, core_num_idx++);

            if (dsu_cluster_size)
            {
                if (core_num_idx != dsu_cluster_size)
                {
                    //std::wcout << "warning: detect uncore: failed CM_Get_DevNode_PropertyW, cancel unCore support" << std::endl;
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
            //std::wcout << "warning: detect uncore: failed finding cluster, cancel unCore support" << std::endl;
            goto fail0;
        }

        if (!dsu_cluster_size)
        {
            //std::wcout << "warning: detect uncore: failed finding core inside cluster, cancel unCore support" << std::endl;
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
        //DEVPROPTYPE PropertyType;
        PWSTR DeviceList = NULL;
        //WCHAR DeviceDesc[512];
        PWSTR CurrentDevice;
        //ULONG PropertySize;
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
        return L"UnKnown";
    }

    HANDLE handle;
    uint32_t pmu_ver;
    const wchar_t* vendor_name;
    uint32_t core_idx;
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
        std::wcout << L"Ctrl-C received, quit counting...";
        return TRUE;
    case CTRL_BREAK_EVENT:
    default:
        std::wcerr << L"unsupported dwCtrlType " << dwCtrlType << std::endl;
        return FALSE;
    }
}

static void print_help()
{
    std::wcout << "Usage:\n";
    std::wcout << "  wperf [options]\n";
    std::wcout << "\n";
    std::wcout << "Options:\n";
    std::wcout << "  list                   List supported events and metrics\n";
    std::wcout << "  stat                   Count events. If -e is not specified, then count default events\n";
    std::wcout << "  -e e1,e2...            Specify events to count. Event eN could be a symbolic name or in raw number.\n";
    std::wcout << "                         Symbolic name should be what's listed by 'perf list', raw number should be rXXXX,\n";
    std::wcout << "                         XXXX is hex value of the number without '0x' prefix\n";
    std::wcout << "  -m m1,m2...            Specify metrics to count. \"imix\", \"icache\", \"dcache\", \"itlb\", \"dtlb\" supported\n";
    std::wcout << "  -d N                   Specify counting duration (in s). The accuracy is 0.1s\n";
    std::wcout << "  sleep N                Like -d, for compatibility with Linux perf\n";
    std::wcout << "  -i N                   Specify counting interval (in s). To be used with -t\n";
    std::wcout << "  -t                     Enable timeline mode. It specifies -i 60 -d 1 implicitly.\n";
    std::wcout << "                         Means counting 1 second after every 60 second, and the result\n";
    std::wcout << "                         is in .csv file in the same folder where wperf is invoked. \n";
    std::wcout << "                         You can use -i and -d to change counting duration and interval. \n";
    std::wcout << "  -C config_file         Provide customized config file which describes metrics etc.\n";
    std::wcout << "  -c core_idx            Profile on the specified core\n";
    std::wcout << "                         CORE_INDEX could be a number or 'ALL' (default if -c not specified)\n";
    std::wcout << "  -dmc dmc_idx           Profile on the specified DDR controller\n";
    std::wcout << "                         DMC_INDEX could be a number or 'ALL' (default if -dmc not specified)\n";
    std::wcout << "  -k                     Count kernel model as well (disabled at default)\n";
    std::wcout << "  -h                     Show tool help\n";
    std::wcout << "  -l                     Alias of 'list'\n";
    std::wcout << "  -verbose               Enable verbose output\n";
    std::wcout << "  -v                     Alias of '-verbose'\n";
    std::wcout << "  -version               Show tool version\n";
    std::wcout << "\n";
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

    if ( !GetDevicePath(
            (LPGUID) &GUID_DEVINTERFACE_WINDOWSPERF,
            G_DevicePath,
            sizeof(G_DevicePath)/sizeof(G_DevicePath[0])) )
    {
        return FALSE;
    }

    hDevice = CreateFile(G_DevicePath,
                         GENERIC_READ|GENERIC_WRITE,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         NULL,
                         OPEN_EXISTING,
                         0,
                         NULL );

    if (hDevice == INVALID_HANDLE_VALUE) {
        WindowsPerfDbgPrint("Failed to open device. Error %d\n",GetLastError());
        return FALSE;
    }

    //
    // Port Begin
    //

    if (argc < 2)
    {
        print_help();
        return 0;
    }

    pmu_device pmu_device(hDevice);

    wstr_vec raw_args;
    for (int i = 1; i < argc; i++)
        raw_args.push_back(argv[i]);

    user_request request(raw_args, pmu_device.builtin_metrics);

    if (request.do_help)
    {
        print_help();
        return 0;
    }

    if (request.do_version)
    {
        std::wcout << L"wperf version " << MAJOR << "." << MINOR << "."
                   << PATCH << "\n";
        return 0;
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
            std::wcout << L"Unrecognized EVT_CLASS when mapping enable_bits: " << a.first << "\n";
            return 0;
        }
    }

    version_info driver_ver;
    pmu_device.version_query(driver_ver);
    if (driver_ver.major != MAJOR || driver_ver.minor != MINOR
        || driver_ver.patch != PATCH)
    {
        std::wcerr << L"Version mismatch between wperf-driver and wperf.\n";
        std::wcerr << L"wperf-driver version: " << driver_ver.major << "."
                   << driver_ver.minor << "." << driver_ver.patch << "\n";
        std::wcerr << L"wperf version: " << MAJOR << "." << MINOR << "."
                   << PATCH << "\n";
        return -1;
    }

    try
    {
        if (request.do_list)
        {
            // Print pre-defined events
            {
                // Query for available events
                std::map<enum evt_class, std::vector<uint16_t>> events;
                pmu_device.events_query(events);

                PrettyTable ptable;
                std::vector<std::wstring> col_alias_name, col_raw_index, col_event_type;

                std::wcout << std::endl
                           << L"List of pre-defined events (to be used in -e)"
                           << std::endl << std::endl;

                for (auto a : events)
                {
                    const wchar_t* prefix = evt_name_prefix[a.first];
                    for (auto b : a.second) {
                        col_alias_name.push_back(std::wstring(prefix) + std::wstring(get_event_name(b, a.first)));
                        col_raw_index.push_back(IntToHexWideString(b, 2));
                        col_event_type.push_back(L"[" + std::wstring(evt_class_name[a.first]) + std::wstring(L" PMU event") + L"]");
                    }
                }

                ptable.AddColumn(L"Alias Name", col_alias_name);
                ptable.AddColumn(L"Raw Index", col_raw_index, PrettyTable::RIGHT);
                ptable.AddColumn(L"Event Type", col_event_type);
                ptable.Print();
            }

            // Print supported metrics
            {
                PrettyTable ptable;
                std::vector<std::wstring> col_metric, col_events;

                std::wcout << std::endl
                           << L"List of supported metrics (to be used in -m)"
                           << std::endl << std::endl;

                for (const auto& [key, value] : request.metrics) {
                    col_metric.push_back(key);
                    col_events.push_back(value.raw_str);
                }
                ptable.AddColumn(L"Metric", col_metric);
                ptable.AddColumn(L"Events", col_events);
                ptable.Print();
            }
            return 0;
        }

        pmu_device.post_init(request.core_idx, request.dmc_idx, request.do_timeline, enable_bits);

        if (request.do_count)
        {
            if (!request.has_events())
            {
                std::wcerr << "no event specified\n";
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

            pmu_device.events_assign(request.ioctl_events, request.do_kernel);

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

                std::wcout << L"counting ... -";

                int progress_map_index = 0;
                wchar_t progress_map[] = { L'/', L'|', L'\\', L'-' };
                int64_t t_count1 = counting_duration_iter;

                while (t_count1 > 0 && no_ctrl_c)
                {
                    std::wcout << L'\b' << progress_map[progress_map_index % 4];
                    t_count1--;
                    Sleep(100);
                    progress_map_index++;
                }
                std::wcout << L'\b' << "done\n";

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
                    std::wcout << std::endl;
                    std::wcout << std::right << std::setw(20)
                        << (double)(li_b.QuadPart - li_a.QuadPart) / 10000000.0
                        << L" seconds time elapsed" << std::endl;
                }
                else
                {
                    std::wcout << L"sleeping ... -";
                    int64_t t_count2 = counting_interval_iter;
                    for (; t_count2 > 0 && no_ctrl_c; t_count2--)
                    {
                        std::wcout << L'\b' << progress_map[t_count2 % 4];
                        Sleep(500);
                    }

                    std::wcout << L'\b' << "done\n";
                }

            } while (request.do_timeline && no_ctrl_c);
        }
    }
	catch (fatal_exception& e)
	{
		std::cerr << e.what() << std::endl;
		return -1;
	}

    //
    // Port End
    //

    if (hDevice != INVALID_HANDLE_VALUE) {
        CloseHandle(hDevice);
    }

    return 0;
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
        WindowsPerfDbgPrint("Error 0x%x retrieving device interface list size.\n", cr);
        goto clean0;
    }

    if (deviceInterfaceListLength <= 1) {
        bRet = FALSE;
        WindowsPerfDbgPrint("Error: No active device interfaces found.\n"
            " Is the sample driver loaded?");
        goto clean0;
    }

    deviceInterfaceList = (PWSTR)malloc(deviceInterfaceListLength * sizeof(WCHAR));
    if (deviceInterfaceList == NULL) {
        WindowsPerfDbgPrint("Error allocating memory for device interface list.\n");
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
        WindowsPerfDbgPrint("Error 0x%x retrieving device interface list.\n", cr);
        goto clean0;
    }

    nextInterface = deviceInterfaceList + wcslen(deviceInterfaceList) + 1;
    if (*nextInterface != UNICODE_NULL) {
        WindowsPerfDbgPrint("Warning: More than one device interface instance found. \n"
            "Selecting first matching device.\n\n");
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
