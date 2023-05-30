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

#include <windows.h>
#include <algorithm>
#include <map>
#include <string>
#include "events.h"
#include "utils.h"
#include "wperf-common/iorequest.h"


namespace pmu_events
{
    std::map<enum evt_class, std::vector<struct extra_event>> extra_events;

    wchar_t* evt_class_name[EVT_CLASS_NUM] =
    {
        L"core",
        L"dsu",
        L"dmc_clk",
        L"dmc_clkdiv2",
    };

    const wchar_t* evt_name_prefix[EVT_CLASS_NUM] =
    {
        L"",
        L"/dsu/",
        L"/dmc_clk/",
        L"/dmc_clkdiv2/",
    };
}

const wchar_t* pmu_events::get_evt_class_name(enum evt_class e_class)
{
    return evt_class_name[e_class];
}

const wchar_t* pmu_events::get_evt_name_prefix(enum evt_class e_class)
{
    return evt_name_prefix[e_class];
}

enum evt_class pmu_events::get_event_class_from_prefix(std::wstring prefix)
{
    if (CaseInsensitiveWStringComparision(prefix, L"/dsu/")) return EVT_DSU;
    if (CaseInsensitiveWStringComparision(prefix, L"/dmc_clk/")) return EVT_DMC_CLK;
    if (CaseInsensitiveWStringComparision(prefix, L"/dmc_clkdiv2/")) return EVT_DMC_CLKDIV2;
    return EVT_CLASS_NUM;
}

wchar_t* pmu_events::get_dmc_clk_event_name(uint16_t index)
{
    switch (index)
    {
#define WPERF_DMC_CLK_EVENTS(a,b,c) case b: return L##c;
#include "wperf-common\dmc-clk-events.def"
#undef WPERF_DMC_CLK_EVENTS
    default: return L"unknown event";
    }
}

const wchar_t* pmu_events::get_dmc_clkdiv2_event_name(uint16_t index)
{
    switch (index)
    {
#define WPERF_DMC_CLKDIV2_EVENTS(a,b,c) case b: return L##c;
#include "wperf-common\dmc-clkdiv2-events.def"
#undef WPERF_DMC_CLKDIV2_EVENTS
    default: return L"unknown event";
    }
}

const wchar_t* pmu_events::get_core_event_name(uint16_t index)
{
    switch (index)
    {
#define WPERF_ARMV8_ARCH_EVENTS(n,a,b,c,d) case b: return L##c;
#include "wperf-common\armv8-arch-events.def"
#undef WPERF_ARMV8_ARCH_EVENTS
    case 0xFFFF: return L"cycle";
    default: return L"unknown event";
    }
}

const wchar_t* pmu_events::get_event_name(uint16_t index, enum evt_class e_class)
{
    const wchar_t* result = get_extra_event_name(index, e_class);
    if (result == nullptr)
        result = get_builtin_event_name(index, e_class);

    return result;
}

const wchar_t* pmu_events::get_builtin_event_name(uint16_t index, enum evt_class e_class)
{
    if (e_class == EVT_DMC_CLK)
        return get_dmc_clk_event_name(index);

    if (e_class == EVT_DMC_CLKDIV2)
        return get_dmc_clkdiv2_event_name(index);

    return get_core_event_name(index);
}

const wchar_t* pmu_events::get_extra_event_name(uint16_t index, enum evt_class e_class)
{
    if (extra_events.count(e_class))
    {
        auto it = std::find_if(extra_events[e_class].begin(), extra_events[e_class].end(),
            [index](const auto& e) { return e.hdr.num == index; });
        if (it != extra_events[e_class].end())
        {
            return (*it).name.c_str();
        }
    }

    return nullptr;
}

int pmu_events::get_core_event_index(std::wstring name)
{
#define WPERF_ARMV8_ARCH_EVENTS(n,a,b,c,d) if (CaseInsensitiveWStringComparision(name, std::wstring(L##c))) return b;
#include "wperf-common\armv8-arch-events.def"
#undef WPERF_ARMV8_ARCH_EVENTS
    return -1;
}

int pmu_events::get_dmc_clk_event_index(std::wstring name)
{
#define WPERF_DMC_CLK_EVENTS(a,b,c) if (CaseInsensitiveWStringComparision(name, std::wstring(L##c))) return b;
#include "wperf-common\dmc-clk-events.def"
#undef WPERF_DMC_CLK_EVENTS
    return -1;
}

int pmu_events::get_dmc_clkdiv2_event_index(std::wstring name)
{
#define WPERF_DMC_CLKDIV2_EVENTS(a,b,c) if (CaseInsensitiveWStringComparision(name, std::wstring(L##c))) return b;
#include "wperf-common\dmc-clkdiv2-events.def"
#undef WPERF_DMC_CLKDIV2_EVENTS
    return -1;
}

int pmu_events::get_event_index(std::wstring name, enum evt_class e_class)
{
    int result = get_extra_event_index(name, e_class);
    if (result < 0)
        result = get_builtin_event_index(name, e_class);

    return result;
}

int pmu_events::get_builtin_event_index(std::wstring name, enum evt_class e_class)
{
    if (e_class == EVT_DMC_CLK)
        return get_dmc_clk_event_index(name);

    if (e_class == EVT_DMC_CLKDIV2)
        return get_dmc_clkdiv2_event_index(name);

    return get_core_event_index(name);
}

int pmu_events::get_extra_event_index(std::wstring name, enum evt_class e_class)
{
    if (extra_events.count(e_class))
    {
        auto it = std::find_if(extra_events[e_class].begin(), extra_events[e_class].end(),
            [name](const auto& e) { return e.name == name; });
        if (it != extra_events[e_class].end())
        {
            return static_cast<int>((*it).hdr.num);
        }
    }
    return -1;
}
