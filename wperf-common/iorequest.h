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

//
// Below structures represent binary protocol between wperf and wperf-driver
//
enum evt_class
{
    EVT_CLASS_FIRST,
    EVT_CORE = EVT_CLASS_FIRST,
    EVT_DSU,
    EVT_DMC_CLK,
    EVT_DMC_CLKDIV2,
    EVT_CLASS_NUM,
};

struct evt_hdr
{
    enum evt_class evt_class;
    UINT16 num;
};

//
// PMU communication
//
enum pmu_ctl_action
{
	PMU_CTL_START,
	PMU_CTL_STOP,
	PMU_CTL_RESET,
	PMU_CTL_QUERY_HW_CFG,
	PMU_CTL_QUERY_SUPP_EVENTS,
	PMU_CTL_ASSIGN_EVENTS,
	PMU_CTL_READ_COUNTING,
	DSU_CTL_INIT,
	DSU_CTL_READ_COUNTING,
	DMC_CTL_INIT,
	DMC_CTL_READ_COUNTING,
};

struct pmu_ctl_hdr
{
	enum pmu_ctl_action action;
	UINT32 core_idx;
	UINT8 dmc_idx;
#define CTL_FLAG_CORE (0x1 << 0)
#define CTL_FLAG_DSU  (0x1 << 1)
#define CTL_FLAG_DMC  (0x1 << 2)
	UINT32 flags;
};

struct pmu_ctl_evt_assign_hdr
{
    enum pmu_ctl_action action;
    UINT32 core_idx;
    UINT8 dmc_idx;
    UINT64 filter_bits;
};

struct pmu_event_usr
{
    UINT32 event_idx;
    UINT64 filter_bits;
    UINT64 value;
    UINT64 scheduled;
};

typedef struct pmu_event_read_out
{
    UINT32 evt_num;
    UINT64 round;
    struct pmu_event_usr evts[MAX_MANAGED_CORE_EVENTS];
} ReadOut;

//
// DSU communication
//
struct dsu_ctl_hdr
{
    enum pmu_ctl_action action;
    UINT16 cluster_num;
    UINT16 cluster_size;
};

#pragma warning(push)
#pragma warning(disable:4200)
struct dmc_ctl_hdr
{
    enum pmu_ctl_action action;
    UINT8 dmc_num;
    UINT64 addr[0];
};
#pragma warning(pop)

typedef struct dsu_read_counting_out
{
    UINT32 evt_num;
    UINT64 round;
    struct pmu_event_usr evts[MAX_MANAGED_DSU_EVENTS];
} DSUReadOut;

//
// DMC communication
//
typedef struct dmc_pmu_event_read_out
{
	struct pmu_event_usr clk_events[MAX_MANAGED_DMC_CLK_EVENTS];
	struct pmu_event_usr clkdiv2_events[MAX_MANAGED_DMC_CLKDIV2_EVENTS];
	UINT8 clk_events_num;
	UINT8 clkdiv2_events_num;
} DMCReadOut;
