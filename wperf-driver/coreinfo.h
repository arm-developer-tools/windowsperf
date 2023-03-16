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

#include "pmu.h"
#include "queue.h"
#include "wperf-common\iorequest.h"

enum prof_action
{
    PROF_DISABLED,
    PROF_NORMAL,
    PROF_MULTIPLEX,
};

typedef struct core_info
{
    struct pmu_event_pseudo events[MAX_MANAGED_CORE_EVENTS];
    struct pmu_event_pseudo dsu_events[MAX_MANAGED_DSU_EVENTS];
    UINT32 events_num;
    UINT32 dsu_events_num;
    UINT64 timer_round;
    KTIMER timer;
    UINT8 timer_running;
    UINT8 dmc_ch;
    KDPC dpc;
    enum prof_action prof_core;
    enum prof_action prof_dsu;
    enum prof_action prof_dmc;
    KSPIN_LOCK SampleLock;
    PQUEUE_CONTEXT get_sample_irp;
    FrameChain samples[SAMPLE_CHAIN_BUFFER_SIZE];
    UINT16 sample_idx;
    UINT64 sample_generated;
    UINT64 sample_dropped;
    UINT32 sample_interval[AARCH64_MAX_HWC_SUPP + numFPC];
    UINT64 ov_mask;
} CoreInfo;
