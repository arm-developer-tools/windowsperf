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

#include <algorithm>
#include <cwctype>
#include <iostream>
#include <cstdint>
#include <sstream>
#include <stdexcept>

#include "parsers.h"
#include "exception.h"
#include "pmu_device.h"
#include "events.h"
#include "output.h"
#include "utils.h"


static bool sort_ioctl_events_sample(const struct evt_sample_src& a, const struct evt_sample_src& b)
{
    return a.index < b.index;
}

static uint16_t get_raw_event_index(std::wstring event_num_str, int Base)
{
    uint16_t event_num;

    try
    {
        int val = std::stoi(event_num_str, NULL, Base);

        if (val > std::numeric_limits<uint16_t>::max())
            throw std::out_of_range("extra event value out of range");

        event_num = static_cast<uint16_t>(val);
    }
    catch (const std::invalid_argument& e)
    {
        m_out.GetErrorOutputStream() << e.what() << L" in " << event_num_str << std::endl;
        throw fatal_exception("ERROR_EXTRA");
    }
    catch (const std::out_of_range& e)
    {
        m_out.GetErrorOutputStream() << e.what() << L" in " << event_num_str << std::endl;
        throw fatal_exception("ERROR_EXTRA");
    }

    return event_num;
}

static bool is_raw_event(const std::wstring& event)
{
    return (event.length() > 1 &&
        event[0] == L'r' &&
        std::all_of(event.begin() + 1, event.end(), std::iswxdigit));
}

static void push_raw_extra_event(uint16_t raw_event, enum evt_class e_class)
{
    if (pmu_events::extra_events.count(e_class)
        && std::any_of(pmu_events::extra_events[e_class].begin(),
            pmu_events::extra_events[e_class].end(),
            [raw_event](const auto& e) { return e.hdr.num == raw_event; }))
        return;

    std::wstring name = L"r" + IntToHexWideStringNoPrefix(raw_event, 1);
    struct extra_event e = { e_class, raw_event, name };
    pmu_events::extra_events[e_class].push_back(e);
}

void parse_events_extra(std::wstring events_str, std::map<enum evt_class, std::vector<struct extra_event>>& events)
{
    std::wistringstream event_stream(events_str);
    std::wstring event;

    if (std::any_of(events_str.begin(), events_str.end(), iswspace))
    {
        m_out.GetErrorOutputStream() << L"whitespace not allowed in extra event string in '" << events_str << L"'" << std::endl;
        throw fatal_exception("ERROR_EXTRA");
    }

    if (events_str.empty())
    {
        m_out.GetErrorOutputStream() << L"nothing to parse!" << std::endl;
        throw fatal_exception("ERROR_EXTRA");
    }

    while (std::getline(event_stream, event, L','))
    {
        if (event.empty())
        {
            m_out.GetErrorOutputStream() << L"empty token in '" << events_str << L"'" << std::endl;
            throw fatal_exception("ERROR_EXTRA");
        }

        if (std::count_if(event.begin(), event.end(), [](auto& c) { return c == L':'; }) > 1)
        {
            m_out.GetErrorOutputStream() << L"too many delimeters in " << event << std::endl;
            throw fatal_exception("ERROR_EXTRA");
        }

        uint16_t event_num;
        size_t delim_pos = event.find(L":");
        std::wstring event_name, event_num_str;

        if (delim_pos == std::string::npos)
        {
            m_out.GetErrorOutputStream() << L"missing extra event delimeter ':' in " << event << std::endl;
            throw fatal_exception("ERROR_EXTRA");
        }
        else
        {
            event_name = event.substr(0, delim_pos);
            event_num_str = event.substr(delim_pos + 1);

            if (event_num_str.empty())
            {
                m_out.GetErrorOutputStream() << L"missing extra event value after ':' in " << event << std::endl;
                throw fatal_exception("ERROR_EXTRA");
            }

            event_num = get_raw_event_index(event_num_str, 16);
            
            enum evt_class e_class = EVT_CORE;

            for (int e = EVT_CLASS_FIRST + 1; e < EVT_CLASS_NUM; e++)
            {
                std::wstring prefix = pmu_events::get_evt_name_prefix(static_cast<enum evt_class>(e));
                if (CaseInsensitiveWStringStartsWith(event_name, prefix))
                {
                    e_class = pmu_events::get_event_class_from_prefix(WStringToLower(prefix));
                    break;
                }
            }

            if (std::any_of(events[e_class].begin(),
                events[e_class].end(),
                [event_num](const auto& e) { return e.hdr.num == event_num; }) == false)
                events[e_class].push_back( { e_class, event_num, WStringToLower(event_name)} );
        }
    }
}

/*
    Example string to parse:
    $ perf record -e arm_spe_0/branch_filter=1,jitter=1/ -- workload
    $ perf record -e arm_spe/branch_filter=1,jitter=1/ -- workload
    $ perf record -e spe/branch_filter=1,jitter=1/ -- workload
*/
bool parse_events_str_for_feat_spe(std::wstring events_str, std::map<std::wstring,uint32_t>& flags)
{
    // Detect arm_spe_0/*/      - where '*'
    if (events_str.size() >= std::wstring(L"arm_spe_0//").size()
        && events_str.rfind(L"arm_spe_0/", 0) == 0
        && events_str.back() == L'/')
    {
        std::wstring filters_str(events_str.begin() + std::wstring(L"arm_spe_0/").size(), events_str.end() - 1);    // Get '*'
        std::wistringstream filter_stream(filters_str);
        std::wstring filter;

        // "branch_filter=1,jitter=1"
        while (std::getline(filter_stream, filter, L','))
        {
            auto pos = filter.find(L'=');
            if (pos == std::string::npos)
            {
                m_out.GetErrorOutputStream() << L"incorrect SPE filter: " << L"'" << filter << L"' in " << filters_str << std::endl;
                throw fatal_exception("ERROR_SPE_FILTER");
            }

            std::wstring filter_name(filter.begin(), filter.begin() + pos);
            std::wstring filter_value(filter.begin() + pos + 1, filter.end());

            if (filter_name.empty())
            {
                m_out.GetErrorOutputStream() << L"incorrect SPE filter name: " << L"'" << filter << L"'" << std::endl;
                throw fatal_exception("ERROR_SPE_FILTER_NAME");
            }

            if (filter_value != L"0" && filter_value != L"1")
            {
                m_out.GetErrorOutputStream() << L"incorrect SPE filter value: " << L"'" << filter << L"'. 0 or 1 allowed" << std::endl;
                throw fatal_exception("ERROR_SPE_FILTER_VALUE");
            }

            flags[filter_name] = true ? filter_value == L"1" : false;
        }

        return true;
    }

    return false;
}

void parse_events_str_for_sample(std::wstring events_str, std::vector<struct evt_sample_src> &ioctl_events_sample,
    std::map<uint32_t, uint32_t>& sampling_inverval)
{
    std::wistringstream event_stream(events_str);
    std::wstring event;

    while (std::getline(event_stream, event, L','))
    {
        uint16_t raw_event;
        uint32_t interval = PARSE_INTERVAL_DEFAULT;
        size_t delim_pos = event.find(L":");
        std::wstring str1;

        if (delim_pos == std::string::npos)
        {
            str1 = event;
        }
        else
        {
            str1 = event.substr(0, delim_pos);
            std::wstring interval_str(event.substr(delim_pos + 1, std::string::npos));
            try
            {
                interval = std::stoul(interval_str, NULL, 0);
            }
            catch (std::invalid_argument const& ex)
            {
                m_out.GetErrorOutputStream() << L"event interval: " << interval_str << L" is invalid!" << std::endl;
                m_out.GetErrorOutputStream() << L"note: " << ex.what() << std::endl;
                throw fatal_exception("ERROR_EVENT_SAMPLE_INTERVAL");
            }
            catch (std::out_of_range const& ex)
            {
                m_out.GetErrorOutputStream() << L"event interval: " << interval_str << L" is out of range!" << std::endl;
                m_out.GetErrorOutputStream() << L"note: " << ex.what() << std::endl;
                throw fatal_exception("ERROR_EVENT_SAMPLE_INTERVAL");
            }
        }

        if (std::iswdigit(str1[0]))
        {
            raw_event = get_raw_event_index(str1, 0);
            push_raw_extra_event(raw_event, EVT_CORE);
        }
        else if (is_raw_event(str1))
        {
            raw_event = get_raw_event_index(str1.substr(1, std::string::npos), 16);
            push_raw_extra_event(raw_event, EVT_CORE);
        }
        else
        {
            int idx = pmu_events::get_event_index(str1);
            if (idx < 0)
            {
                m_out.GetErrorOutputStream() << L"unknown event name: " << str1 << std::endl;
                throw fatal_exception("ERROR_EVENT_SAMPLE");
            }

            raw_event = static_cast<uint16_t>(idx);
        }

        // convert to fixed cycle event to save one GPC
        if (raw_event == 0x11)
            ioctl_events_sample.push_back({ CYCLE_EVT_IDX, interval });
        else
            ioctl_events_sample.push_back({ raw_event, interval });

        sampling_inverval[raw_event] = interval;
    }

    std::sort(ioctl_events_sample.begin(), ioctl_events_sample.end(), sort_ioctl_events_sample);
}

void parse_events_str(std::wstring events_str,
    std::map<enum evt_class,
    std::deque<struct evt_noted>>& events,
    std::map<enum evt_class,
    std::vector<struct evt_noted>>& groups,
    std::wstring metric_name,
    const struct pmu_device_cfg& pmu_cfg)
{
    std::wistringstream event_stream(events_str);
    std::wstring event;
    uint16_t group_size = 0;

    while (std::getline(event_stream, event, L','))
    {
        bool push_group = false, push_group_last = false;
        enum evt_class e_class = EVT_CORE;
        uint16_t raw_event;

        for (auto e : { EVT_DSU, EVT_DMC_CLK, EVT_DMC_CLKDIV2 })
        {
            std::wstring prefix = pmu_events::get_evt_name_prefix(static_cast<enum evt_class>(e));
            if (CaseInsensitiveWStringStartsWith(event, prefix))
            {
                e_class = e;
                event.erase(0, prefix.size());
                break;
            }
        }

        const wchar_t* chars = event.c_str();

        if (std::iswdigit(chars[0]))
        {
            raw_event = static_cast<uint16_t>(wcstol(chars, NULL, 0));
            push_raw_extra_event(raw_event, e_class);

            if (group_size)
            {
                push_group = true;
                group_size++;
            }
        }
        else if (is_raw_event(event))
        {
            raw_event = get_raw_event_index(event.substr(1, std::string::npos), 16);
            push_raw_extra_event(raw_event, e_class);

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
                if (std::iswdigit(strip_str[0]))
                {
                    raw_event = get_raw_event_index(chars, 0);
                    push_raw_extra_event(raw_event, e_class);
                }
                else if (is_raw_event(strip_str))
                {
                    raw_event = get_raw_event_index(strip_str.substr(1, std::string::npos), 16);
                    push_raw_extra_event(raw_event, e_class);
                }
                else
                {
                    int idx = pmu_events::get_event_index(strip_str, e_class);

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
                    raw_event = get_raw_event_index(chars, 0);
                    push_raw_extra_event(raw_event, e_class);
                }
                else if (is_raw_event(event))
                {
                    raw_event = get_raw_event_index(event.substr(1, std::string::npos), 16);
                    push_raw_extra_event(raw_event, e_class);
                }
                else
                {
                    int idx = pmu_events::get_event_index(event, e_class);

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
                int idx = pmu_events::get_event_index(event, e_class);

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

        std::wstring note = metric_name;

        if (push_group)
        {
            // We know how many groups are now inserted fully by counting EVT_HDR marker which describes group in vector of events
            // See below how and why we insert EVT_HDR to groups[e_class].
            const auto cnt = std::count_if(groups[e_class].begin(), groups[e_class].end(),
                                              [](const evt_noted& e) { return e.type == EVT_HDR; });

            groups[e_class].push_back({ raw_event, note == L"" ? EVT_GROUPED : EVT_METRIC_GROUPED, note, static_cast<int>(cnt), metric_name });

            if (push_group_last)
            {
                if (group_size > pmu_cfg.gpc_nums[e_class])
                {
                    if (group_size > pmu_cfg.total_gpc_num)
                    {
                        m_out.GetErrorOutputStream() << L"event group size(" << int(group_size)
                            << ") exceeds number of hardware PMU counters("
                            << pmu_cfg.total_gpc_num << ")" << std::endl;
                        throw fatal_exception("ERROR_GROUP");
                    }

                    m_out.GetErrorOutputStream() << L"event group size(" << int(group_size)
                        << ") exceeds number of free hardware PMU counters("
                        << pmu_cfg.gpc_nums[e_class] << ") out of a total of (" << pmu_cfg.total_gpc_num << ")" << std::endl;
                    throw fatal_exception("ERROR_GROUP");
                }

                // Insert EVT_HDR in front of group we've just added. This entity
                // will store information about following group of events.
                auto it = groups[e_class].end();
                groups[e_class].insert(std::prev(it, group_size), { group_size, EVT_HDR, note, EVT_NOTED_NO_GROUP, metric_name });
                group_size = 0;
            }
        }
        else
        {
            if (note == L"")
                events[e_class].push_back({ raw_event, EVT_NORMAL, L"e", EVT_NOTED_NO_GROUP, metric_name });
            else
                events[e_class].push_back({ raw_event, EVT_METRIC_NORMAL, L"e," + note, EVT_NOTED_NO_GROUP, metric_name });
        }
    }
}
