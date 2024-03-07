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


//
// Constants
//
#define dsu_numFPC          1
#define numFPC              1

#define DMC_COUNTER_BASE(val)			(0x10 + (UINT64)val * 0x28)
#define DMC_COUNTER_CTL_OFFSET			(0x10)
#define DMC_COUNTER_VAL_OFFSET			(0x20)
#define DMC_CTL_BIT_ENABLE				(0x01 << 0)
#define DMC_CTL_BIT_EMUX				(0x1F << 2)
#define DMC_CLKDIV2_NUMGPC				8
#define DMC_CLK_NUMGPC					2

typedef struct _QUEUE_CONTEXT QUEUE_CONTEXT, * PQUEUE_CONTEXT;
typedef struct _DEVICE_EXTENSION  DEVICE_EXTENSION, * PDEVICE_EXTENSION;

//
// Structures
//
typedef struct _pmu_event_pseudo
{
    UINT32 event_idx;
    UINT64 filter_bits;
    UINT32 counter_idx;
    UINT32 enable_irq;
    UINT64 value;
    UINT64 scheduled;
}pmu_event_pseudo, *ppmu_event_pseudo;

typedef struct _pmu_event_kernel
{
    UINT32 event_idx;
    UINT64 filter_bits;
    UINT32 counter_idx;
    UINT32 enable_irq;
}pmu_event_kernel, *ppmu_event_kernel;


enum prof_action
{
    PROF_DISABLED,
    PROF_NORMAL,
    PROF_MULTIPLEX,
};

enum dmc_clkdiv2_event
{
#define WPERF_DMC_CLKDIV2_EVENTS(a,b,c) DMC_CLKDIV2_##a,
#include "wperf-common\dmc-clkdiv2-events.def"
#undef WPERF_DMC_CLKDIV2_EVENTS
    DMC_CLKDIV2_EVT_NUM,
};

enum dmc_clk_event
{
#define WPERF_DMC_CLK_EVENTS(a,b,c) DMC_CLK_##a,
#include "wperf-common\dmc-clk-events.def"
#undef WPERF_DMC_CLK_EVENTS
    DMC_CLK_EVT_NUM,
};

typedef struct _dmc_desc
{
    UINT64 iomem_start;
    UINT64 iomem_len;
    pmu_event_pseudo clk_events[MAX_MANAGED_DMC_CLK_EVENTS];
    pmu_event_pseudo clkdiv2_events[MAX_MANAGED_DMC_CLKDIV2_EVENTS];
    UINT8 clk_events_num;
    UINT8 clkdiv2_events_num;
}dmc_desc, *pdmc_desc;

typedef struct _dmcs_desc
{
    pdmc_desc dmcs;
    UINT8 dmc_num;
}dmcs_desc, *pdmcs_desc;

typedef struct core_info
{
    pmu_event_pseudo events[MAX_MANAGED_CORE_EVENTS];
    pmu_event_pseudo dsu_events[MAX_MANAGED_DSU_EVENTS];
    UINT32 events_num;
    UINT32 dsu_events_num;
    UINT64 timer_round;
    KTIMER timer;
    UINT8 timer_running;
    UINT8 dmc_ch;
    KDPC dpc_overflow, dpc_multiplex, dpc_queue, dpc_reset;
    //enum prof_action prof_core;
    //enum prof_action prof_dsu;
    enum prof_action prof_dmc;
    KSPIN_LOCK SampleLock;
    PQUEUE_CONTEXT get_sample_irp;
    FrameChain samples[SAMPLE_CHAIN_BUFFER_SIZE];
    UINT16 sample_idx;
    UINT64 sample_generated;
    UINT64 sample_dropped;
    UINT32 sample_interval[AARCH64_MAX_HWC_SUPP + numFPC];
    UINT64 ov_mask;
    UINT64 idx;
    PDEVICE_EXTENSION devExt;
} CoreInfo;


typedef struct _LOCK_STATUS
{
    enum status_flag status;
    ULONG ioctl;
    KSPIN_LOCK sts_lock;
    WDFFILEOBJECT  file_object;
    LONG pmu_held;
} LOCK_STATUS;

//
// The device context performs the same job as
// a WDM device extension in the driver frameworks
//
typedef struct _DEVICE_EXTENSION
{
    LOCK_STATUS   current_status;
    dmcs_desc dmc_array;
    UINT8 dsu_numGPC;
    UINT16 dsu_numCluster;
    UINT16 dsu_sizeCluster;

    // bit N set if evt N is supported, not used at the moment, but should.
    UINT32 dsu_evt_mask_lo;
    UINT32 dsu_evt_mask_hi;
    LONG volatile cpunos;
    ULONG numCores;
    UINT8 numGPC;
    UINT8 numFreeGPC;
    UINT64 dfr0_value;
    UINT64 midr_value;
    UINT64 id_aa64dfr0_el1_value;
    HANDLE pmc_resource_handle;
    UINT8 counter_idx_map[AARCH64_MAX_HWC_SUPP + 1];
    CoreInfo* core_info;
    // Use this array to calculate the value for fixed counters via a delta approach as we are no longer resetting it.
    // See comment on CoreCounterReset() for an explanation.
    UINT64* last_fpc_read ;
    KEVENT sync_reset_dpc;
    volatile long  InUse;
    USHORT AskedToRemove;
    PQUEUE_CONTEXT  queContext;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

//
// This macro will generate an inline function called DeviceGetContext
// which will be used to get a pointer to the device context memory
// in a type safe manner.
//
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_EXTENSION, GetDeviceExtension)

//
// Function to initialize the device and its callbacks
//
NTSTATUS
WperfDriver_TCreateDevice(
    _Inout_ PWDFDEVICE_INIT DeviceInit
    );
