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

#include "driver.h"
#include "device.h"
#include "debug.h"
#include "dmc.h"
#include "dsu.h"
#include "pmu.h"
#include "coreinfo.h"
#include "sysregs.h"
#include "wperf-common/macros.h"

//
// Constants
//
#define dsu_numFPC          1
#define numFPC              1

//
// Device events
//
EVT_WDF_DEVICE_SELF_MANAGED_IO_INIT WindowsPerfEvtDeviceSelfManagedIoStart;
EVT_WDF_DEVICE_SELF_MANAGED_IO_SUSPEND WindowsPerfEvtDeviceSelfManagedIoSuspend;


#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, WindowsPerfDeviceCreate)
#pragma alloc_text (PAGE, WindowsPerfEvtDeviceSelfManagedIoSuspend)
#endif

//
// Port Begin
//

static VOID(*dsu_ctl_funcs[3])(VOID) = { DSUCounterStart, DSUCounterStop, DSUCounterReset };

static UINT8 dsu_numGPC;
static UINT16 dsu_numCluster;
static UINT16 dsu_sizeCluster;

// bit N set if evt N is supported, not used at the moment, but should.
static UINT32 dsu_evt_mask_lo;
static UINT32 dsu_evt_mask_hi;

static LONG volatile cpunos;
static ULONG numCores;
static UINT8 numGPC;
static UINT64 dfr0_value;
static UINT64 midr_value;
static KEVENT SyncPMCEnc;
// not sure if MSVC zero globals
static HANDLE pmc_resource_handle = NULL;
static UINT8 has_long_event_support = 0;

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

struct PMUCtlHdr
{
    enum pmu_ctl_action action;
    UINT32 core_idx;
    UINT8 dmc_idx;
#define CTL_FLAG_CORE (0x1 << 0)
#define CTL_FLAG_DSU  (0x1 << 1)
#define CTL_FLAG_DMC  (0x1 << 2)
    UINT32 flags;
};

struct DSUCtlHdr
{
    enum pmu_ctl_action action;
    UINT16 cluster_num;
    UINT16 cluster_size;
};

#pragma warning(push)
#pragma warning(disable:4200)
struct DMCCtlHdr
{
    enum pmu_ctl_action action;
    UINT8 dmc_num;
    UINT64 addr[0];
};
#pragma warning(pop)

struct PMUCtlEvtAssignHdr
{
    enum pmu_ctl_action action;
    UINT32 core_idx;
    UINT8 dmc_idx;
    UINT64 filter_bits;
};

enum evt_class
{
    EVT_CORE,
    EVT_DSU,
    EVT_DMC_CLK,
    EVT_DMC_CLKDIV2,
};

struct evt_hdr
{
    enum evt_class evt_class;
    UINT16 num;
};

struct pmu_event_usr
{
    UINT32 event_idx;
    UINT64 filter_bits;
    UINT64 value;
    UINT64 scheduled;
};

typedef struct read_counting_out
{
    UINT32 evt_num;
    UINT64 round;
    struct pmu_event_usr evts[MAX_MANAGED_CORE_EVENTS];
} ReadOut;

typedef struct dsu_read_counting_out
{
    UINT32 evt_num;
    UINT64 round;
    struct pmu_event_usr evts[MAX_MANAGED_DSU_EVENTS];
} DSUReadOut;

CoreInfo* core_info;

struct hw_cfg
{
    UINT8   pmu_ver;
    UINT8   fpc_num;
    UINT8   gpc_num;
    UINT8   vendor_id;
    UINT8   variant_id;
    UINT8   arch_id;
    UINT8   rev_id;
    UINT16  part_id;
    UINT16  core_num;
};

struct dmc_desc
{
    UINT64 iomem_start;
    UINT64 iomem_len;
    struct pmu_event_pseudo clk_events[MAX_MANAGED_DMC_CLK_EVENTS];
    struct pmu_event_pseudo clkdiv2_events[MAX_MANAGED_DMC_CLKDIV2_EVENTS];
    UINT8 clk_events_num;
    UINT8 clkdiv2_events_num;
};

typedef struct dmc_read_counting_out
{
    struct pmu_event_usr clk_events[MAX_MANAGED_DMC_CLK_EVENTS];
    struct pmu_event_usr clkdiv2_events[MAX_MANAGED_DMC_CLKDIV2_EVENTS];
    UINT8 clk_events_num;
    UINT8 clkdiv2_events_num;
} DMCReadOut;

static struct dmc_desc* dmcs;
static UINT8 dmc_num;

static VOID dmc_counter_start(UINT8 ch_idx, UINT8 counter_idx)
{
    UINT64 op_base = dmcs[ch_idx].iomem_start;

    __int32 value = __iso_volatile_load32((volatile __int32*)(op_base + DMC_COUNTER_BASE(counter_idx) + DMC_COUNTER_CTL_OFFSET));
    __dmb(_ARM64_BARRIER_LD);

    value |= DMC_CTL_BIT_ENABLE;

    __dmb(_ARM64_BARRIER_ST);
    __iso_volatile_store32((volatile __int32*)(op_base + DMC_COUNTER_BASE(counter_idx) + DMC_COUNTER_CTL_OFFSET), value);
}

static VOID dmc_counter_stop(UINT8 ch_idx, UINT8 counter_idx)
{
    UINT64 op_base = dmcs[ch_idx].iomem_start;

    __int32 value = __iso_volatile_load32((volatile __int32*)(op_base + DMC_COUNTER_BASE(counter_idx) + DMC_COUNTER_CTL_OFFSET));
    __dmb(_ARM64_BARRIER_LD);

    value &= ~((__int32)DMC_CTL_BIT_ENABLE);

    __dmb(_ARM64_BARRIER_ST);
    __iso_volatile_store32((volatile __int32*)(op_base + DMC_COUNTER_BASE(counter_idx) + DMC_COUNTER_CTL_OFFSET), value);
}

static VOID dmc_counter_reset(UINT8 ch_idx, UINT8 counter_idx)
{
    UINT64 op_base = dmcs[ch_idx].iomem_start;

    __int32 value = 0;
    __dmb(_ARM64_BARRIER_ST);
    __iso_volatile_store32((volatile __int32*)(op_base + DMC_COUNTER_BASE(counter_idx) + DMC_COUNTER_VAL_OFFSET), value);
}

static VOID(*dmc_ctl_funcs[3])(UINT8, UINT8) = { dmc_counter_start, dmc_counter_stop, dmc_counter_reset };

static UINT64 dmc_counter_read(UINT8 ch_idx, UINT16 counter_idx)
{
    UINT64 op_base = dmcs[ch_idx].iomem_start;

    __int32 value = __iso_volatile_load32((volatile __int32*)(op_base + DMC_COUNTER_BASE(counter_idx) + DMC_COUNTER_VAL_OFFSET));
    __dmb(_ARM64_BARRIER_LD);

    return (UINT64)((UINT32)value);
}

static dmc_channel_iterator(UINT8 ch_base, UINT8 ch_end, VOID(*do_func)(UINT8, UINT8))
{
    for (UINT8 ch_idx = ch_base; ch_idx < ch_end; ch_idx++)
    {
        struct dmc_desc* dmc = dmcs + ch_idx;

        for (UINT8 counter_idx = 0; counter_idx < dmc->clk_events_num; counter_idx++)
            do_func(ch_idx, DMC_CLKDIV2_NUMGPC + counter_idx);
        for (UINT8 counter_idx = 0; counter_idx < dmc->clkdiv2_events_num; counter_idx++)
            do_func(ch_idx, counter_idx);
    }
}

static VOID dmc_enable_event(UINT8 ch_idx, UINT32 counter_idx, UINT16 event_idx)
{
    UINT64 op_base = dmcs[ch_idx].iomem_start;

    __int32 value = __iso_volatile_load32((volatile __int32*)(op_base + DMC_COUNTER_BASE(counter_idx) + DMC_COUNTER_CTL_OFFSET));
    __dmb(_ARM64_BARRIER_LD);

    value &= ~((__int32)DMC_CTL_BIT_EMUX);
    value |= (event_idx << 2) & DMC_CTL_BIT_EMUX;

    __dmb(_ARM64_BARRIER_ST);
    __iso_volatile_store32((volatile __int32*)(op_base + DMC_COUNTER_BASE(counter_idx) + DMC_COUNTER_CTL_OFFSET), value);
}

#define ARMV8_PMCR_MASK         0x3f
#define ARMV8_PMCR_E            (1 << 0) /*  Enable all counters */
#define ARMV8_PMCR_P            (1 << 1) /*  Reset all counters */
#define ARMV8_PMCR_C            (1 << 2) /*  Cycle counter reset */
#define ARMV8_PMCR_LC           (1 << 6) /*  Overflow on 64-bit cycle counter*/
#define ARMV8_PMCR_LP           (1 << 7) /*  Long event counter enable */
#define	FILTER_EXCL_EL1	(1U << 31)
#define ARMV8_EVTYPE_MASK   0xc800ffff  // Mask for writable bits

enum
{
#define WPERF_ARMV8_ARCH_EVENTS(a,b,c) PMU_EVENT_##a = b,
#include "wperf-common\armv8-arch-events.def"
#undef WPERF_ARMV8_ARCH_EVENTS
};

static UINT16 armv8_arch_core_events[] =
{
#define WPERF_ARMV8_ARCH_EVENTS(a,b,c) b,
#include "wperf-common\armv8-arch-events.def"
#undef WPERF_ARMV8_ARCH_EVENTS
};

static UINT32 core_pmcr_get(VOID)
{
    return (UINT32)_ReadStatusReg(PMCR_EL0);
}

static VOID core_pmcr_set(UINT32 val)
{
    val &= ARMV8_PMCR_MASK;
    __isb(_ARM64_BARRIER_SY);
    _WriteStatusReg(PMCR_EL0, (__int64)val);
}

static VOID core_counter_disable(UINT32 mask)
{
    _WriteStatusReg(PMCNTENCLR_EL0, (__int64)mask);
    __isb(_ARM64_BARRIER_SY);
}

static VOID core_counter_enable(UINT32 mask)
{
    __isb(_ARM64_BARRIER_SY);
    _WriteStatusReg(PMCNTENSET_EL0, (__int64)mask);
}

static VOID core_counter_irq_disable(UINT32 mask)
{
    _WriteStatusReg(PMINTENCLR_EL1, (__int64)mask);
    __isb(_ARM64_BARRIER_SY);
    _WriteStatusReg(PMOVSCLR_EL0, (__int64)mask);
    __isb(_ARM64_BARRIER_SY);
}

static VOID core_counter_reset(VOID)
{
    UINT32 pmcr = ARMV8_PMCR_P | ARMV8_PMCR_C | ARMV8_PMCR_LC;
    if (has_long_event_support)
        pmcr |= ARMV8_PMCR_LP;

    core_pmcr_set(pmcr);
}

static VOID core_counter_start(VOID)
{
    core_pmcr_set(core_pmcr_get() | ARMV8_PMCR_E);
}

static VOID core_counter_stop(VOID)
{
    core_pmcr_set(core_pmcr_get() & ~ARMV8_PMCR_E);
}

// must sync with enum pmu_ctl_action
static VOID(*core_ctl_funcs[3])(VOID) = { core_counter_start, core_counter_stop, core_counter_reset };

/* Enable/Disable the counter associated with the event */
static VOID event_enable_counter(struct pmu_event_kernel* event)
{
    core_counter_enable(1U << event->counter_idx);
}

static VOID event_disable_counter(struct pmu_event_kernel* event)
{
    core_counter_disable(1U << event->counter_idx);
}

#define SET_COUNTER_TYPE(N) case N: _WriteStatusReg(PMEVTYPER##N##_EL0, evtype_val); break
static VOID core_couter_set_type(UINT32 counter_idx, __int64 evtype_val)
{
    evtype_val &= ARMV8_EVTYPE_MASK;
    // can't do lookup table as _WriteStatusReg takes contant register number
    switch (counter_idx)
    {
        SET_COUNTER_TYPE(0);
        SET_COUNTER_TYPE(1);
        SET_COUNTER_TYPE(2);
        SET_COUNTER_TYPE(3);
        SET_COUNTER_TYPE(4);
        SET_COUNTER_TYPE(5);
        SET_COUNTER_TYPE(6);
        SET_COUNTER_TYPE(7);
        SET_COUNTER_TYPE(8);
        SET_COUNTER_TYPE(9);
        SET_COUNTER_TYPE(10);
        SET_COUNTER_TYPE(11);
        SET_COUNTER_TYPE(12);
        SET_COUNTER_TYPE(13);
        SET_COUNTER_TYPE(14);
        SET_COUNTER_TYPE(15);
        SET_COUNTER_TYPE(16);
        SET_COUNTER_TYPE(17);
        SET_COUNTER_TYPE(18);
        SET_COUNTER_TYPE(19);
        SET_COUNTER_TYPE(20);
        SET_COUNTER_TYPE(21);
        SET_COUNTER_TYPE(22);
        SET_COUNTER_TYPE(23);
        SET_COUNTER_TYPE(24);
        SET_COUNTER_TYPE(25);
        SET_COUNTER_TYPE(26);
        SET_COUNTER_TYPE(27);
        SET_COUNTER_TYPE(28);
        SET_COUNTER_TYPE(29);
        SET_COUNTER_TYPE(30);
    default:
        WindowsPerfKdPrintInfo("Warn: Invalid PMEVTYPE index: %d\n", counter_idx);
        break;
    }
}

static VOID event_config_type(struct pmu_event_kernel* event)
{
    UINT32 event_idx = event->event_idx;

    if (event_idx == CYCLE_EVENT_IDX)
        _WriteStatusReg(PMCCFILTR_EL0, (__int64)event->filter_bits);
    else
        core_couter_set_type(event->counter_idx, (__int64)((UINT64)event_idx | event->filter_bits));
}

static VOID core_counter_enable_irq(UINT32 mask)
{
    _WriteStatusReg(PMINTENSET_EL1, (__int64)mask);
}

static VOID event_enable_irq(struct pmu_event_kernel* event)
{
    core_counter_enable_irq(1U << event->counter_idx);
}

static VOID event_disable_irq(struct pmu_event_kernel* event)
{
    core_counter_irq_disable(1U << event->counter_idx);
}

static VOID event_enable(struct pmu_event_kernel* event)
{
    event_disable_counter(event);

    event_config_type(event);

    if (event->enable_irq)
        event_enable_irq(event);

    event_enable_counter(event);
}

#define READ_COUNTER(N) case N: return (UINT64)_ReadStatusReg(PMEVCNTR##N##_EL0)
static UINT64 core_read_counter(UINT32 counter_idx)
{
    switch (counter_idx)
    {
        READ_COUNTER(0);
        READ_COUNTER(1);
        READ_COUNTER(2);
        READ_COUNTER(3);
        READ_COUNTER(4);
        READ_COUNTER(5);
        READ_COUNTER(6);
        READ_COUNTER(7);
        READ_COUNTER(8);
        READ_COUNTER(9);
        READ_COUNTER(10);
        READ_COUNTER(11);
        READ_COUNTER(12);
        READ_COUNTER(13);
        READ_COUNTER(14);
        READ_COUNTER(15);
        READ_COUNTER(16);
        READ_COUNTER(17);
        READ_COUNTER(18);
        READ_COUNTER(19);
        READ_COUNTER(20);
        READ_COUNTER(21);
        READ_COUNTER(22);
        READ_COUNTER(23);
        READ_COUNTER(24);
        READ_COUNTER(25);
        READ_COUNTER(26);
        READ_COUNTER(27);
        READ_COUNTER(28);
        READ_COUNTER(29);
        READ_COUNTER(30);
    default:
        WindowsPerfKdPrintInfo("Warn: Invalid PMEVTYPE index: %d\n", counter_idx);
        break;
    }

    return 0xcafedead;
}

static UINT64 event_get_counting(struct pmu_event_kernel* event)
{
    UINT32 event_idx = event->event_idx;

    if (event_idx == CYCLE_EVENT_IDX)
        return _ReadStatusReg(PMCCNTR_EL0);

    return core_read_counter(event->counter_idx);
}

static VOID update_core_counting(CoreInfo* core)
{
    UINT32 events_num = core->events_num;
    struct pmu_event_pseudo* events = core->events;

    core_counter_stop();

    for (UINT32 i = 0; i < events_num; i++)
    {
        events[i].value += event_get_counting((struct pmu_event_kernel*)&events[i]);
        events[i].scheduled += 1;
    }

    core_counter_reset();
    core_counter_start();
}

static VOID update_dmc_counting(UINT8 dmc_ch)
{
    UINT8 ch_base, ch_end;

    if (dmc_ch == ALL_DMC_CHANNEL)
    {
        ch_base = 0;
        ch_end = dmc_num;
    }
    else
    {
        ch_base = dmc_ch;
        ch_end = dmc_ch + 1;
    }

    dmc_channel_iterator(ch_base, ch_end, dmc_counter_stop);

    for (UINT8 ch_idx = ch_base; ch_idx < ch_end; ch_idx++)
    {
        struct dmc_desc* dmc = dmcs + ch_idx;
        struct pmu_event_pseudo* events = dmc->clk_events;
        for (UINT8 i = 0; i < dmc->clk_events_num; i++)
        {
            events[i].value += dmc_counter_read(ch_idx, DMC_CLKDIV2_EVT_NUM + i);
            events[i].scheduled += 1;
        }

        events = dmc->clkdiv2_events;
        for (UINT8 i = 0; i < dmc->clkdiv2_events_num; i++)
        {
            events[i].value += dmc_counter_read(ch_idx, i);
            events[i].scheduled += 1;
        }
    }

    dmc_channel_iterator(ch_base, ch_end, dmc_counter_reset);
    dmc_channel_iterator(ch_base, ch_end, dmc_counter_start);
}

static VOID multiplex_dpc(struct _KDPC* dpc, PVOID ctx, PVOID sys_arg1, PVOID sys_arg2)
{
    UNREFERENCED_PARAMETER(dpc);
    UNREFERENCED_PARAMETER(sys_arg1);
    UNREFERENCED_PARAMETER(sys_arg2);

    CoreInfo* core = (CoreInfo*)ctx;
    UINT64 round = core->timer_round;
    UINT64 new_round = round + 1;

    if (core->prof_core == PROF_NORMAL)
    {
        update_core_counting(core);
    }
    else if (core->prof_core == PROF_MULTIPLEX)
    {
        struct pmu_event_pseudo* events = core->events;
        UINT32 events_num = core->events_num;

        UINT32 event_start_idx1 = (numGPC * round) % (events_num - numFPC);
        UINT32 event_start_idx2 = (numGPC * new_round) % (events_num - numFPC);

        core_counter_stop();

        //Only one FPC, cycle counter
        //We will improve the logic handling FPC later
        events[0].value += _ReadStatusReg(PMCCNTR_EL0);
        events[0].scheduled += 1;

        for (UINT32 i = 0; i < numGPC; i++)
        {
            UINT32 adjusted_idx = (event_start_idx1 + i) % (events_num - numFPC);
            events[adjusted_idx + numFPC].value += core_read_counter(i);
            events[adjusted_idx + numFPC].scheduled += 1;
        }

        core_counter_reset();

        struct pmu_event_kernel cycle_event;
        cycle_event.event_idx = CYCLE_EVENT_IDX;
        cycle_event.counter_idx = CYCLE_COUNTER_IDX;
        cycle_event.filter_bits = events[0].filter_bits;
        event_enable(&cycle_event);

        for (UINT32 i = 0; i < numGPC; i++)
        {
            UINT32 adjusted_idx = (event_start_idx2 + i) % (events_num - numFPC);

            struct pmu_event_kernel event;
            event.event_idx = events[adjusted_idx + numFPC].event_idx;
            event.filter_bits = events[adjusted_idx + numFPC].filter_bits;
            event.counter_idx = i;
            event_enable(&event);
        }

        core_counter_start();
    }

    if (core->prof_dsu == PROF_NORMAL)
    {
        DSUUpdateDSUCounting(core);
    }
    else if (core->prof_dsu == PROF_MULTIPLEX)
    {
        struct pmu_event_pseudo* events = core->dsu_events;
        UINT32 events_num = core->dsu_events_num;

        UINT32 event_start_idx1 = (dsu_numGPC * round) % (events_num - dsu_numFPC);
        UINT32 event_start_idx2 = (dsu_numGPC * new_round) % (events_num - dsu_numFPC);

        DSUCounterStop();

        //Only one FPC, cycle counter
        //We will improve the logic handling FPC later
        events[0].value += DSUReadPMCCNTR();
        events[0].scheduled += 1;

        for (UINT32 i = 0; i < dsu_numGPC; i++)
        {
            UINT32 adjusted_idx = (event_start_idx1 + i) % (events_num - dsu_numFPC);
            events[adjusted_idx + dsu_numFPC].value += DSUReadCounter(i);
            events[adjusted_idx + dsu_numFPC].scheduled += 1;
        }

        DSUCounterReset();

        struct pmu_event_kernel cycle_event;
        cycle_event.event_idx = CYCLE_EVENT_IDX;
        cycle_event.counter_idx = CYCLE_COUNTER_IDX;
        DSUEventEnable(&cycle_event);

        for (UINT32 i = 0; i < dsu_numGPC; i++)
        {
            UINT32 adjusted_idx = (event_start_idx2 + i) % (events_num - dsu_numFPC);

            struct pmu_event_kernel event;
            event.event_idx = events[adjusted_idx + dsu_numFPC].event_idx;
            event.counter_idx = i;
            DSUEventEnable(&event);
        }

        DSUCounterStart();
    }

    if (core->prof_dmc != PROF_DISABLED)
        update_dmc_counting(core->dmc_ch);


    core->timer_round = new_round;
}

// When there is no event multiplexing, we still need to use multiplexing-like timer for
// collecting counter value before overflow.
static VOID overflow_dpc(struct _KDPC* dpc, PVOID ctx, PVOID sys_arg1, PVOID sys_arg2)
{
    UNREFERENCED_PARAMETER(dpc);
    UNREFERENCED_PARAMETER(sys_arg1);
    UNREFERENCED_PARAMETER(sys_arg2);

    CoreInfo* core = (CoreInfo*)ctx;

    if (core->prof_core != PROF_DISABLED)
        update_core_counting(core);

    if (core->prof_dsu != PROF_DISABLED)
        DSUUpdateDSUCounting(core);

    if (core->prof_dmc != PROF_DISABLED)
        update_dmc_counting(core->dmc_ch);

    core->timer_round++;
}

// Default events that will be assigned to counters when driver loaded.
struct pmu_event_kernel default_events[AARCH64_MAX_HWC_SUPP + numFPC] =
{
    {CYCLE_EVENT_IDX,                   FILTER_EXCL_EL1, CYCLE_COUNTER_IDX, 0},
    {PMU_EVENT_INST_RETIRED,            FILTER_EXCL_EL1, 0,                 0},
    {PMU_EVENT_STALL_FRONTEND,          FILTER_EXCL_EL1, 1,                 0},
    {PMU_EVENT_STALL_BACKEND,           FILTER_EXCL_EL1, 2,                 0},
    {PMU_EVENT_L1I_CACHE_REFILL,        FILTER_EXCL_EL1, 3,                 0},
    {PMU_EVENT_L1I_CACHE,               FILTER_EXCL_EL1, 4,                 0},
    {PMU_EVENT_L1D_CACHE_REFILL,        FILTER_EXCL_EL1, 5,                 0},
    {PMU_EVENT_L1D_CACHE,               FILTER_EXCL_EL1, 6,                 0},
    {PMU_EVENT_BR_RETIRED,              FILTER_EXCL_EL1, 7,                 0},
    {PMU_EVENT_BR_MIS_PRED_RETIRED,     FILTER_EXCL_EL1, 8,                 0},
    {PMU_EVENT_INST_SPEC,               FILTER_EXCL_EL1, 9,                 0},
    {PMU_EVENT_ASE_SPEC,                FILTER_EXCL_EL1, 10,                0},
    {PMU_EVENT_VFP_SPEC,                FILTER_EXCL_EL1, 11,                0},
    {PMU_EVENT_BUS_ACCESS,              FILTER_EXCL_EL1, 12,                0},
    {PMU_EVENT_BUS_CYCLES,              FILTER_EXCL_EL1, 13,                0},
    {PMU_EVENT_LDST_SPEC,               FILTER_EXCL_EL1, 14,                0},
    {PMU_EVENT_DP_SPEC,                 FILTER_EXCL_EL1, 15,                0},
    {PMU_EVENT_CRYPTO_SPEC,             FILTER_EXCL_EL1, 16,                0},
    {PMU_EVENT_STREX_FAIL_SPEC,         FILTER_EXCL_EL1, 17,                0},
    {PMU_EVENT_BR_IMMED_SPEC,           FILTER_EXCL_EL1, 18,                0},
    {PMU_EVENT_BR_RETURN_SPEC,          FILTER_EXCL_EL1, 19,                0},
    {PMU_EVENT_BR_INDIRECT_SPEC,        FILTER_EXCL_EL1, 20,                0},
    {PMU_EVENT_L2I_CACHE,               FILTER_EXCL_EL1, 21,                0},
    {PMU_EVENT_L2I_CACHE_REFILL,        FILTER_EXCL_EL1, 22,                0},
    {PMU_EVENT_L2D_CACHE,               FILTER_EXCL_EL1, 23,                0},
    {PMU_EVENT_L2D_CACHE_REFILL,        FILTER_EXCL_EL1, 24,                0},
    {PMU_EVENT_L1I_TLB,                 FILTER_EXCL_EL1, 25,                0},
    {PMU_EVENT_L1I_TLB_REFILL,          FILTER_EXCL_EL1, 26,                0},
    {PMU_EVENT_L1D_TLB,                 FILTER_EXCL_EL1, 27,                0},
    {PMU_EVENT_L1D_TLB_REFILL,          FILTER_EXCL_EL1, 28,                0},
    {PMU_EVENT_L2I_TLB,                 FILTER_EXCL_EL1, 29,                0},
    {PMU_EVENT_L2I_TLB_REFILL,          FILTER_EXCL_EL1, 30,                0},
};

static VOID arm64pmc_enable_default(struct _KDPC* dpc, PVOID ctx, PVOID sys_arg1, PVOID sys_arg2)
{

    UNREFERENCED_PARAMETER(dpc);
    UNREFERENCED_PARAMETER(ctx);
    UNREFERENCED_PARAMETER(sys_arg1);
    UNREFERENCED_PARAMETER(sys_arg2);

    core_counter_reset();

    UINT32 up_limit = numGPC + numFPC;
    for (UINT32 i = 0; i < up_limit; i++)
        event_enable(default_events + i);

    core_counter_start();

    ULONG core_idx = KeGetCurrentProcessorNumberEx(NULL);
    WindowsPerfKdPrintInfo("core %d PMC enabled\n", core_idx);

    InterlockedIncrement(&cpunos);
    if ((ULONG)cpunos >= numCores)
        KeSetEvent(&SyncPMCEnc, 0, FALSE);
}

VOID free_pmu_resource(VOID)
{
    if (pmc_resource_handle == NULL)
        return;

    NTSTATUS status = HalFreeHardwareCounters(pmc_resource_handle);

    if (status != STATUS_SUCCESS)
    {
        WindowsPerfKdPrintInfo("HalFreeHardwareCounters: failed 0x%x\n", status);
    }
    else
    {
        WindowsPerfKdPrintInfo("HalFreeHardwareCounters: success\n");
    }
}

static NTSTATUS evt_assign_dsu(UINT32 core_base, UINT32 core_end, UINT16 dsu_event_num, UINT16* dsu_events)
{
    if ((dsu_event_num + dsu_numFPC) > MAX_MANAGED_DSU_EVENTS)
    {
        WindowsPerfKdPrintInfo("IOCTL: assigned dsu_event_num %d > %d\n",
            dsu_event_num, (MAX_MANAGED_DSU_EVENTS - dsu_numFPC));
        return STATUS_INVALID_PARAMETER;
    }

    WindowsPerfKdPrintInfo("IOCTL: assign %d dsu_events (%s)\n",
        dsu_event_num, dsu_event_num > dsu_numGPC ? "multiplexing" : "no-multiplexing");

    for (UINT32 i = core_base; i < core_end; i += dsu_sizeCluster)
    {
        CoreInfo* core = &core_info[i];
        core->dsu_events_num = dsu_event_num + dsu_numFPC;
        UINT32 init_num = dsu_event_num <= dsu_numGPC ? dsu_event_num : dsu_numGPC;
        struct pmu_event_pseudo* events = &core->dsu_events[0];

        RtlSecureZeroMemory(&events[dsu_numFPC], sizeof(struct pmu_event_pseudo) * (MAX_MANAGED_DSU_EVENTS - dsu_numFPC));

        events[0].event_idx = CYCLE_EVENT_IDX;
        events[0].counter_idx = CYCLE_COUNTER_IDX;
        events[0].enable_irq = 0;
        events[0].value = 0;
        events[0].scheduled = 0;

        for (UINT32 j = 0; j < dsu_event_num; j++)
        {
            struct pmu_event_kernel* event = (struct pmu_event_kernel*)&events[dsu_numFPC + j];

            event->event_idx = dsu_events[j];
            event->enable_irq = 0;
            if (j < init_num)
                event->counter_idx = j;
        }

        GROUP_AFFINITY old_affinity, new_affinity;
        PROCESSOR_NUMBER ProcNumber;

        RtlSecureZeroMemory(&new_affinity, sizeof(GROUP_AFFINITY));
        RtlSecureZeroMemory(&old_affinity, sizeof(GROUP_AFFINITY));
        RtlSecureZeroMemory(&ProcNumber, sizeof(PROCESSOR_NUMBER));
        KeGetProcessorNumberFromIndex(i, &ProcNumber);
        new_affinity.Group = ProcNumber.Group;
        new_affinity.Mask = 1ULL << (ProcNumber.Number);
        KeSetSystemGroupAffinityThread(&new_affinity, &old_affinity);

        for (UINT32 j = 0; j < init_num; j++)
        {
            struct pmu_event_kernel* event = (struct pmu_event_kernel*)&events[dsu_numFPC + j];
            DSUEventEnable(event);
        }

        KeRevertToUserGroupAffinityThread(&old_affinity);
    }

    return STATUS_SUCCESS;
}

static NTSTATUS evt_assign_core(UINT32 core_base, UINT32 core_end, UINT16 core_event_num, UINT16* core_events, UINT64 filter_bits)
{
    if ((core_event_num + numFPC) > MAX_MANAGED_CORE_EVENTS)
    {
        WindowsPerfKdPrintInfo("IOCTL: assigned core_event_num %d > %d\n",
            core_event_num, (MAX_MANAGED_CORE_EVENTS - numFPC));
        return STATUS_INVALID_PARAMETER;
    }

    WindowsPerfKdPrintInfo("IOCTL: assign %d events (%s)\n",
        core_event_num, core_event_num > numGPC ? "multiplexing" : "no-multiplexing");

    for (UINT32 i = core_base; i < core_end; i++)
    {
        CoreInfo* core = &core_info[i];
        core->events_num = core_event_num + numFPC;
        UINT32 init_num = core_event_num <= numGPC ? core_event_num : numGPC;
        struct pmu_event_pseudo* events = &core->events[0];

        RtlSecureZeroMemory(&events[numFPC], sizeof(struct pmu_event_pseudo) * (MAX_MANAGED_CORE_EVENTS - numFPC));

        // Don't clear event_idx and counter_idx, they are fixed.
        events[0].value = 0;
        events[0].scheduled = 0;
        events[0].filter_bits = filter_bits;

        for (UINT32 j = 0; j < core_event_num; j++)
        {
            struct pmu_event_kernel* event = (struct pmu_event_kernel*)&events[numFPC + j];

            event->event_idx = core_events[j];
            event->filter_bits = filter_bits;
            event->enable_irq = 0;
            if (j < init_num)
                event->counter_idx = j;
        }

        GROUP_AFFINITY old_affinity, new_affinity;
        PROCESSOR_NUMBER ProcNumber;

        RtlSecureZeroMemory(&new_affinity, sizeof(GROUP_AFFINITY));
        RtlSecureZeroMemory(&old_affinity, sizeof(GROUP_AFFINITY));
        RtlSecureZeroMemory(&ProcNumber, sizeof(PROCESSOR_NUMBER));
        KeGetProcessorNumberFromIndex(i, &ProcNumber);
        new_affinity.Group = ProcNumber.Group;
        new_affinity.Mask = 1ULL << (ProcNumber.Number);
        KeSetSystemGroupAffinityThread(&new_affinity, &old_affinity);

        for (UINT32 j = 0; j < init_num; j++)
        {
            struct pmu_event_kernel* event = (struct pmu_event_kernel*)&events[numFPC + j];
            event_enable(event);
        }

        KeRevertToUserGroupAffinityThread(&old_affinity);
    }

    return STATUS_SUCCESS;
}

// Execution DO_FUNC on CORE_IDX, then switch back
static VOID per_core_exec(UINT32 core_idx, VOID(*do_func)(VOID), VOID(*do_func2)(VOID))
{
    GROUP_AFFINITY old_affinity, new_affinity;
    PROCESSOR_NUMBER ProcNumber;

    RtlSecureZeroMemory(&new_affinity, sizeof(GROUP_AFFINITY));
    RtlSecureZeroMemory(&old_affinity, sizeof(GROUP_AFFINITY));
    RtlSecureZeroMemory(&ProcNumber, sizeof(PROCESSOR_NUMBER));

    KeGetProcessorNumberFromIndex(core_idx, &ProcNumber);
    new_affinity.Group = ProcNumber.Group;
    new_affinity.Mask = 1ULL << (ProcNumber.Number);
    KeSetSystemGroupAffinityThread(&new_affinity, &old_affinity);

    if (do_func)
        do_func();
    if (do_func2)
        do_func2();

    KeRevertToUserGroupAffinityThread(&old_affinity);
}


NTSTATUS deviceControl(
    _In_    PVOID   pBuffer,
    _In_    ULONG   inputSize,
    _Out_   PULONG  outputSize
)
{
    NTSTATUS status = STATUS_SUCCESS; 

    WindowsPerfKdPrint("IOCTL: pBuffer=%p \n", pBuffer);
    WindowsPerfKdPrint("IOCTL: inputSize=%d \n", inputSize);

    for (ULONG i = 0; i < inputSize; i++)
    {
        BYTE* p = (BYTE*)pBuffer;
        WindowsPerfKdPrint("IOCTL: pBuffer[%d]=%d \n", i, p[i]);
    }

    enum pmu_ctl_action action = *(enum pmu_ctl_action*)pBuffer;

    switch (action)
    {
    case PMU_CTL_START:
    case PMU_CTL_STOP:
    case PMU_CTL_RESET:
    {
        struct PMUCtlHdr* ctl_req = (struct PMUCtlHdr*)pBuffer;
        UINT8 dmc_ch_base = 0, dmc_ch_end = 0, dmc_idx = ALL_DMC_CHANNEL;
        UINT32 core_base, core_end;
        UINT32 ctl_flags = ctl_req->flags;
        UINT32 core_idx = ctl_req->core_idx;
        UINT32 dmc_core_idx = ALL_CORE;

        if (inputSize != sizeof(struct PMUCtlHdr))
        {
            WindowsPerfKdPrintInfo("IOCTL: invalid inputsize %ld for action %d\n", inputSize, action);
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        WindowsPerfKdPrintInfo("IOCTL: action %d\n", action);

        VOID(*core_func)(VOID) = NULL;
        VOID(*dsu_func)(VOID) = NULL;
        VOID(*dmc_func)(UINT8, UINT8) = NULL;

        if (ctl_flags & CTL_FLAG_CORE)
            core_func = core_ctl_funcs[action];
        if (ctl_flags & CTL_FLAG_DSU)
            dsu_func = dsu_ctl_funcs[action];

        if (core_idx == ALL_CORE)
        {
            core_base = 0;
            core_end = numCores;
        }
        else
        {
            core_base = core_idx;
            core_end = core_idx + 1;
        }

        for (ULONG i = core_base; i < core_end; i++)
        {
            UINT8 dsu_cluster_head = !(i & (dsu_sizeCluster - 1));
            VOID(*dsu_func2)(VOID) = dsu_func;

            // Only call on cluster head.
            if (core_idx == ALL_CORE && !dsu_cluster_head)
                dsu_func2 = NULL;

            if (core_func || dsu_func2)
                per_core_exec(i, core_func, dsu_func2);
        }

        if (ctl_flags & CTL_FLAG_DMC)
        {
            dmc_func = dmc_ctl_funcs[action];
            dmc_idx = ctl_req->dmc_idx;

            if (dmc_idx == ALL_DMC_CHANNEL)
            {
                dmc_ch_base = 0;
                dmc_ch_end = dmc_num;
            }
            else
            {
                dmc_ch_base = dmc_idx;
                dmc_ch_end = dmc_idx + 1;
            }

            dmc_channel_iterator(dmc_ch_base, dmc_ch_end, dmc_func);
            dmc_core_idx = core_idx == ALL_CORE ? (numCores - 1) : core_idx;
        }

        if (action == PMU_CTL_START)
        {
            for (UINT32 i = core_base; i < core_end; i++)
            {
                CoreInfo* core = &core_info[i];
                if (core->timer_running)
                {
                    KeCancelTimer(&core->timer);
                    core->timer_running = 0;
                }

                core->prof_core = PROF_DISABLED;
                core->prof_dsu = PROF_DISABLED;
                core->prof_dmc = PROF_DISABLED;

                UINT8 do_multiplex = 0;

                if (ctl_flags & CTL_FLAG_CORE)
                {
                    UINT8 do_core_multiplex = !!(core->events_num > (UINT32)(numGPC + numFPC));
                    core->prof_core = do_core_multiplex ? PROF_MULTIPLEX : PROF_NORMAL;
                    do_multiplex |= do_core_multiplex;
                }

                if (ctl_flags & CTL_FLAG_DSU)
                {
                    UINT8 dsu_cluster_head = !(i & (dsu_sizeCluster - 1));
                    if (core_idx != ALL_CORE || dsu_cluster_head)
                    {
                        UINT8 do_dsu_multiplex = !!(core->dsu_events_num > (UINT32)(dsu_numGPC + dsu_numFPC));
                        core->prof_dsu = do_dsu_multiplex ? PROF_MULTIPLEX : PROF_NORMAL;
                        do_multiplex |= do_dsu_multiplex;
                    }
                }

                if ((ctl_flags & CTL_FLAG_DMC) && i == dmc_core_idx)
                {
                    core->prof_dmc = PROF_NORMAL;
                    core->dmc_ch = dmc_idx;
                }

                if (core->prof_core == PROF_DISABLED && core->prof_dsu == PROF_DISABLED && core->prof_dmc == PROF_DISABLED)
                    continue;

                KeInitializeTimer(&core->timer);
                core->timer_running = 1;
                PRKDPC dpc = &(core->dpc);

                LARGE_INTEGER DueTime;
                LONG Period;

                DueTime.QuadPart = do_multiplex ? -1000000 : -2000000;
                Period = do_multiplex ? 100 : 200;
                KDEFERRED_ROUTINE* dpc_routine = do_multiplex ? multiplex_dpc : overflow_dpc;

                PROCESSOR_NUMBER ProcNumber;
                KeGetProcessorNumberFromIndex(i, &ProcNumber);
                KeInitializeDpc(dpc, dpc_routine, core);
                KeSetTargetProcessorDpcEx(dpc, &ProcNumber);
                KeSetImportanceDpc(dpc, HighImportance);
                KeSetTimerEx(&core->timer, DueTime, Period, dpc);
            }
        }
        else if (action == PMU_CTL_STOP)
        {
            for (UINT32 i = core_base; i < core_end; i++)
            {
                CoreInfo* core = &core_info[i];
                if (core->timer_running)
                {
                    KeCancelTimer(&core->timer);
                    core->timer_running = 0;
                }
            }
        }
        else if (action == PMU_CTL_RESET)
        {
            for (UINT32 i = core_base; i < core_end; i++)
            {
                CoreInfo* core = &core_info[i];
                core->timer_round = 0;
                struct pmu_event_pseudo* events = &core->events[0];
                UINT32 events_num = core->events_num;
                for (UINT32 j = 0; j < events_num; j++)
                {
                    events[j].value = 0;
                    events[j].scheduled = 0;
                }

                struct pmu_event_pseudo* dsu_events = &core->dsu_events[0];
                UINT32 dsu_events_num = core->dsu_events_num;
                for (UINT32 j = 0; j < dsu_events_num; j++)
                {
                    dsu_events[j].value = 0;
                    dsu_events[j].scheduled = 0;
                }
            }

            if (ctl_flags & CTL_FLAG_DMC)
            {
                for (UINT8 ch_idx = dmc_ch_base; ch_idx < dmc_ch_end; ch_idx++)
                {
                    struct dmc_desc* dmc = dmcs + ch_idx;
                    struct pmu_event_pseudo* events = dmc->clk_events;
                    UINT8 events_num = dmc->clk_events_num;
                    for (UINT8 j = 0; j < events_num; j++)
                    {
                        events[j].value = 0;
                        events[j].scheduled = 0;
                    }

                    events = dmc->clkdiv2_events;
                    events_num = dmc->clkdiv2_events_num;
                    for (UINT8 j = 0; j < events_num; j++)
                    {
                        events[j].value = 0;
                        events[j].scheduled = 0;
                    }
                }
            }
        }

        *outputSize = 0;
        break;
    }
    case PMU_CTL_QUERY_HW_CFG:
    {
        if (inputSize != sizeof(enum pmu_ctl_action))
        {
            WindowsPerfKdPrintInfo("IOCTL: invalid inputsize %ld for PMU_CTL_QUERY_HW_CFG\n", inputSize);
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        /*
        ULONG outputSize = IrpStackLocation->Parameters.DeviceIoControl.OutputBufferLength;
        if (outputSize != sizeof(struct hw_cfg))
        {
            WindowsPerfKdPrintInfo((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL: invalid outputsize %ld for PMU_CTL_QUERY_HW_CFG\n", outputSize));
            status = STATUS_INVALID_PARAMETER;
            break;
        }
        */

        WindowsPerfKdPrintInfo("IOCTL: QUERY_HW_CFG\n");

        struct hw_cfg* out = (struct hw_cfg*)pBuffer;
        out->core_num = (UINT16)numCores;
        out->fpc_num = numFPC;
        out->gpc_num = numGPC;
        out->pmu_ver = (dfr0_value >> 8) & 0xf;
        out->vendor_id = (midr_value >> 24) & 0xff;
        out->variant_id = (midr_value >> 20) & 0xf;
        out->arch_id = (midr_value >> 16) & 0xf;
        out->rev_id = midr_value & 0xf;
        out->part_id = (midr_value >> 4) & 0xfff;

        *outputSize = sizeof(struct hw_cfg);
        break;
    }
    case PMU_CTL_QUERY_SUPP_EVENTS:
    {
        if (inputSize != sizeof(enum pmu_ctl_action))
        {
            WindowsPerfKdPrintInfo("IOCTL: invalid inputsize %ld for PMU_CTL_QUERY_SUPP_EVENTS\n", inputSize);
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        /*
        ULONG outputSize = IrpStackLocation->Parameters.DeviceIoControl.OutputBufferLength;
        if (outputSize != (2048 * sizeof(UINT16)))
        {
            WindowsPerfKdPrintInfo((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL: invalid outputsize %ld for PMU_CTL_QUERY_SUPP_EVENTS\n", outputSize));
            status = STATUS_INVALID_PARAMETER;
            break;
        }
        */

        WindowsPerfKdPrintInfo("IOCTL: QUERY_SUPP_EVENTS\n");
        
        struct evt_hdr* hdr = (struct evt_hdr*)pBuffer;
        hdr->evt_class = EVT_CORE;
        UINT16 core_evt_num = sizeof(armv8_arch_core_events) / sizeof(armv8_arch_core_events[0]);
        hdr->num = core_evt_num;
        int return_size = 0;

        UINT16* out = (UINT16*)((UINT8*)pBuffer + sizeof(struct evt_hdr));
        for (int i = 0; i < core_evt_num; i++)
            out[i] = armv8_arch_core_events[i];

        out = out + core_evt_num;
        return_size = sizeof(struct evt_hdr);
        return_size += sizeof(armv8_arch_core_events);

        *outputSize = return_size;
        break;
    }
    case PMU_CTL_ASSIGN_EVENTS:
    {
        struct PMUCtlEvtAssignHdr* ctl_req = (struct PMUCtlEvtAssignHdr*)pBuffer;
        UINT64 filter_bits = ctl_req->filter_bits;
        UINT32 core_idx = ctl_req->core_idx;
        UINT32 core_base, core_end;

        if (core_idx == ALL_CORE)
        {
            core_base = 0;
            core_end = numCores;
        }
        else
        {
            core_base = core_idx;
            core_end = core_idx + 1;
        }

        ULONG avail_sz = inputSize - sizeof(struct PMUCtlEvtAssignHdr);
        UINT8* payload_addr = (UINT8*)pBuffer + sizeof(struct PMUCtlEvtAssignHdr);

        for (ULONG consumed_sz = 0; consumed_sz < avail_sz;)
        {
            struct evt_hdr* hdr = (struct evt_hdr*)(payload_addr + consumed_sz);
            enum evt_class evt_class = hdr->evt_class;
            UINT16 evt_num = hdr->num;
            UINT16* raw_evts = (UINT16*)(hdr + 1);

            if (evt_class == EVT_CORE)
            {
                status = evt_assign_core(core_base, core_end, evt_num, raw_evts, filter_bits);
                if (status != STATUS_SUCCESS)
                    break;
            }
            else if (evt_class == EVT_DSU)
            {
                status = evt_assign_dsu(core_base, core_end, evt_num, raw_evts);
                if (status != STATUS_SUCCESS)
                    break;
            }
            else if (evt_class == EVT_DMC_CLK || evt_class == EVT_DMC_CLKDIV2)
            {
                UINT8 dmc_idx = ctl_req->dmc_idx;
                UINT8 ch_base, ch_end;

                if (dmc_idx == ALL_DMC_CHANNEL)
                {
                    ch_base = 0;
                    ch_end = dmc_num;
                }
                else
                {
                    ch_base = dmc_idx;
                    ch_end = dmc_idx + 1;
                }

                for (UINT8 ch_idx = ch_base; ch_idx < ch_end; ch_idx++)
                {
                    struct dmc_desc* dmc = dmcs + ch_idx;
                    dmc->clk_events_num = 0;
                    dmc->clkdiv2_events_num = 0;
                    RtlSecureZeroMemory(dmc->clk_events, sizeof(struct pmu_event_pseudo) * MAX_MANAGED_DMC_CLK_EVENTS);
                    RtlSecureZeroMemory(dmc->clkdiv2_events, sizeof(struct pmu_event_pseudo) * MAX_MANAGED_DMC_CLKDIV2_EVENTS);
                    struct pmu_event_pseudo* to_events;
                    UINT16 counter_adj;

                    if (evt_class == EVT_DMC_CLK)
                    {
                        to_events = dmc->clk_events;
                        dmc->clk_events_num = (UINT8)evt_num;
                        counter_adj = DMC_CLKDIV2_NUMGPC;
                    }
                    else
                    {
                        to_events = dmc->clkdiv2_events;
                        dmc->clkdiv2_events_num = (UINT8)evt_num;
                        counter_adj = 0;
                    }

                    WindowsPerfKdPrintInfo("IOCTL: assign %d dmc_%s_events (no-multiplexing)\n",
                        evt_num, evt_class == EVT_DMC_CLK ? "clk" : "clkdiv2");

                    for (UINT16 i = 0; i < evt_num; i++)
                    {
                        struct pmu_event_pseudo* to_event = to_events + i;
                        to_event->event_idx = raw_evts[i];
                        // clk event counters start after clkdiv2 counters
                        to_event->counter_idx = counter_adj + i;
                        to_event->value = 0;
                        dmc_enable_event(ch_idx, counter_adj + i, raw_evts[i]);
                    }
                }
            }

            consumed_sz += sizeof(struct evt_hdr) + evt_num * sizeof(UINT16);
        }

        *outputSize = 0;
        break;
    }
    case PMU_CTL_READ_COUNTING:
    {
        struct PMUCtlHdr* ctl_req = (struct PMUCtlHdr*)pBuffer;
        UINT32 core_idx = ctl_req->core_idx;

        if (inputSize != sizeof(struct PMUCtlHdr))
        {
            WindowsPerfKdPrintInfo("IOCTL: invalid inputsize %ld for PMU_CTL_READ_COUNTING\n", inputSize);
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        if (core_idx != ALL_CORE && core_idx >= numCores)
        {
            WindowsPerfKdPrintInfo("IOCTL: invalid core_idx %d for PMU_CTL_READ_COUNTING\n", core_idx);
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        //ULONG outputSize = IrpStackLocation->Parameters.DeviceIoControl.OutputBufferLength;
        ULONG outputSizeExpect, outputSizeReturned;

        if (core_idx == ALL_CORE)
            outputSizeExpect = sizeof(ReadOut) * numCores;
        else
            outputSizeExpect = sizeof(ReadOut);

        /*
        if (outputSize != outputSizeExpect)
        {
            WindowsPerfKdPrintInfo((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, 
                "IOCTL: invalid outputsize %ld for PMU_CTL_READ_COUNTING\n", outputSize));
            status = STATUS_INVALID_PARAMETER;
            break;
        }
        */

        UINT32 core_base, core_end;
        if (core_idx == ALL_CORE)
        {
            core_base = 0;
            core_end = numCores;
        }
        else
        {
            core_base = core_idx;
            core_end = core_idx + 1;
        }

        outputSizeReturned = 0;
        for (UINT32 i = core_base; i < core_end; i++)
        {
            CoreInfo* core = &core_info[i];
            ReadOut* out = (ReadOut*)((UINT8*)pBuffer + sizeof(ReadOut) * (i - core_base));
            UINT32 events_num = core->events_num;
            out->evt_num = events_num;
            out->round = core->timer_round;

            struct pmu_event_usr* out_events = &out->evts[0];
            struct pmu_event_pseudo* events = core->events;

            for (UINT32 j = 0; j < events_num; j++)
            {
                struct pmu_event_pseudo* event = events + j;
                struct pmu_event_usr* out_event = out_events + j;
                out_event->event_idx = event->event_idx;
                out_event->filter_bits = event->filter_bits;
                out_event->scheduled = event->scheduled;
                out_event->value = event->value;
            }

            outputSizeReturned += sizeof(ReadOut);
        }

        *outputSize = outputSizeReturned;
        break;
    }
    case DSU_CTL_INIT:
    {
        struct DSUCtlHdr* ctl_req = (struct DSUCtlHdr*)pBuffer;

        if (inputSize != sizeof(struct DSUCtlHdr))
        {
            WindowsPerfKdPrintInfo("IOCTL: invalid inputsize %ld for DSU_CTL_INIT\n", inputSize);
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        dsu_numCluster = ctl_req->cluster_num;
        dsu_sizeCluster = ctl_req->cluster_size;

        /*
        ULONG outputSize = IrpStackLocation->Parameters.DeviceIoControl.OutputBufferLength;
        if (outputSize != sizeof(struct dsu_cfg))
        {
            WindowsPerfKdPrintInfo((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL: invalid outputsize %ld for DSU_CTL_INIT\n", outputSize));
            status = STATUS_INVALID_PARAMETER;
            break;
        }
        */

        WindowsPerfKdPrintInfo("IOCTL: DSU_CTL_INIT\n");

        DSUProbePMU(&dsu_numGPC, &dsu_evt_mask_lo, &dsu_evt_mask_hi);
        WindowsPerfKdPrintInfo("dsu pmu num %d\n", dsu_numGPC);
        WindowsPerfKdPrintInfo("dsu pmu event mask 0x%x, 0x%x\n", dsu_evt_mask_lo, dsu_evt_mask_hi);

        struct dsu_cfg* out = (struct dsu_cfg*)pBuffer;
        out->fpc_num = dsu_numFPC;
        out->gpc_num = dsu_numGPC;
        //out->evt_mask_lo = dsu_evt_mask_lo;
        //out->evt_mask_hi = dsu_evt_mask_hi;

        *outputSize = sizeof(struct dsu_cfg);
        break;
    }
    case DSU_CTL_READ_COUNTING:
    {
        struct PMUCtlHdr* ctl_req = (struct PMUCtlHdr*)pBuffer;
        UINT32 core_idx = ctl_req->core_idx;

        if (inputSize != sizeof(struct PMUCtlHdr))
        {
            WindowsPerfKdPrintInfo("IOCTL: invalid inputsize %ld for DSU_CTL_READ_COUNTING\n", inputSize);
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        if (core_idx != ALL_CORE && core_idx >= numCores)
        {
            WindowsPerfKdPrintInfo("IOCTL: invalid core_idx %d for DSU_CTL_READ_COUNTING\n", core_idx);
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        //ULONG outputSize = IrpStackLocation->Parameters.DeviceIoControl.OutputBufferLength;
        ULONG outputSizeExpect, outputSizeReturned;

        if (core_idx == ALL_CORE)
            outputSizeExpect = sizeof(DSUReadOut) * (numCores / dsu_sizeCluster);
        else
            outputSizeExpect = sizeof(DSUReadOut);

        /*
        if (outputSize != outputSizeExpect)
        {
            WindowsPerfKdPrintInfo((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL: invalid outputsize %ld for DSU_CTL_READ_COUNTING\n", outputSize));
            status = STATUS_INVALID_PARAMETER;
            break;
        }
        */

        UINT32 core_base, core_end;
        if (core_idx == ALL_CORE)
        {
            core_base = 0;
            core_end = numCores;
        }
        else
        {
            core_base = core_idx;
            core_end = core_idx + 1;
        }

        outputSizeReturned = 0;
        for (UINT32 i = core_base; i < core_end; i += dsu_sizeCluster)
        {
            CoreInfo* core = &core_info[i];
            DSUReadOut* out = (DSUReadOut*)((UINT8*)pBuffer + sizeof(DSUReadOut) * ((i - core_base) / dsu_sizeCluster));
            UINT32 events_num = core->dsu_events_num;
            out->evt_num = events_num;
            out->round = core->timer_round;

            struct pmu_event_usr* out_events = &out->evts[0];
            struct pmu_event_pseudo* events = core->dsu_events;

            for (UINT32 j = 0; j < events_num; j++)
            {
                struct pmu_event_pseudo* event = events + j;
                struct pmu_event_usr* out_event = out_events + j;
                out_event->event_idx = event->event_idx;
                out_event->scheduled = event->scheduled;
                out_event->value = event->value;
            }

            outputSizeReturned += sizeof(DSUReadOut);
        }

        *outputSize = outputSizeReturned;
        break;
    }
    case DMC_CTL_INIT:
    {
        struct DMCCtlHdr* ctl_req = (struct DMCCtlHdr*)pBuffer;
        ULONG expected_size;

        dmc_num = ctl_req->dmc_num;
        expected_size = sizeof(struct DMCCtlHdr) + dmc_num * sizeof(UINT64) * 2;

        if (inputSize != expected_size)
        {
            WindowsPerfKdPrintInfo("IOCTL: invalid inputsize %ld for DMC_CTL_INIT\n", inputSize);
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        if (!dmcs)
        {
            dmcs = (struct dmc_desc*)ExAllocatePool2(POOL_FLAG_NON_PAGED, sizeof(struct dmc_desc) * ctl_req->dmc_num, 'DMCR');
            if (!dmcs)
            {
                WindowsPerfKdPrintInfo("DMC_CTL_INIT: allocate dmcs failed\n");
                status = STATUS_INVALID_PARAMETER;
                break;
            }


            for (UINT8 i = 0; i < dmc_num; i++)
            {
                UINT64 iomem_len = ctl_req->addr[2 * i + 1] - ctl_req->addr[2 * i] + 1;
                PHYSICAL_ADDRESS phy_addr;
                phy_addr.QuadPart = ctl_req->addr[2 * i];
                dmcs[i].iomem_start = (UINT64)MmMapIoSpace(phy_addr, iomem_len, MmNonCached);
                if (!dmcs[i].iomem_start)
                {
                    WindowsPerfKdPrintInfo("IOCTL: MmMapIoSpace failed\n");
                    status = STATUS_INVALID_PARAMETER;
                    break;
                }
                dmcs[i].iomem_len = iomem_len;
                dmcs[i].clk_events_num = 0;
                dmcs[i].clkdiv2_events_num = 0;
            }
            if (status != STATUS_SUCCESS)
                break;
        }

        /*
        ULONG outputSize = IrpStackLocation->Parameters.DeviceIoControl.OutputBufferLength;
        if (outputSize != sizeof(struct dmc_cfg))
        {
            WindowsPerfKdPrintInfo((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL: invalid outputsize %ld for DMC_CTL_INIT\n", outputSize));
            status = STATUS_INVALID_PARAMETER;
            break;
        }
        */

        WindowsPerfKdPrintInfo("IOCTL: DMC_CTL_INIT\n");

        struct dmc_cfg* out = (struct dmc_cfg*)pBuffer;
        out->clk_fpc_num = 0;
        out->clk_gpc_num = DMC_CLK_NUMGPC;
        out->clkdiv2_fpc_num = 0;
        out->clkdiv2_gpc_num = DMC_CLKDIV2_NUMGPC;
        *outputSize = sizeof(struct dmc_cfg);
        break;
    }
    case DMC_CTL_READ_COUNTING:
    {
        struct PMUCtlHdr* ctl_req = (struct PMUCtlHdr*)pBuffer;
        UINT8 dmc_idx = ctl_req->dmc_idx;

        if (inputSize != sizeof(struct PMUCtlHdr))
        {
            WindowsPerfKdPrintInfo("IOCTL: invalid inputsize %ld for DMC_CTL_READ_COUNTING\n", inputSize);
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        if (dmc_idx != ALL_DMC_CHANNEL && dmc_idx >= dmc_num)
        {
            WindowsPerfKdPrintInfo("IOCTL: invalid dmc_idx %d for DMC_CTL_READ_COUNTING\n", dmc_idx);
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        //ULONG outputSize = IrpStackLocation->Parameters.DeviceIoControl.OutputBufferLength;
        ULONG outputSizeExpect, outputSizeReturned;

        if (dmc_idx == ALL_DMC_CHANNEL)
            outputSizeExpect = sizeof(DMCReadOut) * dmc_num;
        else
            outputSizeExpect = sizeof(DMCReadOut);

        /*
        if (outputSize != outputSizeExpect)
        {
            WindowsPerfKdPrintInfo((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL: invalid outputsize %ld for DMC_CTL_READ_COUNTING\n", outputSize));
            status = STATUS_INVALID_PARAMETER;
            break;
        }
        */

        UINT8 dmc_ch_base, dmc_ch_end;

        if (dmc_idx == ALL_DMC_CHANNEL)
        {
            dmc_ch_base = 0;
            dmc_ch_end = dmc_num;
        }
        else
        {
            dmc_ch_base = dmc_idx;
            dmc_ch_end = dmc_idx + 1;
        }

        outputSizeReturned = 0;
        for (UINT8 i = dmc_ch_base; i < dmc_ch_end; i++)
        {
            struct dmc_desc* dmc = dmcs + i;
            DMCReadOut* out = (DMCReadOut*)((UINT8*)pBuffer + sizeof(DMCReadOut) * (i - dmc_ch_base));
            UINT8 clk_events_num = dmc->clk_events_num;
            UINT8 clkdiv2_events_num = dmc->clkdiv2_events_num;
            out->clk_events_num = clk_events_num;
            out->clkdiv2_events_num = clkdiv2_events_num;

            struct pmu_event_usr* to_events = out->clk_events;
            struct pmu_event_pseudo* from_events = dmc->clk_events;
            for (UINT8 j = 0; j < clk_events_num; j++)
            {
                struct pmu_event_pseudo* from_event = from_events + j;
                struct pmu_event_usr* to_event = to_events + j;
                to_event->event_idx = from_event->event_idx;
                to_event->scheduled = from_event->scheduled;
                to_event->value = from_event->value;
            }

            to_events = out->clkdiv2_events;
            from_events = dmc->clkdiv2_events;
            for (UINT8 j = 0; j < clkdiv2_events_num; j++)
            {
                struct pmu_event_pseudo* from_event = from_events + j;
                struct pmu_event_usr* to_event = to_events + j;
                to_event->event_idx = from_event->event_idx;
                to_event->scheduled = from_event->scheduled;
                to_event->value = from_event->value;
            }

            outputSizeReturned += sizeof(DMCReadOut);
        }

        *outputSize = outputSizeReturned;
        break;
    }
    default:
        WindowsPerfKdPrint("IOCTL: invalid action %d\n", action);
        status = STATUS_INVALID_PARAMETER;
        *outputSize = 0;
        break;
    }

    return status;
}

VOID WindowsPerfDeviceUnload()
{
    free_pmu_resource();
    ExFreePoolWithTag(core_info, 'CORE');

    if (dmcs)
    {
        for (UINT8 i = 0; i < dmc_num; i++)
            MmUnmapIoSpace((PVOID)dmcs[i].iomem_start, dmcs[i].iomem_len);

        ExFreePoolWithTag(dmcs, 'DMCR');
        dmcs = NULL;
    }
}


//
// Port End
//

/// <summary>
/// Worker routine called to create a device and its software resources.
/// </summary>
/// <param name="DeviceInit">Pointer to an opaque init structure. Memory
/// for this structure will be freed by the framework when the
/// WdfDeviceCreate succeeds. So don't access the structure after that
/// point.</param>
/// <returns>NTSTATUS</returns>
NTSTATUS
WindowsPerfDeviceCreate(
    PWDFDEVICE_INIT DeviceInit
    )
{
    WDF_OBJECT_ATTRIBUTES   deviceAttributes;
    PDEVICE_CONTEXT deviceContext;
    WDF_PNPPOWER_EVENT_CALLBACKS    pnpPowerCallbacks;
    WDFDEVICE device;
    NTSTATUS status;

    PAGED_CODE();

    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);

    //
    // Register pnp/power callbacks so that we can start and stop the timer as the device
    // gets started and stopped.
    //
    pnpPowerCallbacks.EvtDeviceSelfManagedIoInit    = WindowsPerfEvtDeviceSelfManagedIoStart;
    pnpPowerCallbacks.EvtDeviceSelfManagedIoSuspend = WindowsPerfEvtDeviceSelfManagedIoSuspend;

    //
    // Function used for both Init and Restart Callbacks
    //
    #pragma warning(suppress: 28024)
    pnpPowerCallbacks.EvtDeviceSelfManagedIoRestart = WindowsPerfEvtDeviceSelfManagedIoStart;

    //
    // Register the PnP and power callbacks. Power policy related callbacks will be registered
    // later in SotwareInit.
    //
    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpPowerCallbacks);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceAttributes, DEVICE_CONTEXT);

    status = WdfDeviceCreate(&DeviceInit, &deviceAttributes, &device);

    if (NT_SUCCESS(status)) {
        //
        // Get the device context and initialize it. WdfObjectGet_DEVICE_CONTEXT is an
        // inline function generated by WDF_DECLARE_CONTEXT_TYPE macro in the
        // device.h header file. This function will do the type checking and return
        // the device context. If you pass a wrong object  handle
        // it will return NULL and assert if run under framework verifier mode.
        //
        deviceContext = WdfObjectGet_DEVICE_CONTEXT(device);
        deviceContext->PrivateDeviceData = 0;

        //
        // Create a device interface so that application can find and talk
        // to us.
        //
        status = WdfDeviceCreateDeviceInterface(
            device,
            &GUID_DEVINTERFACE_WINDOWSPERF,
            NULL // ReferenceString
            );

        if (NT_SUCCESS(status)) {
            //
            // Initialize the I/O Package and any Queues
            //
            status = WindowsPerfQueueInitialize(device);
        }
    }

    //
    // Port Begin
    // 

    dfr0_value = _ReadStatusReg(ID_DFR0_EL1);
    int pmu_ver = (dfr0_value >> 8) & 0xf;

    if (pmu_ver == 0x0)
    {
        WindowsPerfKdPrintInfo("PMUv3 not supported by hardware\n");
        return STATUS_FAIL_CHECK;
    }

    WindowsPerfKdPrintInfo("PMU version %d\n", pmu_ver);

    midr_value = _ReadStatusReg(MIDR_EL1);
    UINT8 implementer = (midr_value >> 24) & 0xff;
    UINT8 variant = (midr_value >> 20) & 0xf;
    UINT8 arch_num = (midr_value >> 16) & 0xf;
    UINT16 part_num = (midr_value >> 4) & 0xfff;
    UINT8 revision = midr_value & 0xf;
    WindowsPerfKdPrintInfo("arch: %d, implementer %d, variant: %d, part_num: %d, revision: %d\n",
        arch_num, implementer, variant, part_num, revision);

    if (pmu_ver == 0x6 || pmu_ver == 0x7)
        has_long_event_support = 1;

    UINT32 pmcr = core_pmcr_get();
    numGPC = (pmcr >> 11) & 0xf;
    WindowsPerfKdPrintInfo("%d general purpose hardware counters detected\n", numGPC);

    numCores = KeQueryActiveProcessorCountEx(ALL_PROCESSOR_GROUPS);
    WindowsPerfKdPrintInfo("%d cores detected\n", numCores);

    // Finally, alloc PMU counters
    status = HalAllocateHardwareCounters(NULL, 0, NULL, &pmc_resource_handle);
    if (status == STATUS_INSUFFICIENT_RESOURCES)
    {
        WindowsPerfKdPrintInfo("HAL: counters allocated by other kernel modules\n");
        return status;
    }

    if (status != STATUS_SUCCESS)
    {
        WindowsPerfKdPrintInfo("HAL: allocate failed 0x%x\n", status);
        return status;
    }

    // This driver expose private APIs (IOCTL commands), but also enable ThreadProfiling APIs.
    HARDWARE_COUNTER counter_descs[AARCH64_MAX_HWC_SUPP];
    RtlSecureZeroMemory(&counter_descs, sizeof(HARDWARE_COUNTER) * numGPC);
    for (int i = 0; i < numGPC; i++)
    {
        counter_descs[i].Type = PMCCounter;
        counter_descs[i].Index = i;

        status = KeSetHardwareCounterConfiguration(&counter_descs[i], 1);
        if (status == STATUS_WMI_ALREADY_ENABLED)
        {
            WindowsPerfKdPrintInfo("KeSetHardwareCounterConfiguration: counter %d already enabled for ThreadProfiling\n", i);
        }
        else if (status != STATUS_SUCCESS)
        {
            WindowsPerfKdPrintInfo("KeSetHardwareCounterConfiguration: counter %d failed 0x%x\n", i, status);
            return status;
        }
    }

    core_info = (CoreInfo*)ExAllocatePool2(POOL_FLAG_NON_PAGED, sizeof(CoreInfo) * numCores, 'CORE');
    if (!core_info)
    {
        WindowsPerfKdPrintInfo("ExAllocatePoolWithTag: failed \n");
        return STATUS_FAIL_CHECK;
    }
    RtlSecureZeroMemory(core_info, sizeof(CoreInfo) * numCores);

    for (ULONG i = 0; i < numCores; i++)
    {
        CoreInfo* core = &core_info[i];
        PROCESSOR_NUMBER ProcNumber;
        status = KeGetProcessorNumberFromIndex(i, &ProcNumber);
        if (status != STATUS_SUCCESS)
            return status;

        RtlCopyMemory(core->events, default_events, sizeof(struct pmu_event_kernel) * ((size_t)numGPC + numFPC));
        core->events_num = numFPC + numGPC;

        PRKDPC dpc = &core_info[i].dpc;
        KeInitializeDpc(dpc, arm64pmc_enable_default, NULL);
        status = KeSetTargetProcessorDpcEx(dpc, &ProcNumber);
        if (status != STATUS_SUCCESS)
            return status;
        KeSetImportanceDpc(dpc, HighImportance);
        KeInsertQueueDpc(dpc, NULL, NULL);
    }

    KeInitializeEvent(&SyncPMCEnc, NotificationEvent, FALSE);
    KeWaitForSingleObject(&SyncPMCEnc, Executive, KernelMode, 0, NULL);
    KeClearEvent(&SyncPMCEnc);

    WindowsPerfKdPrintInfo("loaded\n");
    //
    // Port End
    //

    return status;
}

/// <summary>
/// This event is called by the Framework when the device is started
/// or restarted after a suspend operation.
///
/// This function is not marked pageable because this function is in the
/// device power up path.When a function is marked pagable and the code
/// section is paged out, it will generate a page fault which could impact
/// the fast resume behavior because the client driver will have to wait
/// until the system drivers can service this page fau
/// </summary>
/// <param name="Device">Handle to a framework device object.</param>
/// <returns>NTSTATUS - Failures will result in the device stack being torn down.</returns>
NTSTATUS
WindowsPerfEvtDeviceSelfManagedIoStart(
    IN  WDFDEVICE Device
    )
{
    PQUEUE_CONTEXT queueContext = QueueGetContext(WdfDeviceGetDefaultQueue(Device));
    LARGE_INTEGER DueTime;

    WindowsPerfKdPrint("--> WindowsPerfEvtDeviceSelfManagedIoInit\n");

    //
    // Restart the queue and the periodic timer. We stopped them before going
    // into low power state.
    //
    WdfIoQueueStart(WdfDeviceGetDefaultQueue(Device));

    DueTime.QuadPart = WDF_REL_TIMEOUT_IN_MS(100);

    WdfTimerStart(queueContext->Timer,  DueTime.QuadPart);

    WindowsPerfKdPrint( "<-- WindowsPerfEvtDeviceSelfManagedIoInit\n");

    return STATUS_SUCCESS;
}

/// <summary>
/// This event is called by the Framework when the device is stopped
/// for resource rebalance or suspended when the system is entering
///  Sx state.
/// </summary>
/// <param name="Device">Handle to a framework device object.</param>
/// <returns>NTSTATUS - The driver is not allowed to fail this function.  If it does, the
///  device stack will be torn down.</returns>
NTSTATUS
WindowsPerfEvtDeviceSelfManagedIoSuspend(
    IN  WDFDEVICE Device
    )
{
    PQUEUE_CONTEXT queueContext = QueueGetContext(WdfDeviceGetDefaultQueue(Device));

    PAGED_CODE();

    WindowsPerfKdPrint("--> WindowsPerfEvtDeviceSelfManagedIoSuspend\n");

    //
    // Before we stop the timer we should make sure there are no outstanding
    // i/o. We need to do that because framework cannot suspend the device
    // if there are requests owned by the driver. There are two ways to solve
    // this issue: 1) We can wait for the outstanding I/O to be complete by the
    // periodic timer 2) Register EvtIoStop callback on the queue and acknowledge
    // the request to inform the framework that it's okay to suspend the device
    // with outstanding I/O. In this sample we will use the 1st approach
    // because it's pretty easy to do. We will restart the queue when the
    // device is restarted.
    //
    WdfIoQueueStopSynchronously(WdfDeviceGetDefaultQueue(Device));

    //
    // Stop the watchdog timer and wait for DPC to run to completion if it's already fired.
    //
    WdfTimerStop(queueContext->Timer, TRUE);

    WindowsPerfKdPrint( "<-- WindowsPerfEvtDeviceSelfManagedIoSuspend\n");

    return STATUS_SUCCESS;
}

