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
#include "device.tmh"
#include "dmc.h"
#include "dsu.h"
#include "core.h"
#include "pmu.h"
#include "coreinfo.h"
#include "sysregs.h"
#include "wperf-common\macros.h"
#include "wperf-common\iorequest.h"
#include "wperf-common\inline.h"

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
static VOID(*dmc_ctl_funcs[3])(UINT8, UINT8, struct dmcs_desc*) = { DmcCounterStart, DmcCounterStop, DmcCounterReset };

static struct dmcs_desc dmc_array;

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
static HANDLE pmc_resource_handle = NULL;

static CoreInfo* core_info;

enum
{
#define WPERF_ARMV8_ARCH_EVENTS(n,a,b,c,d) PMU_EVENT_##a = b,
#include "wperf-common\armv8-arch-events.def"
#undef WPERF_ARMV8_ARCH_EVENTS
};

static UINT16 armv8_arch_core_events[] =
{
#define WPERF_ARMV8_ARCH_EVENTS(n,a,b,c,d) b,
#include "wperf-common\armv8-arch-events.def"
#undef WPERF_ARMV8_ARCH_EVENTS
};

// must sync with enum pmu_ctl_action
static VOID(*core_ctl_funcs[3])(VOID) = { CoreCounterStart, CoreCounterStop, CoreCounterReset };

/* Enable/Disable the counter associated with the event */
static VOID event_enable_counter(struct pmu_event_kernel* event)
{
    CoreCounterEnable(1U << event->counter_idx);
}

static VOID event_disable_counter(struct pmu_event_kernel* event)
{
    CoreCounterDisable(1U << event->counter_idx);
}

static VOID event_config_type(struct pmu_event_kernel* event)
{
    UINT32 event_idx = event->event_idx;

    if (event_idx == CYCLE_EVENT_IDX)
        _WriteStatusReg(PMCCFILTR_EL0, (__int64)event->filter_bits);
    else
        CoreCouterSetType(event->counter_idx, (__int64)((UINT64)event_idx | event->filter_bits));
}

static VOID event_enable_irq(struct pmu_event_kernel* event)
{
    CoreCounterEnableIrq(1U << event->counter_idx);
}

static VOID event_disable_irq(struct pmu_event_kernel* event)
{
    CoreCounterIrqDisable(1U << event->counter_idx);
}

static VOID event_enable(struct pmu_event_kernel* event)
{
    event_disable_counter(event);

    event_config_type(event);

    if (event->enable_irq)
        event_enable_irq(event);
    else
        event_disable_irq(event);

    event_enable_counter(event);
}

#define WRITE_COUNTER(N) case N: _WriteStatusReg(PMEVCNTR##N##_EL0, (__int64)val); break;
static VOID core_write_counter(UINT32 counter_idx, __int64 val)
{
    switch(counter_idx)
    {
        WRITE_COUNTER(0);
        WRITE_COUNTER(1);
        WRITE_COUNTER(2);
        WRITE_COUNTER(3);
        WRITE_COUNTER(4);
        WRITE_COUNTER(5);
        WRITE_COUNTER(6);
        WRITE_COUNTER(7);
        WRITE_COUNTER(8);
        WRITE_COUNTER(9);
        WRITE_COUNTER(10);
        WRITE_COUNTER(11);
        WRITE_COUNTER(12);
        WRITE_COUNTER(13);
        WRITE_COUNTER(14);
        WRITE_COUNTER(15);
        WRITE_COUNTER(16);
        WRITE_COUNTER(17);
        WRITE_COUNTER(18);
        WRITE_COUNTER(19);
        WRITE_COUNTER(20);
        WRITE_COUNTER(21);
        WRITE_COUNTER(22);
        WRITE_COUNTER(23);
        WRITE_COUNTER(24);
        WRITE_COUNTER(25);
        WRITE_COUNTER(26);
        WRITE_COUNTER(27);
        WRITE_COUNTER(28);
        WRITE_COUNTER(29);
        WRITE_COUNTER(30);
        default:
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL,
                       "WindowsPerf: Warn: Invalid PMEVTYPE index: %d\n", counter_idx));
            break;
    }
}

static UINT64 event_get_counting(struct pmu_event_kernel* event)
{
    UINT32 event_idx = event->event_idx;

    if (event_idx == CYCLE_EVENT_IDX)
        return _ReadStatusReg(PMCCNTR_EL0);

    return CoreReadCounter(event->counter_idx);
}

#define PMOVSCLR_VALID_BITS_MASK 0xffffffffULL
static UINT64 arm64_clear_ov_flags(void)
{
    UINT64 pmov_value = _ReadStatusReg(PMOVSCLR_EL0);
    pmov_value &= PMOVSCLR_VALID_BITS_MASK;
    _WriteStatusReg(PMOVSCLR_EL0, (__int64)pmov_value);
    __isb(_ARM64_BARRIER_SY);
    return pmov_value;
}

typedef VOID (*PMIHANDLER)(PKTRAP_FRAME TrapFrame);

VOID arm64_pmi_handler(PKTRAP_FRAME pTrapFrame)
{
    ULONG core_idx = KeGetCurrentProcessorNumberEx(NULL);
    CoreInfo* core = core_info + core_idx;
    UINT64 ov_flags = arm64_clear_ov_flags();
    ov_flags &= core->ov_mask;

    if (!ov_flags)
        return;

    core->sample_generated++;

    if (!KeTryToAcquireSpinLockAtDpcLevel(&core->SampleLock))
    {
        core->sample_dropped++;
        return;
    }

    if (core->sample_idx == SAMPLE_CHAIN_BUFFER_SIZE)
    {

        KeReleaseSpinLockFromDpcLevel(&core->SampleLock);
        core->sample_dropped++;
        return;
    }
    else
    {
        CoreCounterStop();

        core->samples[core->sample_idx].lr = pTrapFrame->Lr;
        core->samples[core->sample_idx].pc = pTrapFrame->Pc;
        core->samples[core->sample_idx].ov_flags = ov_flags;
        core->sample_idx++;

        KeReleaseSpinLockFromDpcLevel(&core->SampleLock);

        for (int i = 0; i < 32; i++)
        {
            if (!(ov_flags & (1ULL << i)))
                continue;

            UINT32 val = 0xFFFFFFFF - core->sample_interval[i];

            if (i == 31)
                _WriteStatusReg(PMCCNTR_EL0, (__int64)val);
            else
                core_write_counter(i, (__int64)val);
        }
        CoreCounterStart();
    }
}

static VOID update_core_counting(CoreInfo* core)
{
    UINT32 events_num = core->events_num;
    struct pmu_event_pseudo* events = core->events;
    CoreCounterStop();

    for (UINT32 i = 0; i < events_num; i++)
    {
        events[i].value += event_get_counting((struct pmu_event_kernel*)&events[i]);
        events[i].scheduled += 1;
    }

    CoreCounterReset();
    CoreCounterStart();
}

static VOID multiplex_dpc(struct _KDPC* dpc, PVOID ctx, PVOID sys_arg1, PVOID sys_arg2)
{
    UNREFERENCED_PARAMETER(dpc);
    UNREFERENCED_PARAMETER(sys_arg1);
    UNREFERENCED_PARAMETER(sys_arg2);

    if (ctx == NULL)
        return;

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

        CoreCounterStop();

        //Only one FPC, cycle counter
        //We will improve the logic handling FPC later
        events[0].value += _ReadStatusReg(PMCCNTR_EL0);
        events[0].scheduled += 1;

        for (UINT32 i = 0; i < numGPC; i++)
        {
            UINT32 adjusted_idx = (event_start_idx1 + i) % (events_num - numFPC);
            events[adjusted_idx + numFPC].value += CoreReadCounter(i);
            events[adjusted_idx + numFPC].scheduled += 1;
        }

        CoreCounterReset();

        for (UINT32 i = 0; i < numGPC; i++)
        {
            UINT32 adjusted_idx = (event_start_idx2 + i) % (events_num - numFPC);

            struct pmu_event_kernel event;
            event.event_idx = events[adjusted_idx + numFPC].event_idx;
            event.filter_bits = events[adjusted_idx + numFPC].filter_bits;
            event.counter_idx = i;
            event.enable_irq = 0;
            event_enable(&event);
        }

        CoreCounterStart();
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
        UpdateDmcCounting(core->dmc_ch, &dmc_array);

    core->timer_round = new_round;
}

// When there is no event multiplexing, we still need to use multiplexing-like timer for
// collecting counter value before overflow.
static VOID overflow_dpc(struct _KDPC* dpc, PVOID ctx, PVOID sys_arg1, PVOID sys_arg2)
{
    UNREFERENCED_PARAMETER(dpc);
    UNREFERENCED_PARAMETER(sys_arg1);
    UNREFERENCED_PARAMETER(sys_arg2);

    if (ctx == NULL)
        return;

    CoreInfo* core = (CoreInfo*)ctx;
    if (core->prof_core != PROF_DISABLED)
        update_core_counting(core);

    if (core->prof_dsu != PROF_DISABLED)
        DSUUpdateDSUCounting(core);

    if (core->prof_dmc != PROF_DISABLED)
        UpdateDmcCounting(core->dmc_ch, &dmc_array);

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

    CoreCounterReset();

    UINT32 up_limit = numGPC + numFPC;
    for (UINT32 i = 0; i < up_limit; i++)
        event_enable(default_events + i);

    CoreCounterStart();

    ULONG core_idx = KeGetCurrentProcessorNumberEx(NULL);
    KdPrint(("core %d PMC enabled\n", core_idx));

    InterlockedIncrement(&cpunos);
    if ((ULONG)cpunos >= numCores)
        KeSetEvent(&SyncPMCEnc, 0, FALSE);
}

VOID free_pmu_resource(VOID)
{
    if (pmc_resource_handle == NULL)
        return;

    NTSTATUS status = HalFreeHardwareCounters(pmc_resource_handle);

    pmc_resource_handle = NULL;

    if (status != STATUS_SUCCESS)
    {
        KdPrint(("HalFreeHardwareCounters: failed 0x%x\n", status));
    }
    else
    {
        KdPrint(("HalFreeHardwareCounters: success\n"));
    }
}

static NTSTATUS evt_assign_dsu(PQUEUE_CONTEXT queueContext, UINT32 core_base, UINT32 core_end, UINT16 dsu_event_num, UINT16* dsu_events)
{
    if ((dsu_event_num + dsu_numFPC) > MAX_MANAGED_DSU_EVENTS)
    {
        KdPrint(("IOCTL: assigned dsu_event_num %d > %d\n",
            dsu_event_num, (MAX_MANAGED_DSU_EVENTS - dsu_numFPC)));
        return STATUS_INVALID_PARAMETER;
    }

    KdPrint(("IOCTL: assign %d dsu_events (%s)\n",
        dsu_event_num, dsu_event_num > dsu_numGPC ? "multiplexing" : "no-multiplexing"));

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
            else
                event->counter_idx = INVALID_COUNTER_IDX;
        }
    }

    PWORK_ITEM_CTXT context;
    context = WdfObjectGet_WORK_ITEM_CTXT(queueContext->WorkItem);
    context->action = PMU_CTL_ASSIGN_EVENTS;
    context->isDSU = true;
    context->core_base = core_base;
    context->core_end = core_end;
    context->event_num = dsu_event_num;
    KdPrint(("%!FUNC! enqueuing for action %d", context->action));
    WdfWorkItemEnqueue(queueContext->WorkItem);

    return STATUS_SUCCESS;
}

static NTSTATUS evt_assign_core(PQUEUE_CONTEXT queueContext, UINT32 core_base, UINT32 core_end, UINT16 core_event_num, UINT16* core_events, UINT64 filter_bits)
{
    if ((core_event_num + numFPC) > MAX_MANAGED_CORE_EVENTS)
    {
        KdPrint(("IOCTL: assigned core_event_num %d > %d\n",
            core_event_num, (MAX_MANAGED_CORE_EVENTS - numFPC)));
        return STATUS_INVALID_PARAMETER;
    }

    KdPrint(("IOCTL: assign %d events (%s)\n",
        core_event_num, core_event_num > numGPC ? "multiplexing" : "no-multiplexing"));

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
        events[0].counter_idx = CYCLE_COUNTER_IDX;

        for (UINT32 j = 0; j < core_event_num; j++)
        {
            struct pmu_event_kernel* event = (struct pmu_event_kernel*)&events[numFPC + j];

            event->event_idx = core_events[j];
            event->filter_bits = filter_bits;
            event->enable_irq = 0;
            if (j < init_num)
                event->counter_idx = j;
            else
                event->counter_idx = INVALID_COUNTER_IDX;
        }
    }

    PWORK_ITEM_CTXT context;
    context = WdfObjectGet_WORK_ITEM_CTXT(queueContext->WorkItem);
    context->action = PMU_CTL_ASSIGN_EVENTS;
    context->isDSU = false;
    context->core_base = core_base;
    context->core_end = core_end;
    context->event_num = core_event_num;
    KdPrint(("%!FUNC! enqueuing for action %d", context->action));
    WdfWorkItemEnqueue(queueContext->WorkItem);

    return STATUS_SUCCESS;
}

VOID EvtWorkItemFunc(WDFWORKITEM WorkItem)
{
    PWORK_ITEM_CTXT context;
    context = WdfObjectGet_WORK_ITEM_CTXT(WorkItem);

    const enum pmu_ctl_action action = context->action;
    const UINT32 core_idx = context->core_idx;

    KdPrint(("%!FUNC! Entry (%d) for action %d", core_idx, action));

    if (action == PMU_CTL_ASSIGN_EVENTS)
    {
        VOID(*func)(struct pmu_event_kernel* event) = NULL;
        if (context->isDSU)
        {
            func = DSUEventEnable;
        }
        else {
            func = event_enable;
        }

        for (UINT32 i = context->core_base; i < context->core_end; i++)
        {
            GROUP_AFFINITY old_affinity, new_affinity;
            PROCESSOR_NUMBER ProcNumber;

            RtlSecureZeroMemory(&new_affinity, sizeof(GROUP_AFFINITY));
            RtlSecureZeroMemory(&old_affinity, sizeof(GROUP_AFFINITY));
            RtlSecureZeroMemory(&ProcNumber, sizeof(PROCESSOR_NUMBER));
            KeGetProcessorNumberFromIndex(i, &ProcNumber);

            new_affinity.Group = ProcNumber.Group;
            new_affinity.Mask = 1ULL << (ProcNumber.Number);
            KeSetSystemGroupAffinityThread(&new_affinity, &old_affinity);

            CoreInfo* core = &core_info[i];
            UINT32 init_num = context->event_num <= numGPC ? context->event_num : numGPC;
            struct pmu_event_pseudo* events = &core->events[0];
            for (UINT32 j = 0; j < init_num; j++)
            {
                struct pmu_event_kernel* event = (struct pmu_event_kernel*)&events[numFPC + j];
                func(event);
            }
            KeRevertToUserGroupAffinityThread(&old_affinity);
        }
        KdPrint(("%!FUNC! Exit"));
        return;
    }
    else if (action == PMU_CTL_START || action == PMU_CTL_STOP || action == PMU_CTL_RESET)
    {
        int last_cluster = -1;
        for (auto k = 0; k < context->cores_count; k++)
        {
            int i = context->ctl_req->cores_idx.cores_no[k];
            VOID(*dsu_func2)(VOID) = context->do_func2;

            if (context->ctl_flags & CTL_FLAG_DSU)
            {
                int cluster_no = i / dsu_sizeCluster;

                // This works only if ctl_req->cores_idx.cores_no[] is sorted
                // We will only cofigure one core in cluster with per_core_exec
                if (last_cluster != cluster_no)
                    last_cluster = cluster_no;
                else
                    dsu_func2 = NULL;
            }

            if (context->do_func || dsu_func2)
            {
                GROUP_AFFINITY old_affinity, new_affinity;
                PROCESSOR_NUMBER ProcNumber;

                RtlSecureZeroMemory(&new_affinity, sizeof(GROUP_AFFINITY));
                RtlSecureZeroMemory(&old_affinity, sizeof(GROUP_AFFINITY));
                RtlSecureZeroMemory(&ProcNumber, sizeof(PROCESSOR_NUMBER));
                KeGetProcessorNumberFromIndex(i, &ProcNumber);

                new_affinity.Group = ProcNumber.Group;
                new_affinity.Mask = 1ULL << (ProcNumber.Number);
                KeSetSystemGroupAffinityThread(&new_affinity, &old_affinity);

                if (context->do_func)
                    context->do_func();
                if (context->do_func2)
                    context->do_func2();

                KeRevertToUserGroupAffinityThread(&old_affinity);
            }
        }
        KdPrint(("%!FUNC! Exit"));
        return;
    }

    GROUP_AFFINITY old_affinity, new_affinity;
    PROCESSOR_NUMBER ProcNumber;

    RtlSecureZeroMemory(&new_affinity, sizeof(GROUP_AFFINITY));
    RtlSecureZeroMemory(&old_affinity, sizeof(GROUP_AFFINITY));
    RtlSecureZeroMemory(&ProcNumber, sizeof(PROCESSOR_NUMBER));
    KeGetProcessorNumberFromIndex(core_idx, &ProcNumber);

    new_affinity.Group = ProcNumber.Group;
    new_affinity.Mask = 1ULL << (ProcNumber.Number);
    KeSetSystemGroupAffinityThread(&new_affinity, &old_affinity);
    // NOTE: counters have been enabled inside DriverEntry, so just assign event and enable irq is enough.
    UINT64 ov_mask = 0;
    int gpc_num = 0;

    switch (action) // Actions that must be done on `core_idx`
    {
    case PMU_CTL_SAMPLE_START:
    {
        CoreCounterStart();
        break;
    }

    case PMU_CTL_SAMPLE_SET_SRC:
    {
        CoreCounterStop();
        CoreCounterReset();

        for (int i = 0; i < context->sample_src_num; i++)
        {
            SampleSrcDesc* src_desc = &context->sample_req->sources[i];
            UINT32 val = 0xffffffff - src_desc->interval;
            UINT32 event_src = src_desc->event_src;
            UINT32 filter_bits = src_desc->filter_bits;

            if (event_src == CYCLE_EVENT_IDX)
            {
                _WriteStatusReg(PMCCFILTR_EL0, (__int64)filter_bits);
                ov_mask |= 1ULL << 31;
                CoreCounterEnableIrq(1U << 31);
                _WriteStatusReg(PMCCNTR_EL0, (__int64)val);
            }
            else
            {
                CoreCouterSetType(gpc_num, (__int64)((UINT64)event_src | (UINT64)filter_bits));
                ov_mask |= 1ULL << gpc_num;
                CoreCounterEnableIrq(1U << gpc_num);
                core_write_counter(gpc_num, (__int64)val);
                gpc_num++;
            }
        }

        for (; gpc_num < numGPC; gpc_num++)
            CoreCounterIrqDisable(1U << gpc_num);
        CoreInfo* core = core_info + context->core_idx;
        core->ov_mask = ov_mask;
        break;
    }

    case PMU_CTL_SAMPLE_STOP:
    {
        CoreCounterStop();

        CoreInfo* core = core_info + core_idx;
        for (int i = 0; i < 32; i++)
            if (core->ov_mask & (1ULL << i))
                CoreCounterIrqDisable(1U << i);
        break;
    }
    }

    KeRevertToUserGroupAffinityThread(&old_affinity);
    KdPrint(("%!FUNC! Exit"));
}

NTSTATUS deviceControl(
    _In_    PVOID   pBuffer,
    _In_    ULONG   inputSize,
    _Out_   PULONG  outputSize,
    _Inout_ PQUEUE_CONTEXT queueContext
)
{
    NTSTATUS status = STATUS_SUCCESS;
    *outputSize = 0;

    enum pmu_ctl_action action = *(enum pmu_ctl_action*)pBuffer;
    queueContext->action = action;  // Save for later processing

    switch (action)
    {
    case PMU_CTL_SAMPLE_START:
    {
        struct pmu_ctl_hdr *ctl_req = (struct pmu_ctl_hdr *)pBuffer;
        size_t cores_count = ctl_req->cores_idx.cores_count;

        if (cores_count != 1)
        {
            KdPrint(("IOCTL: invalid cores_count=%llu (must be 1) for action %d\n",
                                   cores_count, action));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        if (!check_cores_in_pmu_ctl_hdr_p(ctl_req))
        {
            KdPrint(("IOCTL: invalid cores_no for action %d\n", action));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        KdPrint(("IOCTL: PMU_CTL_SAMPLE_START\n"));

        UINT32 core_idx = ctl_req->cores_idx.cores_no[0];

        core_info[core_idx].sample_dropped = 0;
        core_info[core_idx].sample_generated = 0;
        core_info[core_idx].sample_idx = 0;

        PWORK_ITEM_CTXT context;
        context = WdfObjectGet_WORK_ITEM_CTXT(queueContext->WorkItem);
        context->action = PMU_CTL_SAMPLE_START;
        context->core_idx = core_idx;
        WdfWorkItemEnqueue(queueContext->WorkItem);

        *outputSize = 0;
        break;
    }
    case PMU_CTL_SAMPLE_STOP:
    {
        struct pmu_ctl_hdr *ctl_req = (struct pmu_ctl_hdr *)pBuffer;
        size_t cores_count = ctl_req->cores_idx.cores_count;

        if (cores_count != 1)
        {
            KdPrint(("IOCTL: invalid cores_count=%llu (must be 1) for action %d\n",
                                   cores_count, action));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        if (!check_cores_in_pmu_ctl_hdr_p(ctl_req))
        {
            KdPrint(("IOCTL: invalid cores_no for action %d\n", action));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        KdPrint(("IOCTL: PMU_CTL_SAMPLE_STOP\n"));

        UINT32 core_idx = ctl_req->cores_idx.cores_no[0];

        PWORK_ITEM_CTXT context;
        context = WdfObjectGet_WORK_ITEM_CTXT(queueContext->WorkItem);
        context->action = PMU_CTL_SAMPLE_STOP;
        context->core_idx = core_idx;
        WdfWorkItemEnqueue(queueContext->WorkItem);

        struct PMUSampleSummary *out = (struct PMUSampleSummary *)pBuffer;
        out->sample_generated = core_info[core_idx].sample_generated;
        out->sample_dropped = core_info[core_idx].sample_dropped;
        *outputSize = sizeof(struct PMUSampleSummary);
        break;
    }
    case PMU_CTL_SAMPLE_GET:
    {
        struct PMUCtlGetSampleHdr *ctl_req = (struct PMUCtlGetSampleHdr *)pBuffer;
        UINT32 core_idx = ctl_req->core_idx;
        CoreInfo *core = core_info + core_idx;
        KIRQL oldIrql;

        KeAcquireSpinLock(&core->SampleLock, &oldIrql);
        {
            struct PMUSamplePayload *out = (struct PMUSamplePayload*)pBuffer;
            *outputSize = sizeof(struct PMUSamplePayload);

            RtlSecureZeroMemory(out, *outputSize);

            if (core->sample_idx > 0)
            {
                RtlCopyMemory(out->payload, core->samples, sizeof(FrameChain) * core->sample_idx);
                out->size = core->sample_idx;
                core->sample_idx = 0;
            }
        }
        KeReleaseSpinLock(&core->SampleLock, oldIrql);

        return STATUS_SUCCESS;
    }
    case PMU_CTL_SAMPLE_SET_SRC:
    {
        KdPrint(("IOCTL: PMU_CTL_SAMPLE_SET_SRC\n"));

        PMUSampleSetSrcHdr *sample_req = (PMUSampleSetSrcHdr *)pBuffer;
        UINT32 core_idx = sample_req->core_idx;

        int sample_src_num = (inputSize - sizeof(PMUSampleSetSrcHdr)) / sizeof(SampleSrcDesc);
        // rough check
        if (sample_src_num > (numGPC + numFPC))
            sample_src_num = numGPC + numFPC;

        CoreInfo *core = core_info + core_idx;
        int gpc_num = 0;
        for (int i = 0; i < sample_src_num; i++)
        {
            SampleSrcDesc *src_desc = &sample_req->sources[i];
            UINT32 event_src = src_desc->event_src;
            UINT32 interval = src_desc->interval;
            if (event_src == CYCLE_EVENT_IDX)
                core->sample_interval[31] = interval;
            else
                core->sample_interval[gpc_num++] = interval;
        }

        PWORK_ITEM_CTXT context;
        context = WdfObjectGet_WORK_ITEM_CTXT(queueContext->WorkItem);
        context->action = PMU_CTL_SAMPLE_SET_SRC;
        context->core_idx = core_idx;
        context->sample_req = sample_req;
        context->sample_src_num = sample_src_num;
        WdfWorkItemEnqueue(queueContext->WorkItem);

        *outputSize = 0;
        break;
    }
    case PMU_CTL_START:
    case PMU_CTL_STOP:
    case PMU_CTL_RESET:
    {
        struct pmu_ctl_hdr* ctl_req = (struct pmu_ctl_hdr*)pBuffer;
        UINT8 dmc_ch_base = 0, dmc_ch_end = 0, dmc_idx = ALL_DMC_CHANNEL;
        UINT32 ctl_flags = ctl_req->flags;
        size_t cores_count = ctl_req->cores_idx.cores_count;
        int dmc_core_idx = ALL_CORE;

        if (inputSize != sizeof(struct pmu_ctl_hdr))
        {
            KdPrint(("IOCTL: invalid inputsize %ld for action %d\n", inputSize, action));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        if (cores_count == 0 || cores_count >= MAX_PMU_CTL_CORES_COUNT)
        {
            KdPrint(("IOCTL: invalid cores_count=%llu (must be 1-%d) for action %d\n",
                                   cores_count, MAX_PMU_CTL_CORES_COUNT, action));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        if (!check_cores_in_pmu_ctl_hdr_p(ctl_req))
        {
            KdPrint(("IOCTL: invalid cores_no for action %d\n", action));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        KdPrint(("IOCTL: action %d\n", action));

        VOID(*core_func)(VOID) = NULL;
        VOID(*dsu_func)(VOID) = NULL;
        VOID(*dmc_func)(UINT8, UINT8, struct dmcs_desc*) = NULL;

        if (ctl_flags & CTL_FLAG_CORE)
            core_func = core_ctl_funcs[action - PMU_CTL_ACTION_OFFSET];
        if (ctl_flags & CTL_FLAG_DSU)
            dsu_func = dsu_ctl_funcs[action - PMU_CTL_ACTION_OFFSET];

        PWORK_ITEM_CTXT context;
        context = WdfObjectGet_WORK_ITEM_CTXT(queueContext->WorkItem);
        context->action = PMU_CTL_START;
        context->do_func = core_func;
        context->do_func2 = dsu_func;
        KdPrint(("%!FUNC! %!LINE! enqueuing for action %d", context->action));
        WdfWorkItemEnqueue(queueContext->WorkItem);

        if ((ctl_flags & CTL_FLAG_DMC))
        {
            dmc_func = dmc_ctl_funcs[action - PMU_CTL_ACTION_OFFSET];
            dmc_idx = ctl_req->dmc_idx;

            if (dmc_idx == ALL_DMC_CHANNEL)
            {
                dmc_ch_base = 0;
                dmc_ch_end = dmc_array.dmc_num;
            }
            else
            {
                dmc_ch_base = dmc_idx;
                dmc_ch_end = dmc_idx + 1;
            }

            DmcChannelIterator(dmc_ch_base, dmc_ch_end, dmc_func, &dmc_array);
            // Use last core from all used.
            dmc_core_idx = ctl_req->cores_idx.cores_no[cores_count - 1];
        }

        if (action == PMU_CTL_START)
        {
            for (auto k = 0; k < cores_count; k++)
            {
                int i = ctl_req->cores_idx.cores_no[k];
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
                    if (cores_count != numCores || dsu_cluster_head)
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
            for (auto k = 0; k < cores_count; k++)
            {
                int i = ctl_req->cores_idx.cores_no[k];
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
            for (auto k = 0; k < cores_count; k++)
            {
                int i = ctl_req->cores_idx.cores_no[k];
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
                    struct dmc_desc* dmc = dmc_array.dmcs + ch_idx;
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
            KdPrint(("IOCTL: invalid inputsize %ld for PMU_CTL_QUERY_HW_CFG\n", inputSize));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        KdPrint(("IOCTL: QUERY_HW_CFG\n"));

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
        out->midr_value = midr_value;

        *outputSize = sizeof(struct hw_cfg);
        break;
    }
    case PMU_CTL_QUERY_SUPP_EVENTS:
    {
        if (inputSize != sizeof(enum pmu_ctl_action))
        {
            KdPrint(("IOCTL: invalid inputsize %ld for PMU_CTL_QUERY_SUPP_EVENTS\n", inputSize));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        KdPrint(("IOCTL: QUERY_SUPP_EVENTS\n"));

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

		if (dsu_numGPC)
		{
			hdr = (struct evt_hdr *)out;
			hdr->evt_class = EVT_DSU;
			UINT16 dsu_evt_num = 0;

			out = (UINT16 *)((UINT8 *)out + sizeof(struct evt_hdr));

			for (UINT16 i = 0; i < 32; i++)
			{
				if (dsu_evt_mask_lo & (1 << i))
					out[dsu_evt_num++]  = i;
			}

			for (UINT16 i = 0; i < 32; i++)
			{
				if (dsu_evt_mask_hi & (1 << i))
					out[dsu_evt_num++]  = 32 + i;
			}

			hdr->num = dsu_evt_num;

			return_size += sizeof(struct evt_hdr);
			return_size += dsu_evt_num * sizeof(UINT16);

			out = out + dsu_evt_num;
		}

		if (dmc_array.dmcs)
		{
			hdr = (struct evt_hdr *)out;
			hdr->evt_class = EVT_DMC_CLK;
			UINT16 dmc_evt_num = 0;

			out = (UINT16 *)((UINT8 *)out + sizeof(struct evt_hdr));

			for (UINT16 evt_idx = 0; evt_idx < DMC_CLK_EVT_NUM; evt_idx++)
				out[dmc_evt_num++]  = evt_idx;

			hdr->num = dmc_evt_num;

			return_size += sizeof(struct evt_hdr);
			return_size += dmc_evt_num * sizeof(UINT16);
			out = out + dmc_evt_num;

			hdr = (struct evt_hdr *)out;
			hdr->evt_class = EVT_DMC_CLKDIV2;
			dmc_evt_num = 0;
			out = (UINT16 *)((UINT8 *)out + sizeof(struct evt_hdr));

			for (UINT16 evt_idx = 0; evt_idx < DMC_CLKDIV2_EVT_NUM; evt_idx++)
				out[dmc_evt_num++]  = evt_idx;

			hdr->num = dmc_evt_num;

			return_size += sizeof(struct evt_hdr);
			return_size += dmc_evt_num * sizeof(UINT16);
			out = out + dmc_evt_num;
		}

        *outputSize = return_size;
        break;
    }
    case PMU_CTL_QUERY_VERSION:
    {
        if (inputSize != sizeof(struct pmu_ctl_ver_hdr))
        {
            KdPrint(("IOCTL: invalid inputsize %ld for PMU_CTL_QUERY_VERSION\n",
                                   inputSize));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        KdPrint(("IOCTL: QUERY_VERSION\n"));

        struct pmu_ctl_ver_hdr* ctl_req = (struct pmu_ctl_ver_hdr*)pBuffer;
        if (ctl_req->version.major != MAJOR || ctl_req->version.minor != MINOR
            || ctl_req->version.patch != PATCH)
        {
            KdPrint(("IOCTL: version mismatch bewteen wperf-driver and wperf\n"));
            KdPrint(("IOCTL: wperf-driver version: %d.%d.%d\n",
                                   MAJOR, MINOR, PATCH));
            KdPrint(("IOCTL: wperf version: %d.%d.%d\n",
                                   ctl_req->version.major,
                                   ctl_req->version.minor,
                                   ctl_req->version.patch));
        }

        struct version_info* ver_info = (struct version_info*)pBuffer;
        ver_info->major = MAJOR;
        ver_info->minor = MINOR;
        ver_info->patch = PATCH;

        *outputSize = sizeof(struct version_info);
        break;
    }
    case PMU_CTL_ASSIGN_EVENTS:
    {
        struct pmu_ctl_evt_assign_hdr* ctl_req = (struct pmu_ctl_evt_assign_hdr*)pBuffer;
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

        ULONG avail_sz = inputSize - sizeof(struct pmu_ctl_evt_assign_hdr);
        UINT8* payload_addr = (UINT8*)pBuffer + sizeof(struct pmu_ctl_evt_assign_hdr);

        for (ULONG consumed_sz = 0; consumed_sz < avail_sz;)
        {
            struct evt_hdr* hdr = (struct evt_hdr*)(payload_addr + consumed_sz);
            enum evt_class evt_class = hdr->evt_class;
            UINT16 evt_num = hdr->num;
            UINT16* raw_evts = (UINT16*)(hdr + 1);

            if (evt_class == EVT_CORE)
            {
                status = evt_assign_core(queueContext, core_base, core_end, evt_num, raw_evts, filter_bits);
                if (status != STATUS_SUCCESS)
                    break;
            }
            else if (evt_class == EVT_DSU)
            {
                status = evt_assign_dsu(queueContext, core_base, core_end, evt_num, raw_evts);
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
                    ch_end = dmc_array.dmc_num;
                }
                else
                {
                    ch_base = dmc_idx;
                    ch_end = dmc_idx + 1;
                }

                for (UINT8 ch_idx = ch_base; ch_idx < ch_end; ch_idx++)
                {
                    struct dmc_desc* dmc = dmc_array.dmcs + ch_idx;
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

                    KdPrint(("IOCTL: assign %d dmc_%s_events (no-multiplexing)\n",
                        evt_num, evt_class == EVT_DMC_CLK ? "clk" : "clkdiv2"));

                    for (UINT16 i = 0; i < evt_num; i++)
                    {
                        struct pmu_event_pseudo* to_event = to_events + i;
                        to_event->event_idx = raw_evts[i];
                        // clk event counters start after clkdiv2 counters
                        to_event->counter_idx = counter_adj + i;
                        to_event->value = 0;
                        DmcEnableEvent(ch_idx, counter_adj + i, raw_evts[i], &dmc_array);
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
        struct pmu_ctl_hdr* ctl_req = (struct pmu_ctl_hdr*)pBuffer;
        size_t cores_count = ctl_req->cores_idx.cores_count;
        UINT8 core_idx = ctl_req->cores_idx.cores_no[0];    // This query supports only 1 core

        if (inputSize != sizeof(struct pmu_ctl_hdr))
        {
            KdPrint(("IOCTL: invalid inputsize %ld for PMU_CTL_READ_COUNTING\n", inputSize));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        if (cores_count != 1)
        {
            KdPrint(("IOCTL: invalid cores_count=%llu (must be 1) for PMU_CTL_READ_COUNTING\n", cores_count));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        if (!check_cores_in_pmu_ctl_hdr_p(ctl_req))
        {
            KdPrint(("IOCTL: invalid cores_no for action %d\n", action));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        ULONG outputSizeExpect, outputSizeReturned;

        if (core_idx == ALL_CORE)
            outputSizeExpect = sizeof(ReadOut) * numCores;
        else
            outputSizeExpect = sizeof(ReadOut);

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
        struct dsu_ctl_hdr* ctl_req = (struct dsu_ctl_hdr*)pBuffer;

        if (inputSize != sizeof(struct dsu_ctl_hdr))
        {
            KdPrint(("IOCTL: invalid inputsize %ld for DSU_CTL_INIT\n", inputSize));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        dsu_numCluster = ctl_req->cluster_num;
        dsu_sizeCluster = ctl_req->cluster_size;

        KdPrint(("IOCTL: DSU_CTL_INIT\n"));

        DSUProbePMU(&dsu_numGPC, &dsu_evt_mask_lo, &dsu_evt_mask_hi);
        KdPrint(("dsu pmu num %d\n", dsu_numGPC));
        KdPrint(("dsu pmu event mask 0x%x, 0x%x\n", dsu_evt_mask_lo, dsu_evt_mask_hi));

        struct dsu_cfg* out = (struct dsu_cfg*)pBuffer;
        out->fpc_num = dsu_numFPC;
        out->gpc_num = dsu_numGPC;

        *outputSize = sizeof(struct dsu_cfg);
        break;
    }
    case DSU_CTL_READ_COUNTING:
    {
        struct pmu_ctl_hdr* ctl_req = (struct pmu_ctl_hdr*)pBuffer;
        size_t cores_count = ctl_req->cores_idx.cores_count;
        UINT8 core_idx = ctl_req->cores_idx.cores_no[0];    // This query supports only 1 core

        if (inputSize != sizeof(struct pmu_ctl_hdr))
        {
            KdPrint(("IOCTL: invalid inputsize %ld for DSU_CTL_READ_COUNTING\n", inputSize));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        if (cores_count != 1)
        {
            KdPrint(("IOCTL: invalid cores_count=%llu (must be 1) for DSU_CTL_READ_COUNTING\n", cores_count));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        if (!check_cores_in_pmu_ctl_hdr_p(ctl_req))
        {
            KdPrint(("IOCTL: invalid cores_no for action %d\n", action));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        ULONG outputSizeExpect, outputSizeReturned;

        if (core_idx == ALL_CORE)
            outputSizeExpect = sizeof(DSUReadOut) * (numCores / dsu_sizeCluster);
        else
            outputSizeExpect = sizeof(DSUReadOut);

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
        struct dmc_ctl_hdr* ctl_req = (struct dmc_ctl_hdr*)pBuffer;
        ULONG expected_size;

        dmc_array.dmc_num = ctl_req->dmc_num;
        expected_size = sizeof(struct dmc_ctl_hdr) + dmc_array.dmc_num * sizeof(UINT64) * 2;

        if (inputSize != expected_size)
        {
            KdPrint(("IOCTL: invalid inputsize %ld for DMC_CTL_INIT\n", inputSize));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        if (!dmc_array.dmcs)
        {
            dmc_array.dmcs = (struct dmc_desc*)ExAllocatePool2(POOL_FLAG_NON_PAGED, sizeof(struct dmc_desc) * ctl_req->dmc_num, 'DMCR');
            if (!dmc_array.dmcs)
            {
                KdPrint(("DMC_CTL_INIT: allocate dmcs failed\n"));
                status = STATUS_INVALID_PARAMETER;
                break;
            }


            for (UINT8 i = 0; i < dmc_array.dmc_num; i++)
            {
                UINT64 iomem_len = ctl_req->addr[2 * i + 1] - ctl_req->addr[2 * i] + 1;
                PHYSICAL_ADDRESS phy_addr;
                phy_addr.QuadPart = ctl_req->addr[2 * i];
                dmc_array.dmcs[i].iomem_start = (UINT64)MmMapIoSpace(phy_addr, iomem_len, MmNonCached);
                if (!dmc_array.dmcs[i].iomem_start)
                {
                    KdPrint(("IOCTL: MmMapIoSpace failed\n"));
                    status = STATUS_INVALID_PARAMETER;
                    break;
                }
                dmc_array.dmcs[i].iomem_len = iomem_len;
                dmc_array.dmcs[i].clk_events_num = 0;
                dmc_array.dmcs[i].clkdiv2_events_num = 0;
            }
            if (status != STATUS_SUCCESS)
                break;
        }

        KdPrint(("IOCTL: DMC_CTL_INIT\n"));

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
        struct pmu_ctl_hdr* ctl_req = (struct pmu_ctl_hdr*)pBuffer;
        UINT8 dmc_idx = ctl_req->dmc_idx;

        if (inputSize != sizeof(struct pmu_ctl_hdr))
        {
            KdPrint(("IOCTL: invalid inputsize %ld for DMC_CTL_READ_COUNTING\n", inputSize));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        if (dmc_idx != ALL_DMC_CHANNEL && dmc_idx >= dmc_array.dmc_num)
        {
            KdPrint(("IOCTL: invalid dmc_idx %d for DMC_CTL_READ_COUNTING\n", dmc_idx));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        ULONG outputSizeExpect, outputSizeReturned;

        if (dmc_idx == ALL_DMC_CHANNEL)
            outputSizeExpect = sizeof(DMCReadOut) * dmc_array.dmc_num;
        else
            outputSizeExpect = sizeof(DMCReadOut);

        UINT8 dmc_ch_base, dmc_ch_end;

        if (dmc_idx == ALL_DMC_CHANNEL)
        {
            dmc_ch_base = 0;
            dmc_ch_end = dmc_array.dmc_num;
        }
        else
        {
            dmc_ch_base = dmc_idx;
            dmc_ch_end = dmc_idx + 1;
        }

        outputSizeReturned = 0;
        for (UINT8 i = dmc_ch_base; i < dmc_ch_end; i++)
        {
            struct dmc_desc* dmc = dmc_array.dmcs + i;
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
        KdPrint(("IOCTL: invalid action %d\n", action));
        status = STATUS_INVALID_PARAMETER;
        *outputSize = 0;
        break;
    }

    return status;
}

VOID WindowsPerfDeviceUnload()
{
    free_pmu_resource();
    if (core_info)
        ExFreePoolWithTag(core_info, 'CORE');

    if (dmc_array.dmcs)
    {
        for (UINT8 i = 0; i < dmc_array.dmc_num; i++)
            MmUnmapIoSpace((PVOID)dmc_array.dmcs[i].iomem_start, dmc_array.dmcs[i].iomem_len);

        ExFreePoolWithTag(dmc_array.dmcs, 'DMCR');
        dmc_array.dmcs = NULL;
    }

    // Uninstall PMI isr
    PMIHANDLER isr = NULL;
    if (HalSetSystemInformation(HalProfileSourceInterruptHandler, sizeof(PMIHANDLER), (PVOID)&isr) != STATUS_SUCCESS)
        KdPrint(("uninstalling sampling isr failed \n"));
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
    // Set exclusive to TRUE so that no more than one app can talk to the
    // control device at any time.
    //
    WdfDeviceInitSetExclusive(DeviceInit, TRUE);

    WdfDeviceInitSetPowerPageable(DeviceInit);

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
        KdPrint(("PMUv3 not supported by hardware\n"));
        return STATUS_FAIL_CHECK;
    }

    KdPrint(("PMU version %d\n", pmu_ver));

    midr_value = _ReadStatusReg(MIDR_EL1);
    UINT8 implementer = (midr_value >> 24) & 0xff;
    UINT8 variant = (midr_value >> 20) & 0xf;
    UINT8 arch_num = (midr_value >> 16) & 0xf;
    UINT16 part_num = (midr_value >> 4) & 0xfff;
    UINT8 revision = midr_value & 0xf;
    KdPrint(("arch: %d, implementer %d, variant: %d, part_num: %d, revision: %d\n",
        arch_num, implementer, variant, part_num, revision));

    if (pmu_ver == 0x6 || pmu_ver == 0x7)
        CpuHasLongEventSupportSet(1);

    UINT32 pmcr = CorePmcrGet();
    numGPC = (pmcr >> ARMV8_PMCR_N_SHIFT) & ARMV8_PMCR_N_MASK;
    KdPrint(("%d general purpose hardware counters detected\n", numGPC));

    numCores = KeQueryActiveProcessorCountEx(ALL_PROCESSOR_GROUPS);
    KdPrint(("%d cores detected\n", numCores));

    // Finally, alloc PMU counters
    status = HalAllocateHardwareCounters(NULL, 0, NULL, &pmc_resource_handle);
    if (status == STATUS_INSUFFICIENT_RESOURCES)
    {
        KdPrint(("HAL: counters allocated by other kernel modules\n"));
        return status;
    }

    if (status != STATUS_SUCCESS)
    {
        KdPrint(("HAL: allocate failed 0x%x\n", status));
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
            KdPrint(("KeSetHardwareCounterConfiguration: counter %d already enabled for ThreadProfiling\n", i));
        }
        else if (status != STATUS_SUCCESS)
        {
            KdPrint(("KeSetHardwareCounterConfiguration: counter %d failed 0x%x\n", i, status));
            return status;
        }
    }

    core_info = (CoreInfo*)ExAllocatePool2(POOL_FLAG_NON_PAGED, sizeof(CoreInfo) * numCores, 'CORE');
    if (!core_info)
    {
        KdPrint(("ExAllocatePoolWithTag: failed \n"));
        return STATUS_INSUFFICIENT_RESOURCES;
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

        // Initialize fields for sampling;
        KeInitializeSpinLock(&core->SampleLock);

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

    PMIHANDLER isr = arm64_pmi_handler;
    status = HalSetSystemInformation(HalProfileSourceInterruptHandler, sizeof(PMIHANDLER), (PVOID)&isr);
    if (status != STATUS_SUCCESS)
    {
        KdPrint(("register sampling isr failed \n"));
        return status;
    }

    KdPrint(("loaded\n"));
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
    KdPrint(("--> WindowsPerfEvtDeviceSelfManagedIoInit\n"));

    //
    // Restart the queue and the periodic timer. We stopped them before going
    // into low power state.
    //
    WdfIoQueueStart(WdfDeviceGetDefaultQueue(Device));

    KdPrint(( "<-- WindowsPerfEvtDeviceSelfManagedIoInit\n"));

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
    PAGED_CODE();

    KdPrint(("--> WindowsPerfEvtDeviceSelfManagedIoSuspend\n"));

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

    KdPrint(( "<-- WindowsPerfEvtDeviceSelfManagedIoSuspend\n"));

    return STATUS_SUCCESS;
}
