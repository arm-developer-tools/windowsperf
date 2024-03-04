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



#include "driver.h"
#if defined ENABLE_TRACING
#include "dpc.tmh"
#endif

extern pmu_event_kernel default_events[AARCH64_MAX_HWC_SUPP + numFPC];

static UINT64 core_read_counter_helper(PDEVICE_EXTENSION devExt, UINT32 counter_idx)
{
    return CoreReadCounter(devExt->counter_idx_map[counter_idx]);
}

VOID core_write_counter_helper(PDEVICE_EXTENSION devExt, UINT32 counter_idx, __int64 val)
{
    CoreWriteCounter(devExt->counter_idx_map[counter_idx], val);
}

// For the fixed counter we are getting the delta from the last readings.
static UINT64 get_fixed_counter_value(PDEVICE_EXTENSION devExt, UINT64 core_idx)
{
    UINT64 curr = _ReadStatusReg(PMCCNTR_EL0);

    // We no longer reset the fixed counter so we need to keep track of its last value
    UINT64 delta = curr - devExt->last_fpc_read[core_idx];

    // Just to avoid astronomical numbers when something weird happens
    delta = curr < devExt->last_fpc_read[core_idx] ? 0 : delta;

    devExt->last_fpc_read[core_idx] = curr;
    return delta;
}


static UINT64 event_get_counting(PDEVICE_EXTENSION devExt,  ppmu_event_kernel event, UINT64 core_idx)
{
    UINT32 event_idx = event->event_idx;

    if (event_idx == CYCLE_EVENT_IDX)
    {
        return get_fixed_counter_value(devExt, core_idx);
    }

    return core_read_counter_helper(devExt, event->counter_idx);
}

static VOID update_core_counting(PDEVICE_EXTENSION devExt, CoreInfo* core)
{
    UINT32 events_num = core->events_num;
    ppmu_event_pseudo events = core->events;
    CoreCounterStop();

    for (UINT32 i = 0; i < events_num; i++)
    {
        events[i].value += event_get_counting(devExt, (ppmu_event_kernel)&events[i], core->idx);
        events[i].scheduled += 1;
    }

    //update_last_fixed_counter(core->idx);
    devExt->last_fpc_read[core->idx] = _ReadStatusReg(PMCCNTR_EL0);

    CoreCounterReset();
    CoreCounterStart();
}

VOID multiplex_dpc(struct _KDPC* dpc, PVOID ctx, PVOID sys_arg1, PVOID sys_arg2)
{
    UNREFERENCED_PARAMETER(dpc);
    UNREFERENCED_PARAMETER(sys_arg2);
    PDEVICE_EXTENSION devExt = (PDEVICE_EXTENSION)sys_arg1;

    if (ctx == NULL)
        return;

    CoreInfo* core = (CoreInfo*)ctx;
    UINT64 round = core->timer_round;
    UINT64 new_round = round + 1;
/*
    if (core->prof_core == PROF_NORMAL)
    {
        update_core_counting(devExt, core);
    }
    else if (core->prof_core == PROF_MULTIPLEX)
    {
        ppmu_event_pseudo events = core->events;
        UINT32 events_num = core->events_num;

        UINT32 event_start_idx1 = (devExt->numFreeGPC * round) % (events_num - numFPC);
        UINT32 event_start_idx2 = (devExt->numFreeGPC * new_round) % (events_num - numFPC);

        CoreCounterStop();

        //Only one FPC, cycle counter
        //We will improve the logic handling FPC later
        events[0].value += get_fixed_counter_value(devExt, core->idx); //_ReadStatusReg(PMCCNTR_EL0);
        events[0].scheduled += 1;

        for (UINT32 i = 0; i < devExt->numFreeGPC; i++)
        {
            UINT32 adjusted_idx = (event_start_idx1 + i) % (events_num - numFPC);
            events[adjusted_idx + numFPC].value += core_read_counter_helper(devExt, i);
            events[adjusted_idx + numFPC].scheduled += 1;
        }

        //update_last_fixed_counter(core->idx);
        devExt->last_fpc_read[core->idx] = _ReadStatusReg(PMCCNTR_EL0);
        CoreCounterReset();

        for (UINT32 i = 0; i < devExt->numFreeGPC; i++)
        {
            UINT32 adjusted_idx = (event_start_idx2 + i) % (events_num - numFPC);

            pmu_event_kernel event;
            event.event_idx = events[adjusted_idx + numFPC].event_idx;
            event.filter_bits = events[adjusted_idx + numFPC].filter_bits;
            event.counter_idx = i;
            event.enable_irq = 0;
            event_enable(&event);
        }

        CoreCounterStart();
    }
    */
    if (core->prof_dmc != PROF_DISABLED)
        UpdateDmcCounting(core->dmc_ch, &devExt->dmc_array);

    core->timer_round = new_round;
}

// When there is no event multiplexing, we still need to use multiplexing-like timer for
// collecting counter value before overflow.
VOID overflow_dpc(struct _KDPC* dpc, PVOID ctx, PVOID sys_arg1, PVOID sys_arg2)
{
    UNREFERENCED_PARAMETER(dpc);
    UNREFERENCED_PARAMETER(sys_arg2);
    PDEVICE_EXTENSION devExt = (PDEVICE_EXTENSION)sys_arg1;

    if (ctx == NULL)
        return;

    CoreInfo* core = (CoreInfo*)ctx;

    if (core->prof_dmc != PROF_DISABLED)
        UpdateDmcCounting(core->dmc_ch, &devExt->dmc_array);

    core->timer_round++;
}

VOID reset_dpc(struct _KDPC* dpc, PVOID ctx, PVOID sys_arg1, PVOID sys_arg2)
{
    UNREFERENCED_PARAMETER(dpc);
    PDEVICE_EXTENSION devExt = (PDEVICE_EXTENSION)sys_arg2;

    if (ctx == NULL)
        return;

    CoreInfo* core = (CoreInfo*)ctx;
    CoreCounterStop();
    //update_last_fixed_counter(core->idx);
    devExt->last_fpc_read[core->idx] = _ReadStatusReg(PMCCNTR_EL0);
    CoreCounterReset();

    ULONG_PTR cores_count = (ULONG_PTR)sys_arg1;

    InterlockedIncrement(&devExt->cpunos);
    if ((ULONG)devExt->cpunos >= cores_count)
        KeSetEvent(&devExt->sync_reset_dpc, 0, FALSE);
}

VOID arm64pmc_enable_default(struct _KDPC* dpc, PVOID ctx, PVOID sys_arg1, PVOID sys_arg2)
{
    UNREFERENCED_PARAMETER(dpc);
    UNREFERENCED_PARAMETER(ctx);
    UNREFERENCED_PARAMETER(sys_arg2);

    PDEVICE_EXTENSION devExt = (PDEVICE_EXTENSION)sys_arg1;

    CoreCounterReset();

    UINT32 up_limit = devExt->numFreeGPC + numFPC;
    for (UINT32 i = 0; i < up_limit; i++)
        event_enable(devExt,  default_events + i);

    CoreCounterStart();

    ULONG core_idx = KeGetCurrentProcessorNumberEx(NULL);
    //update_last_fixed_counter(core_idx);
    devExt->last_fpc_read[core_idx] = _ReadStatusReg(PMCCNTR_EL0);
    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "core %d PMC enabled\n", core_idx));
}
