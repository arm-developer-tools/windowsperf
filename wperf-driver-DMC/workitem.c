
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
#include "workitem.tmh"
#endif


/* Enable/Disable the counter associated with the event */
static VOID event_enable_counter(PDEVICE_EXTENSION devExt, ppmu_event_kernel event)
{
    CoreCounterEnable(1U << devExt->counter_idx_map[event->counter_idx]);
}

static VOID event_disable_counter(PDEVICE_EXTENSION devExt, ppmu_event_kernel event)
{
    CoreCounterDisable(1U << devExt->counter_idx_map[event->counter_idx]);
}

static VOID core_counter_set_type_helper(PDEVICE_EXTENSION devExt, UINT32 counter_idx, __int64 evtype_val)
{
    CoreCouterSetType(devExt->counter_idx_map[counter_idx], evtype_val);
}

static VOID event_config_type(PDEVICE_EXTENSION devExt, ppmu_event_kernel event)
{
    UINT32 event_idx = event->event_idx;

    if (event_idx == CYCLE_EVENT_IDX)
        _WriteStatusReg(PMCCFILTR_EL0, (__int64)event->filter_bits);
    else
        core_counter_set_type_helper(devExt, event->counter_idx, (__int64)((UINT64)event_idx | event->filter_bits));
}

static VOID core_counter_enable_irq_helper(PDEVICE_EXTENSION devExt, UINT32 idx)
{
    CoreCounterEnableIrq(1U << devExt->counter_idx_map[idx]);
}

static VOID core_counter_disable_irq_helper(PDEVICE_EXTENSION devExt, UINT32 idx)
{
    CoreCounterIrqDisable(1U << devExt->counter_idx_map[idx]);
}

static VOID core_write_counter_helper(PDEVICE_EXTENSION devExt, UINT32 counter_idx, __int64 val)
{
    CoreWriteCounter(devExt->counter_idx_map[counter_idx], val);
}

VOID event_enable(PDEVICE_EXTENSION devExt, ppmu_event_kernel evt)
{
    event_disable_counter(devExt, evt);

    event_config_type(devExt, evt);

    if (evt->enable_irq)
        core_counter_enable_irq_helper(devExt, evt->counter_idx);
    else
        core_counter_disable_irq_helper(devExt, evt->counter_idx);

    event_enable_counter(devExt, evt);
}


VOID EvtWorkItemFunc(WDFWORKITEM WorkItem)
{
    PWORK_ITEM_CTXT context;
    context = WdfObjectGet_WORK_ITEM_CTXT(WorkItem);
    const UINT32 core_idx = context->core_idx;

    if (context->devExt->current_status.status == STS_IDLE)
        return;

    if (context->devExt->AskedToRemove)
        return;

    KdPrintEx((DPFLTR_IHVDRIVER_ID,  DPFLTR_INFO_LEVEL, "%!FUNC! Entry (%d) for action %s\n", core_idx, GetIoctlStr(context->IoCtl)));
/*
    if (context->IoCtl  == PMU_CTL_ASSIGN_EVENTS)
    {
        VOID(*func)(ppmu_event_kernel event) = NULL;
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
            UINT32 init_num = context->event_num <= numFreeGPC ? context->event_num : numFreeGPC;
            ppmu_event_pseudo events = &core->events[0];
            for (UINT32 j = 0; j < init_num; j++)
            {
                ppmu_event_kernel event = (struct pmu_event_kernel*)&events[numFPC + j];
                func(event);
            }
            KeRevertToUserGroupAffinityThread(&old_affinity);
        }
        KdPrintEx((DPFLTR_IHVDRIVER_ID,  DPFLTR_INFO_LEVEL, "%!FUNC! Exit\n"));
        return;
    }
    else */if (context->IoCtl == IOCTL_PMU_CTL_START || context->IoCtl == IOCTL_PMU_CTL_STOP || context->IoCtl == IOCTL_PMU_CTL_RESET)
    {
       // int last_cluster = -1;
        for (auto k = 0; k < context->cores_count; k++)
        {
            int i = context->ctl_req->cores_idx.cores_no[k];
          
            if (context->do_func || context->do_func2)
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
        KdPrintEx((DPFLTR_IHVDRIVER_ID,  DPFLTR_INFO_LEVEL, "%!FUNC! Exit\n"));
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

    switch (context->IoCtl) // Actions that must be done on `core_idx`
    {
    case IOCTL_PMU_CTL_SAMPLE_START:
    {
        CoreCounterStart();
        break;
    }

    case IOCTL_PMU_CTL_SAMPLE_SET_SRC:
    {
        context->devExt->last_fpc_read[core_idx] = _ReadStatusReg(PMCCNTR_EL0);
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
                // Since gpc_num is being increased at each execution it might not represent real GPCs and it needs to be mapped.
                core_counter_set_type_helper(context->devExt, gpc_num, (__int64)((UINT64)event_src | (UINT64)filter_bits));
                ov_mask |= 1ULL << context->devExt->counter_idx_map[gpc_num];
                core_counter_enable_irq_helper(context->devExt, gpc_num);
                core_write_counter_helper(context->devExt, gpc_num, (__int64)val);
                KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Enabling event 0x%x to GPC %d with ov_mask 0x%llx\n", 
                    event_src, 
                    context->devExt->counter_idx_map[gpc_num],
                    ov_mask));
                gpc_num++;
            }
        }

        for (; gpc_num < context->devExt->numFreeGPC; gpc_num++)
            core_counter_disable_irq_helper(context->devExt, gpc_num);
        CoreInfo* core = context->devExt->core_info + context->core_idx;
        core->ov_mask = ov_mask;
        break;
    }

    case IOCTL_PMU_CTL_SAMPLE_STOP:
    {
        CoreCounterStop();

        CoreInfo* core = context->devExt->core_info + core_idx;
        for (int i = 0; i < 32; i++)
            if (core->ov_mask & (1ULL << i))
                CoreCounterIrqDisable(1U << i);
        break;
    }
    }

    KeRevertToUserGroupAffinityThread(&old_affinity);
    KdPrintEx((DPFLTR_IHVDRIVER_ID,  DPFLTR_INFO_LEVEL, "%!FUNC! Exit\n"));
}
