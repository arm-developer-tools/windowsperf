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

#include <algorithm>
#include "padding.h"
#include "metric.h"
#include "pmu_device.h"
#include "events.h"


void set_event_padding(std::map<enum evt_class, std::vector<struct evt_noted>>& ioctl_events,
    const struct pmu_device_cfg& pmu_cfg,
    std::map<enum evt_class, std::deque<struct evt_noted>>& events,
    std::map<enum evt_class, std::vector<struct evt_noted>>& groups)
{
    if (groups.size())
    {
        for (const auto& a : groups)
        {
            auto total_size = a.second.size();
            enum evt_class e_class = a.first;
            const uint8_t GPC_NUMS = pmu_cfg.gpc_nums[e_class];

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
                // Check if there is enough continous padding to fit this group behind last group
                size_t padding_count = 0;
                auto insert_pos = ioctl_events[e_class].rbegin();

                if (ioctl_events[e_class].size())
                {
                    auto pos = ioctl_events[e_class].rbegin();
                    while ((*pos).type == EVT_PADDING)
                    {
                        padding_count++;
                        insert_pos = pos++;
                    }
                }

                if (padding_count >= group_size)
                {
                    for (uint16_t elem_idx = group_start; elem_idx < (group_start + group_size); elem_idx++)
                    {
                        struct evt_noted event = a.second[elem_idx];
                        event.note = set_event_note(a.second[elem_idx], group_num);
                        *insert_pos = event;

                        if (insert_pos != ioctl_events[e_class].rbegin())
                            insert_pos--;
                    }
                }
                else
                {
                    for (uint16_t elem_idx = group_start; elem_idx < (group_start + group_size); elem_idx++)
                        push_ioctl_grouped_event(ioctl_events, e_class, a.second[elem_idx], group_num);
                }

                padding_ioctl_events(ioctl_events, e_class, GPC_NUMS, events[e_class]);

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
        for (const auto& a : events)
        {
            enum evt_class e_class = a.first;

            for (const auto& e : a.second)
                push_ioctl_normal_event(ioctl_events, e_class, e);

            if (groups.find(e_class) != groups.end())
                padding_ioctl_events(ioctl_events, e_class, pmu_cfg.gpc_nums[e_class]);
        }
    }
}

void padding_ioctl_events(std::map<enum evt_class, std::vector<struct evt_noted>>& ioctl_events,
    enum evt_class e_class, uint8_t gpc_num, std::deque<struct evt_noted>& padding_vector)
{
    auto event_num = ioctl_events[e_class].size();

    if (!(event_num % gpc_num))
        return;

    auto event_num_after_padding = (event_num + gpc_num - 1) / gpc_num * gpc_num;
    for (auto idx = 0; idx < (event_num_after_padding - event_num); idx++)
    {
        if (padding_vector.size())
        {
            struct evt_noted e = padding_vector.front();
            padding_vector.pop_front();
            push_ioctl_normal_event(ioctl_events, e_class, e);
        }
        else
        {
            push_ioctl_padding_event(ioctl_events, e_class, PMU_EVENT_INST_SPEC);
        }
    }
}

void padding_ioctl_events(std::map<enum evt_class, std::vector<struct evt_noted>>& ioctl_events,
    enum evt_class e_class, uint8_t gpc_num)
{
    auto event_num = ioctl_events[e_class].size();

    if (!(event_num % gpc_num))
        return;

    auto event_num_after_padding = (event_num + gpc_num - 1) / gpc_num * gpc_num;
    for (auto idx = 0; idx < (event_num_after_padding - event_num); idx++)
        push_ioctl_padding_event(ioctl_events, e_class, PMU_EVENT_INST_SPEC);
}

void push_ioctl_normal_event(std::map<enum evt_class, std::vector<struct evt_noted>>& ioctl_events,
    enum evt_class e_class, struct evt_noted event)
{
    ioctl_events[e_class].push_back(event);
}

void push_ioctl_padding_event(std::map<enum evt_class, std::vector<struct evt_noted>>& ioctl_events,
    enum evt_class e_class, uint16_t event)
{
    ioctl_events[e_class].push_back({ event, EVT_PADDING, L"p" });
}

void push_ioctl_grouped_event(std::map<enum evt_class, std::vector<struct evt_noted>>& ioctl_events,
    enum evt_class e_class, struct evt_noted event, uint16_t group_num)
{
    event.note = set_event_note(event, group_num);
    event.group = static_cast<int>(group_num);
    ioctl_events[e_class].push_back(event);
}

std::wstring set_event_note(const struct evt_noted event, uint16_t group_num)
{
    std::wstring note = L"g" + std::to_wstring(group_num);
    if (event.note.empty() == false)
        note += L"," + event.note;
    return note;
}
