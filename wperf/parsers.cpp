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

#include <cwctype>
#include <iostream>
#include <sstream>

#include "parsers.h"
#include "exception.h"
#include "pmu_device.h"
#include "events.h"
#include "output.h"


void parse_events_str_for_sample(std::wstring events_str, std::vector<struct evt_sample_src> &ioctl_events_sample,
    std::map<uint32_t, uint32_t>& sampling_inverval)
{
    std::wistringstream event_stream(events_str);
    std::wstring event;

    while (std::getline(event_stream, event, L','))
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
                m_out.GetErrorOutputStream() << L"unknown event name: " << str1 << std::endl;
                throw fatal_exception("ERROR_EVENT_SAMPLE");
            }

            raw_event = static_cast<uint32_t>(idx);
        }

        // convert to fixed cycle event to save one GPC
        if (raw_event == 0x11)
            raw_event = CYCLE_EVT_IDX;

        struct evt_sample_src _evt = { raw_event, interval };
        ioctl_events_sample.push_back(_evt);
        sampling_inverval[raw_event] = interval;
    }
}

void parse_events_str(std::wstring events_str,
    std::map<enum evt_class,
    std::deque<struct evt_noted>>& events,
    std::map<enum evt_class,
    std::vector<struct evt_noted>>& groups,
    std::wstring note,
    const struct pmu_device_cfg& pmu_cfg)
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
                if (group_size > pmu_cfg.gpc_nums[e_class])
                {
                    m_out.GetErrorOutputStream() << L"event group size(" << group_size
                        << ") exceeds number of hardware PMU counter("
                        << pmu_cfg.gpc_nums[e_class] << ")" << std::endl;
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
