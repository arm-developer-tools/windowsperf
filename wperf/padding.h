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

#include <deque>
#include <map>
#include <string>
#include <vector>

#include <windows.h>
#include "wperf-common/iorequest.h"

void set_event_padding(std::map<enum evt_class, std::vector<struct evt_noted>>& ioctl_events,
    const struct pmu_device_cfg& pmu_cfg,
    std::map<enum evt_class, std::deque<struct evt_noted>>& events,
    std::map<enum evt_class, std::vector<struct evt_noted>>& groups);
void push_ioctl_grouped_event(std::map<enum evt_class, std::vector<struct evt_noted>>& ioctl_events,
    enum evt_class e_class, struct evt_noted event, uint16_t group_num);
void padding_ioctl_events(std::map<enum evt_class, std::vector<struct evt_noted>>& ioctl_events,
    enum evt_class e_class, uint8_t gpc_num, std::deque<struct evt_noted>& padding_vector);
void padding_ioctl_events(std::map<enum evt_class, std::vector<struct evt_noted>>& ioctl_events,
    enum evt_class e_class, uint8_t gpc_num);
void push_ioctl_normal_event(std::map<enum evt_class, std::vector<struct evt_noted>>& ioctl_events,
    enum evt_class e_class, struct evt_noted event);
void push_ioctl_padding_event(std::map<enum evt_class, std::vector<struct evt_noted>>& ioctl_events,
    enum evt_class e_class, uint16_t event);
std::wstring set_event_note(const struct evt_noted event, uint16_t group_num);
