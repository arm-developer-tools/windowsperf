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
#include "device.h"
#if defined ENABLE_TRACING
#include "devioctl.tmh"
#endif
#include "utilities.h"
#include "dmc.h"
#include "dsu.h"
#include "core.h"
#include "wperf-common\gitver.h"
#include "wperf-common\inline.h"


static VOID(*dsu_ctl_funcs[3])(VOID) = { DSUCounterStart, DSUCounterStop, DSUCounterReset };
static VOID(*dmc_ctl_funcs[3])(UINT8, UINT8, struct dmcs_desc*) = { DmcCounterStart, DmcCounterStop, DmcCounterReset };


extern struct dmcs_desc dmc_array;
extern UINT8 dsu_numGPC;
extern UINT16 dsu_numCluster;
extern UINT16 dsu_sizeCluster;
// bit N set if evt N is supported, not used at the moment, but should.
extern UINT32 dsu_evt_mask_lo;
extern UINT32 dsu_evt_mask_hi;
extern LONG volatile cpunos;
extern ULONG numCores;
extern UINT8 numGPC;
extern UINT8 numFreeGPC;
extern UINT64 dfr0_value;
extern UINT64 midr_value;
extern UINT64 id_aa64dfr0_el1_value;
extern HANDLE pmc_resource_handle;
extern CoreInfo* core_info;
extern KEVENT sync_reset_dpc;
extern UINT8 counter_idx_map[AARCH64_MAX_HWC_SUPP + 1];
static UINT16 armv8_arch_core_events[] =
{
#define WPERF_ARMV8_ARCH_EVENTS(n,a,b,c,d) b,
#include "wperf-common\armv8-arch-events.def"
#undef WPERF_ARMV8_ARCH_EVENTS
};

extern LOCK_STATUS   current_status;


// must sync with enum pmu_ctl_action
static VOID(*core_ctl_funcs[3])(VOID) = { CoreCounterStart, CoreCounterStop, CoreCounterReset };

static NTSTATUS evt_assign_core(PQUEUE_CONTEXT queueContext, UINT32 core_base, UINT32 core_end, UINT16 core_event_num, UINT16* core_events, UINT64 filter_bits)
{
    if ((core_event_num + numFPC) > MAX_MANAGED_CORE_EVENTS)
    {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IOCTL: assigned core_event_num %d > %d\n",
            core_event_num, (MAX_MANAGED_CORE_EVENTS - numFPC)));
        return STATUS_INVALID_PARAMETER;
    }

    KdPrintEx((DPFLTR_IHVDRIVER_ID,  DPFLTR_INFO_LEVEL, "IOCTL: assign %d events (%s)\n",
        core_event_num, core_event_num > numFreeGPC ? "multiplexing" : "no-multiplexing"));

    for (UINT32 i = core_base; i < core_end; i++)
    {
        CoreInfo* core = &core_info[i];
        core->events_num = core_event_num + numFPC;
        UINT32 init_num = core_event_num <= numFreeGPC ? core_event_num : numFreeGPC;
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
    KdPrintEx((DPFLTR_IHVDRIVER_ID,  DPFLTR_INFO_LEVEL, "%!FUNC! enqueuing for action %d\n", context->action));
    WdfWorkItemEnqueue(queueContext->WorkItem);
    WdfWorkItemFlush(queueContext->WorkItem);       // Wait for `WdfWorkItemEnqueue` to finish

    return STATUS_SUCCESS;
}

static NTSTATUS evt_assign_dsu(PQUEUE_CONTEXT queueContext, UINT32 core_base, UINT32 core_end, UINT16 dsu_event_num, UINT16* dsu_events)
{
    if ((dsu_event_num + dsu_numFPC) > MAX_MANAGED_DSU_EVENTS)
    {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IOCTL: assigned dsu_event_num %d > %d\n",
            dsu_event_num, (MAX_MANAGED_DSU_EVENTS - dsu_numFPC)));
        return STATUS_INVALID_PARAMETER;
    }

    KdPrintEx((DPFLTR_IHVDRIVER_ID,  DPFLTR_INFO_LEVEL, "IOCTL: assign %d dsu_events (%s)\n",
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
    KdPrintEx((DPFLTR_IHVDRIVER_ID,  DPFLTR_INFO_LEVEL, "%!FUNC! enqueuing for action %d\n", context->action));
    WdfWorkItemEnqueue(queueContext->WorkItem);
    WdfWorkItemFlush(queueContext->WorkItem);       // Wait for `WdfWorkItemEnqueue` to finish

    return STATUS_SUCCESS;
}

/// <summary>
/// Handle `IoCtlCode` incomming IOCTL and form output buffer and return state for the response.
///
/// Note: With method buffered the input and output buffers (`pInBuffer` and `pOutBuffer` respectively)
/// are actually the same, so the input buffer needs to be consumed before the output buffer is
/// written to.
/// </summary>
NTSTATUS deviceControl(
    _In_        WDFFILEOBJECT  file_object,
    _In_        ULONG   IoCtlCode,
    _In_        PVOID   pInBuffer,
    _In_        ULONG   InBufSize,
    _In_opt_    PVOID   pOutBuffer,
    _In_        ULONG   OutBufSize,
    _Out_       PULONG  outputSize,
    _Inout_     PQUEUE_CONTEXT queueContext
)
{
    NTSTATUS status = STATUS_SUCCESS;
    *outputSize = 0;

    ULONG action = (IoCtlCode >> 2) & 0xFFF;   // 12 bits are the 'Function'
    queueContext->action = action;  // Save for later processing  

    //
    //  Do some basic validation of the input puffer
    //
    if (!pInBuffer)
    {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IOCTL: invalid header for %d\n",
            action));
        return STATUS_INVALID_PARAMETER;
    }

    switch (IoCtlCode)
    {
    case IOCTL_PMU_CTL_LOCK_ACQUIRE:
    {
        if (InBufSize != sizeof(struct lock_request))
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IOCTL: invalid inputsize %ld for PMU_CTL_LOCK_ACQUIRE\n", InBufSize));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL: PMU_CTL_LOCK_ACQUIRE for file object %p\n", file_object));

        struct lock_request* in = (struct lock_request*)pInBuffer;
        enum status_flag out = STS_BUSY;

        if (in->flag == LOCK_GET_FORCE)
        {
            if (current_status.pmu_held == 0)
            {
                NTSTATUS st = get_pmu_resource();
                if (st == STATUS_INSUFFICIENT_RESOURCES)
                {
                    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL: PMU_CTL_LOCK_ACQUIRE sending STS_INSUFFICIENT_RESOURCES"));
                    out = STS_INSUFFICIENT_RESOURCES;
                    goto clean_lock_acquire;
                }
                else if (st != STATUS_SUCCESS) {
                    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL: PMU_CTL_LOCK_ACQUIRE sending STS_UNKNOWN_ERROR %x", st));
                    out = STS_UNKNOWN_ERROR;
                    goto clean_lock_acquire;
                }
            }

            InterlockedExchange(&current_status.pmu_held, 1);

            out = STS_LOCK_AQUIRED;
            SetMeBusyForce(IoCtlCode, file_object);
        }
        else if (in->flag == LOCK_GET)
        {
            if (SetMeBusy(IoCtlCode, file_object)) // returns failure if the lock is already held by another process
            {
                if (current_status.pmu_held == 0)
                {
                    NTSTATUS st = get_pmu_resource();
                    if (st == STATUS_INSUFFICIENT_RESOURCES)
                    {
                        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL: PMU_CTL_LOCK_ACQUIRE sending STS_INSUFFICIENT_RESOURCES"));
                        out = STS_INSUFFICIENT_RESOURCES;
                        SetMeIdle(file_object); // Release the lock as we can't use it since there are no resources
                        goto clean_lock_acquire;
                    }
                    else if (st != STATUS_SUCCESS) {
                        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL: PMU_CTL_LOCK_ACQUIRE sending STS_UNKNOWN_ERROR %x", st));
                        out = STS_UNKNOWN_ERROR;
                        SetMeIdle(file_object); // Release the lock as there was an unknown error
                        goto clean_lock_acquire;
                    }
                }

                InterlockedExchange(&current_status.pmu_held, 1);

                out = STS_LOCK_AQUIRED;
                // Note: else STS_BUSY;
            }
        }
        else
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IOCTL: invalid flag %d for PMU_CTL_LOCK_ACQUIRE\n", in->flag));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

clean_lock_acquire:
        *((enum status_flag*)pOutBuffer) = out;
        *outputSize = sizeof(enum status_flag);
        break;
    }

    case IOCTL_PMU_CTL_LOCK_RELEASE:
    {
        if (InBufSize != sizeof(struct lock_request))
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IOCTL: invalid inputsize %ld for PMU_CTL_LOCK_RELEASE\n", InBufSize));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL: PMU_CTL_LOCK_RELEASE for file_object %p\n", file_object));

        struct lock_request* in = (struct lock_request*)pInBuffer;
        enum status_flag out = STS_BUSY;

        if (in->flag == LOCK_RELEASE)
        {
            if (SetMeIdle(file_object)) // returns failure if this process doesnt own the lock 
            {
                out = STS_IDLE;         // All went well and we went IDLE
                LONG oldval = InterlockedExchange(&current_status.pmu_held, 0);
                if (oldval == 1)
                    free_pmu_resource();
            }
            // Note: else out = STS_BUSY;     // This is illegal, as we are not IDLE
        }
        else
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IOCTL: invalid flag %d for PMU_CTL_LOCK_RELEASE\n", in->flag));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        *((enum status_flag*)pOutBuffer) = out;
        *outputSize = sizeof(enum status_flag);
        break;
    }

    case IOCTL_PMU_CTL_SAMPLE_START:
    {
        struct pmu_ctl_hdr* ctl_req = (struct pmu_ctl_hdr*)pInBuffer;
        size_t cores_count = ctl_req->cores_idx.cores_count;

        // does our process own the lock?
        if (!AmILocking(IoCtlCode, file_object))
        {
            status = STATUS_INVALID_DEVICE_STATE;
            break;
        }

        if (InBufSize != sizeof(struct pmu_ctl_hdr))
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IOCTL: invalid inputsize %ld for action %d\n", InBufSize, action));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        if (cores_count != 1)
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IOCTL: invalid cores_count=%llu (must be 1) for action %d\n",
                cores_count, action));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

		if (!CTRL_FLAG_VALID(ctl_req->flags))
		{
			KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IOCTL: invalid flags  0x%X for action %d\n",
				ctl_req->flags, action));
			status = STATUS_INVALID_PARAMETER;
			break;
		}

        if (!check_cores_in_pmu_ctl_hdr_p(ctl_req))
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IOCTL: invalid cores_no for action %d\n", action));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL: PMU_CTL_SAMPLE_START\n"));

        UINT32 core_idx = ctl_req->cores_idx.cores_no[0];

        core_info[core_idx].sample_dropped = 0;
        core_info[core_idx].sample_generated = 0;
        core_info[core_idx].sample_idx = 0;

        PWORK_ITEM_CTXT context;
        context = WdfObjectGet_WORK_ITEM_CTXT(queueContext->WorkItem);
        context->action = PMU_CTL_SAMPLE_START;
        context->core_idx = core_idx;
        WdfWorkItemEnqueue(queueContext->WorkItem);
        WdfWorkItemFlush(queueContext->WorkItem);       // Wait for `WdfWorkItemEnqueue` to finish

        *outputSize = 0;
        break;
    }
    case IOCTL_PMU_CTL_SAMPLE_STOP:
    {
        struct pmu_ctl_hdr* ctl_req = (struct pmu_ctl_hdr*)pInBuffer;
        size_t cores_count = ctl_req->cores_idx.cores_count;

        // does our process own the lock?
        if (!AmILocking(IoCtlCode, file_object))
        {
            status = STATUS_INVALID_DEVICE_STATE;
            break;
        }

        if (InBufSize != sizeof(struct pmu_ctl_hdr))
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IOCTL: invalid inputsize %ld for action %d\n", InBufSize, action));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        if (cores_count != 1)
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IOCTL: invalid cores_count=%llu (must be 1) for action %d\n",
                cores_count, action));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        if (!CTRL_FLAG_VALID(ctl_req->flags))
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IOCTL: invalid flags 0x%X for action %d\n",
                ctl_req->flags, action));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        if (!check_cores_in_pmu_ctl_hdr_p(ctl_req))
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IOCTL: invalid cores_no for action %d\n", action));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL: PMU_CTL_SAMPLE_STOP\n"));

        UINT32 core_idx = ctl_req->cores_idx.cores_no[0];

        PWORK_ITEM_CTXT context;
        context = WdfObjectGet_WORK_ITEM_CTXT(queueContext->WorkItem);
        context->action = PMU_CTL_SAMPLE_STOP;
        context->core_idx = core_idx;
        WdfWorkItemEnqueue(queueContext->WorkItem);
        WdfWorkItemFlush(queueContext->WorkItem);       // Wait for `WdfWorkItemEnqueue` to finish

        struct PMUSampleSummary* out = (struct PMUSampleSummary*)pOutBuffer;
        out->sample_generated = core_info[core_idx].sample_generated;
        out->sample_dropped = core_info[core_idx].sample_dropped;
        *outputSize = sizeof(struct PMUSampleSummary);

        if (*outputSize > OutBufSize)
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "*outputSize > OutBufSize\n"));
            status = STATUS_BUFFER_TOO_SMALL;
        }
        break;
    }
    case IOCTL_PMU_CTL_SAMPLE_GET:
    {
        struct PMUCtlGetSampleHdr* ctl_req = (struct PMUCtlGetSampleHdr*)pInBuffer;
        UINT32 core_idx = ctl_req->core_idx;
        CoreInfo* core = core_info + core_idx;
        KIRQL oldIrql;

        // does our process own the lock?
        if (!AmILocking(IoCtlCode, file_object))
        {
            status = STATUS_INVALID_DEVICE_STATE;
            break;
        }

        if (InBufSize != sizeof(struct PMUCtlGetSampleHdr))
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IOCTL: invalid inputsize %ld for action %d\n", InBufSize, action));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL: PMU_CTL_SAMPLE_GET\n"));

        KeAcquireSpinLock(&core->SampleLock, &oldIrql);
        {
            struct PMUSamplePayload* out = (struct PMUSamplePayload*)pOutBuffer;
            *outputSize = sizeof(struct PMUSamplePayload);
            if (*outputSize > OutBufSize)
            {
                KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "*outputSize > OutBufSize\n"));
                status = STATUS_BUFFER_TOO_SMALL;
            }

            RtlSecureZeroMemory(out, *outputSize);

            if (core->sample_idx > 0)
            {
                RtlCopyMemory(out->payload, core->samples, sizeof(FrameChain) * core->sample_idx);
                out->size = core->sample_idx;
                core->sample_idx = 0;
            }
        }
        KeReleaseSpinLock(&core->SampleLock, oldIrql);
        break;
    }
    case IOCTL_PMU_CTL_SAMPLE_SET_SRC:
    {
        // does our process own the lock?
        if (!AmILocking(IoCtlCode, file_object))
        {
            status = STATUS_INVALID_DEVICE_STATE;
            break;
        }

        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL: PMU_CTL_SAMPLE_SET_SRC\n"));

        PMUSampleSetSrcHdr* sample_req = (PMUSampleSetSrcHdr*)pInBuffer;
        UINT32 core_idx = sample_req->core_idx;

        int sample_src_num = (InBufSize - sizeof(PMUSampleSetSrcHdr)) / sizeof(SampleSrcDesc);

        // rough check
        if (sample_src_num > (numFreeGPC + numFPC))
            sample_src_num = numFreeGPC + numFPC;

        CoreInfo* core = core_info + core_idx;
        int gpc_num = 0;
        for (int i = 0; i < sample_src_num; i++)
        {
            /* Here we greedly assign events to GPCs. We cannot have more events than GPCs as we don't have multiplex implemented
            * for sampling. We need to use counter_idx_map here as the gpc_num-nth available GPC might not be the one we expect it to be.
            */
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Setting sample src to event_src = %d interval = %d gpc = %d\n", sample_req->sources[i].event_src, sample_req->sources[i].interval, counter_idx_map[gpc_num]));
            SampleSrcDesc* src_desc = &sample_req->sources[i];
            UINT32 event_src = src_desc->event_src;
            UINT32 interval = src_desc->interval;
            if (event_src == CYCLE_EVENT_IDX)
                core->sample_interval[31] = interval;
            else
                core->sample_interval[counter_idx_map[gpc_num++]] = interval;
        }

        PWORK_ITEM_CTXT context;
        context = WdfObjectGet_WORK_ITEM_CTXT(queueContext->WorkItem);
        context->action = PMU_CTL_SAMPLE_SET_SRC;
        context->core_idx = core_idx;
        context->sample_req = sample_req;
        context->sample_src_num = sample_src_num;
        WdfWorkItemEnqueue(queueContext->WorkItem);
        WdfWorkItemFlush(queueContext->WorkItem);       // Wait for `WdfWorkItemEnqueue` to finish

        *outputSize = 0;
        break;
    }
    case IOCTL_PMU_CTL_START:
    case IOCTL_PMU_CTL_STOP:
    case IOCTL_PMU_CTL_RESET:
    {
        struct pmu_ctl_hdr* ctl_req = (struct pmu_ctl_hdr*)pInBuffer;
        UINT8 dmc_ch_base = 0, dmc_ch_end = 0, dmc_idx = ALL_DMC_CHANNEL;
        UINT32 ctl_flags = ctl_req->flags;
        size_t cores_count = ctl_req->cores_idx.cores_count;
        int dmc_core_idx = ALL_CORE;

        // does our process own the lock?
        if (!AmILocking(IoCtlCode, file_object))
        {
            status = STATUS_INVALID_DEVICE_STATE;
            break;
        }

        if (InBufSize != sizeof(struct pmu_ctl_hdr))
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IOCTL: invalid inputsize %ld for action %d\n", InBufSize, action));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        if (cores_count == 0 || cores_count >= MAX_PMU_CTL_CORES_COUNT)
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IOCTL: invalid cores_count=%llu (must be 1-%d) for action %d\n",
                cores_count, MAX_PMU_CTL_CORES_COUNT, action));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        if (!CTRL_FLAG_VALID(ctl_req->flags))
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IOCTL: invalid flags  0x%X for action %d\n",
                ctl_req->flags, action));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        if (!check_cores_in_pmu_ctl_hdr_p(ctl_req))
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IOCTL: invalid cores_no for action %d\n", action));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        KdPrintEx((DPFLTR_IHVDRIVER_ID,  DPFLTR_INFO_LEVEL, "IOCTL: action %d\n", action));

        VOID(*core_func)(VOID) = NULL;
        VOID(*dsu_func)(VOID) = NULL;
        VOID(*dmc_func)(UINT8, UINT8, struct dmcs_desc*) = NULL;
        ULONG funcsIdx = (action - PMU_CTL_ACTION_OFFSET);
        const ULONG funcsSize = (ULONG)(sizeof(core_ctl_funcs) / sizeof(core_ctl_funcs[0]));
        
		if (funcsIdx >= funcsSize)
		{
			KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_WARNING_LEVEL, "IOCTL: invalid action %d\n", action));
			status = STATUS_INVALID_PARAMETER;
			break;
		}

        if (ctl_flags & CTL_FLAG_CORE)
            core_func = core_ctl_funcs[funcsIdx];
        if (ctl_flags & CTL_FLAG_DSU)
            dsu_func = dsu_ctl_funcs[funcsIdx];

        PWORK_ITEM_CTXT context;
        context = WdfObjectGet_WORK_ITEM_CTXT(queueContext->WorkItem);
        context->action = PMU_CTL_START;
        context->ctl_flags = ctl_flags;
        context->ctl_req = ctl_req;
        context->cores_count = cores_count;
        context->do_func = core_func;
        context->do_func2 = dsu_func;
        KdPrintEx((DPFLTR_IHVDRIVER_ID,  DPFLTR_INFO_LEVEL, "%!FUNC! %!LINE! enqueuing for action %d\n", context->action));
        WdfWorkItemEnqueue(queueContext->WorkItem);
        WdfWorkItemFlush(queueContext->WorkItem);       // Wait for `WdfWorkItemEnqueue` to finish

        if (ctl_flags & CTL_FLAG_DMC)
        {
            dmc_func = dmc_ctl_funcs[funcsIdx];
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

            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL: action PMU_CTL_START cores_count %lld\n", cores_count));
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
                    UINT8 do_core_multiplex = !!(core->events_num > (UINT32)(numFreeGPC + numFPC));
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

                const LONGLONG ns100 = -10000;  // negative, the expiration time is relative to the current system time
                LARGE_INTEGER DueTime;
                LONG Period = PMU_CTL_START_PERIOD;

                if (ctl_req->period >= PMU_CTL_START_PERIOD_MIN
                    && ctl_req->period <= PMU_CTL_START_PERIOD)
                    Period = ctl_req->period;

                DueTime.QuadPart = do_multiplex ? (Period * ns100) : (2 * (LONGLONG)Period * ns100);
                Period = do_multiplex ? Period : (2 * Period);

                KdPrintEx((DPFLTR_IHVDRIVER_ID,  DPFLTR_INFO_LEVEL, "%!FUNC! %!LINE! ctl_req->period = %d\n", ctl_req->period));
                KdPrintEx((DPFLTR_IHVDRIVER_ID,  DPFLTR_INFO_LEVEL, "%!FUNC! %!LINE! count.period = %d\n", Period));
                KdPrintEx((DPFLTR_IHVDRIVER_ID,  DPFLTR_INFO_LEVEL, "%!FUNC! %!LINE! DueTime.QuadPart = %lld\n", DueTime.QuadPart));

                PRKDPC dpc = do_multiplex ? &core->dpc_multiplex : &core->dpc_overflow;
                KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL: action PMU_CTL_START calling set timer multiplex %s for loop k  %d core idx %lld\n", do_multiplex? "TRUE":"FALSE", k, core->idx));
                KeSetTimerEx(&core->timer, DueTime, Period, dpc);
            }
        }
        else if (action == PMU_CTL_STOP)
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL: action PMU_CTL_STOP\n"));
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
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL: action PMU_CTL_RESET  cores_count %lld\n", cores_count));
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
                KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL: action PMU_CTL_RESET calling insert dpc  loop k is %d core index is %lld\n", k, core->idx));
                KeInsertQueueDpc(&core->dpc_reset, (VOID*)cores_count, NULL);  // cores_count has been validated so the DPC will always be called prior to the code below waiting on its completion
            }
            LARGE_INTEGER li;
            li.QuadPart = 0;
            KeWaitForSingleObject(&sync_reset_dpc, Executive, KernelMode, 0, &li); // wait for all dpcs to complete
            KeClearEvent(&sync_reset_dpc);
            cpunos = 0;

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
    case IOCTL_PMU_CTL_QUERY_HW_CFG:
    {
        // does our process own the lock?
        if (!AmILocking(IoCtlCode, file_object))
        {
            status = STATUS_INVALID_DEVICE_STATE;
            break;
        }

        if (InBufSize != sizeof(enum pmu_ctl_action))
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IOCTL: invalid inputsize %ld for PMU_CTL_QUERY_HW_CFG\n", InBufSize));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL: QUERY_HW_CFG\n"));

        struct hw_cfg* out = (struct hw_cfg*)pOutBuffer;
        out->core_num = (UINT16)numCores;
        out->fpc_num = numFPC;
        out->gpc_num = numFreeGPC;
        out->total_gpc_num = numGPC;
        out->pmu_ver = (dfr0_value >> 8) & 0xf;
        out->vendor_id = (midr_value >> 24) & 0xff;
        out->variant_id = (midr_value >> 20) & 0xf;
        out->arch_id = (midr_value >> 16) & 0xf;
        out->rev_id = midr_value & 0xf;
        out->part_id = (midr_value >> 4) & 0xfff;
        out->midr_value = midr_value;
        out->id_aa64dfr0_value = id_aa64dfr0_el1_value;
        RtlCopyMemory(out->counter_idx_map, counter_idx_map, sizeof(counter_idx_map));

        {   // Setup HW_CFG capability string for this driver:
            const wchar_t device_id_str[] =
                WPERF_HW_CFG_CAPS_CORE_STAT L";"
                WPERF_HW_CFG_CAPS_CORE_SAMPLE L";"
                WPERF_HW_CFG_CAPS_CORE_DSU L";"
                WPERF_HW_CFG_CAPS_CORE_DMC;

            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "*outputSize > OutBufSize\n"));

            RtlSecureZeroMemory(out->device_id_str, sizeof(out->device_id_str));
            RtlCopyMemory(out->device_id_str, device_id_str, sizeof(device_id_str));
        }

        *outputSize = sizeof(struct hw_cfg);
        if (*outputSize > OutBufSize)
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "*outputSize > OutBufSize\n"));
            status = STATUS_BUFFER_TOO_SMALL;
        }
        break;
    }
    case IOCTL_PMU_CTL_QUERY_SUPP_EVENTS:
    {
        // does our process own the lock?
        if (!AmILocking(IoCtlCode, file_object))
        {
            status = STATUS_INVALID_DEVICE_STATE;
            break;
        }

        if (InBufSize != sizeof(enum pmu_ctl_action))
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IOCTL: invalid inputsize %ld for PMU_CTL_QUERY_SUPP_EVENTS\n", InBufSize));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL: QUERY_SUPP_EVENTS\n"));

        struct evt_hdr* hdr = (struct evt_hdr*)pOutBuffer;
        hdr->evt_class = EVT_CORE;
        UINT16 core_evt_num = sizeof(armv8_arch_core_events) / sizeof(armv8_arch_core_events[0]);
        hdr->num = core_evt_num;
        int return_size = 0;

        UINT16* out = (UINT16*)((UINT8*)pOutBuffer + sizeof(struct evt_hdr));
        for (int i = 0; i < core_evt_num; i++)
            out[i] = armv8_arch_core_events[i];

        out = out + core_evt_num;
        return_size = sizeof(struct evt_hdr);
        return_size += sizeof(armv8_arch_core_events);

        if (dsu_numGPC)
        {
            hdr = (struct evt_hdr*)out;
            hdr->evt_class = EVT_DSU;
            UINT16 dsu_evt_num = 0;

            out = (UINT16*)((UINT8*)out + sizeof(struct evt_hdr));

            for (UINT16 i = 0; i < 32; i++)
            {
                if (dsu_evt_mask_lo & (1 << i))
                    out[dsu_evt_num++] = i;
            }

            for (UINT16 i = 0; i < 32; i++)
            {
                if (dsu_evt_mask_hi & (1 << i))
                    out[dsu_evt_num++] = 32 + i;
            }

            hdr->num = dsu_evt_num;

            return_size += sizeof(struct evt_hdr);
            return_size += dsu_evt_num * sizeof(UINT16);

            out = out + dsu_evt_num;
        }

        if (dmc_array.dmcs)
        {
            hdr = (struct evt_hdr*)out;
            hdr->evt_class = EVT_DMC_CLK;
            UINT16 dmc_evt_num = 0;

            out = (UINT16*)((UINT8*)out + sizeof(struct evt_hdr));

            for (UINT16 evt_idx = 0; evt_idx < DMC_CLK_EVT_NUM; evt_idx++)
                out[dmc_evt_num++] = evt_idx;

            hdr->num = dmc_evt_num;

            return_size += sizeof(struct evt_hdr);
            return_size += dmc_evt_num * sizeof(UINT16);
            out = out + dmc_evt_num;

            hdr = (struct evt_hdr*)out;
            hdr->evt_class = EVT_DMC_CLKDIV2;
            dmc_evt_num = 0;
            out = (UINT16*)((UINT8*)out + sizeof(struct evt_hdr));

            for (UINT16 evt_idx = 0; evt_idx < DMC_CLKDIV2_EVT_NUM; evt_idx++)
                out[dmc_evt_num++] = evt_idx;

            hdr->num = dmc_evt_num;

            return_size += sizeof(struct evt_hdr);
            return_size += dmc_evt_num * sizeof(UINT16);
            out = out + dmc_evt_num;
        }

        *outputSize = return_size;
        if (*outputSize > OutBufSize)
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "*outputSize > OutBufSize\n"));
            status = STATUS_BUFFER_TOO_SMALL;
        }
        break;
    }
    case IOCTL_PMU_CTL_QUERY_VERSION:
    {
        // does our process own the lock?
        if (!AmILocking(IoCtlCode, file_object))
        {
            status = STATUS_INVALID_DEVICE_STATE;
            break;
        }

        if (InBufSize != sizeof(struct pmu_ctl_ver_hdr))
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IOCTL: invalid inputsize %ld for PMU_CTL_QUERY_VERSION\n",
                InBufSize));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL: QUERY_VERSION\n"));

        struct pmu_ctl_ver_hdr* ctl_req = (struct pmu_ctl_ver_hdr*)pInBuffer;
        if (ctl_req->version.major != MAJOR || ctl_req->version.minor != MINOR
            || ctl_req->version.patch != PATCH)
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID,  DPFLTR_INFO_LEVEL, "IOCTL: version mismatch bewteen wperf-driver and wperf\n"));
            KdPrintEx((DPFLTR_IHVDRIVER_ID,  DPFLTR_INFO_LEVEL, "IOCTL: wperf-driver version: %d.%d.%d\n",
                MAJOR, MINOR, PATCH));
            KdPrintEx((DPFLTR_IHVDRIVER_ID,  DPFLTR_INFO_LEVEL, "IOCTL: wperf version: %d.%d.%d\n",
                ctl_req->version.major,
                ctl_req->version.minor,
                ctl_req->version.patch));
        }

        struct version_info* ver_info = (struct version_info*)pOutBuffer;
        ver_info->major = MAJOR;
        ver_info->minor = MINOR;
        ver_info->patch = PATCH;
        const wchar_t* gitver = WPERF_GIT_VER_STR;
        memcpy(ver_info->gitver, gitver, sizeof(WPERF_GIT_VER_STR));

        *outputSize = sizeof(struct version_info);
        if (*outputSize > OutBufSize)
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "*outputSize > OutBufSize\n"));
            status = STATUS_BUFFER_TOO_SMALL;
        }
        break;
    }
    case IOCTL_PMU_CTL_ASSIGN_EVENTS:
    {
        // does our process own the lock?
        if (!AmILocking(IoCtlCode, file_object))
        {
            status = STATUS_INVALID_DEVICE_STATE;
            break;
        }

        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL: PMU_CTL_ASSIGN_EVENTS\n"));

        struct pmu_ctl_evt_assign_hdr* ctl_req = (struct pmu_ctl_evt_assign_hdr*)pInBuffer;
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

        ULONG avail_sz = InBufSize - sizeof(struct pmu_ctl_evt_assign_hdr);
        UINT8* payload_addr = (UINT8*)pInBuffer + sizeof(struct pmu_ctl_evt_assign_hdr);

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

                    KdPrintEx((DPFLTR_IHVDRIVER_ID,  DPFLTR_INFO_LEVEL, "IOCTL: assign %d dmc_%s_events (no-multiplexing)\n",
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
    case IOCTL_PMU_CTL_READ_COUNTING:
    {
        // does our process own the lock?
        if (!AmILocking(IoCtlCode, file_object))
        {
            status = STATUS_INVALID_DEVICE_STATE;
            break;
        }

        struct pmu_ctl_hdr* ctl_req = (struct pmu_ctl_hdr*)pInBuffer;
        size_t cores_count = ctl_req->cores_idx.cores_count;
        UINT8 core_idx = ctl_req->cores_idx.cores_no[0];    // This query supports only 1 core

        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL: PMU_CTL_READ_COUNTING\n"));

        if (InBufSize != sizeof(struct pmu_ctl_hdr))
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IOCTL: invalid inputsize %ld for PMU_CTL_READ_COUNTING\n", InBufSize));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        if (cores_count != 1)
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IOCTL: invalid cores_count=%llu (must be 1) for PMU_CTL_READ_COUNTING\n", cores_count));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        if (!check_cores_in_pmu_ctl_hdr_p(ctl_req))
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IOCTL: invalid cores_no for action %d\n", action));
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
            ReadOut* out = (ReadOut*)((UINT8*)pOutBuffer + sizeof(ReadOut) * (i - core_base));
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
        if (*outputSize > OutBufSize)
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "*outputSize > OutBufSize\n"));
            status = STATUS_BUFFER_TOO_SMALL;
        }
        break;
    }
    case IOCTL_DSU_CTL_INIT:
    {
        // does our process own the lock?
        if (!AmILocking(IoCtlCode, file_object))
        {
            status = STATUS_INVALID_DEVICE_STATE;
            break;
        }

        struct dsu_ctl_hdr* ctl_req = (struct dsu_ctl_hdr*)pInBuffer;

        if (InBufSize != sizeof(struct dsu_ctl_hdr))
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IOCTL: invalid InBufSize %ld for DSU_CTL_INIT\n", InBufSize));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        dsu_numCluster = ctl_req->cluster_num;
        dsu_sizeCluster = ctl_req->cluster_size;

        KdPrintEx((DPFLTR_IHVDRIVER_ID,  DPFLTR_INFO_LEVEL, "IOCTL: DSU_CTL_INIT\n"));

        DSUProbePMU(&dsu_numGPC, &dsu_evt_mask_lo, &dsu_evt_mask_hi);
        KdPrintEx((DPFLTR_IHVDRIVER_ID,  DPFLTR_INFO_LEVEL, "dsu pmu num %d\n", dsu_numGPC));
        KdPrintEx((DPFLTR_IHVDRIVER_ID,  DPFLTR_INFO_LEVEL, "dsu pmu event mask 0x%x, 0x%x\n", dsu_evt_mask_lo, dsu_evt_mask_hi));

        struct dsu_cfg* out = (struct dsu_cfg*)pOutBuffer;

        out->fpc_num = dsu_numFPC;
        out->gpc_num = dsu_numGPC;

        *outputSize = sizeof(struct dsu_cfg);
        if (*outputSize > OutBufSize)
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "*outputSize > OutBufSize\n"));
            status = STATUS_BUFFER_TOO_SMALL;
        }
        break;
    }
    case IOCTL_DSU_CTL_READ_COUNTING:
    {
        // does our process own the lock?
        if (!AmILocking(IoCtlCode, file_object))
        {
            status = STATUS_INVALID_DEVICE_STATE;
            break;
        }

        struct pmu_ctl_hdr* ctl_req = (struct pmu_ctl_hdr*)pInBuffer;
        size_t cores_count = ctl_req->cores_idx.cores_count;
        UINT8 core_idx = ctl_req->cores_idx.cores_no[0];    // This query supports only 1 core

        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL: DSU_CTL_READ_COUNTING\n"));

        if (InBufSize != sizeof(struct pmu_ctl_hdr))
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IOCTL: invalid InBufSize %ld for DSU_CTL_READ_COUNTING\n", InBufSize));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        if (cores_count != 1)
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IOCTL: invalid cores_count=%llu (must be 1) for DSU_CTL_READ_COUNTING\n", cores_count));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        if (!CTRL_FLAG_VALID(ctl_req->flags))
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IOCTL: invalid flags 0x%X for action %d\n",
                ctl_req->flags, action));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        if (!check_cores_in_pmu_ctl_hdr_p(ctl_req))
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IOCTL: invalid cores_no for action %d\n", action));
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
            DSUReadOut* out = (DSUReadOut*)((UINT8*)pOutBuffer + sizeof(DSUReadOut) * ((i - core_base) / dsu_sizeCluster));
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
        if (*outputSize > OutBufSize)
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "*outputSize > OutBufSize\n"));
            status = STATUS_BUFFER_TOO_SMALL;
        }
        break;
    }
    case IOCTL_DMC_CTL_INIT:
    {
        // does our process own the lock?
        if (!AmILocking(IoCtlCode, file_object))
        {
            status = STATUS_INVALID_DEVICE_STATE;
            break;
        }

        struct dmc_ctl_hdr* ctl_req = (struct dmc_ctl_hdr*)pInBuffer;
        ULONG expected_size;

        dmc_array.dmc_num = ctl_req->dmc_num;
        expected_size = sizeof(struct dmc_ctl_hdr) + dmc_array.dmc_num * sizeof(UINT64) * 2;

        if (InBufSize != expected_size)
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IOCTL: invalid InBufSize %ld for DMC_CTL_INIT\n", InBufSize));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        if (!dmc_array.dmcs)
        {
            dmc_array.dmcs = (struct dmc_desc*)ExAllocatePool2(POOL_FLAG_NON_PAGED, sizeof(struct dmc_desc) * ctl_req->dmc_num, 'DMCR');
            if (!dmc_array.dmcs)
            {
                KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "DMC_CTL_INIT: allocate dmcs failed\n"));
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
                    ExFreePool(dmc_array.dmcs);
                    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IOCTL: MmMapIoSpace failed\n"));
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

        KdPrintEx((DPFLTR_IHVDRIVER_ID,  DPFLTR_INFO_LEVEL, "IOCTL: DMC_CTL_INIT\n"));

        struct dmc_cfg* out = (struct dmc_cfg*)pOutBuffer;
        out->clk_fpc_num = 0;
        out->clk_gpc_num = DMC_CLK_NUMGPC;
        out->clkdiv2_fpc_num = 0;
        out->clkdiv2_gpc_num = DMC_CLKDIV2_NUMGPC;
        *outputSize = sizeof(struct dmc_cfg);
        if (*outputSize > OutBufSize)
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "*outputSize > OutBufSize\n"));
            status = STATUS_BUFFER_TOO_SMALL;
        }
        break;
    }
    case IOCTL_DMC_CTL_READ_COUNTING:
    {
        // does our process own the lock?
        if (!AmILocking(IoCtlCode, file_object))
        {
            status = STATUS_INVALID_DEVICE_STATE;
            break;
        }

        struct pmu_ctl_hdr* ctl_req = (struct pmu_ctl_hdr*)pInBuffer;
        UINT8 dmc_idx = ctl_req->dmc_idx;

        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL: DMC_CTL_READ_COUNTING\n"));

        if (InBufSize != sizeof(struct pmu_ctl_hdr))
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IOCTL: invalid InBufSize %ld for DMC_CTL_READ_COUNTING\n", InBufSize));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        if (!CTRL_FLAG_VALID(ctl_req->flags))
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IOCTL: invalid flags  0x%X for action %d\n",
                ctl_req->flags, action));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        if (dmc_idx != ALL_DMC_CHANNEL && dmc_idx >= dmc_array.dmc_num)
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IOCTL: invalid dmc_idx %d for DMC_CTL_READ_COUNTING\n", dmc_idx));
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
            DMCReadOut* out = (DMCReadOut*)((UINT8*)pOutBuffer + sizeof(DMCReadOut) * (i - dmc_ch_base));
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
        if (*outputSize > OutBufSize)
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "*outputSize > OutBufSize\n"));
            status = STATUS_BUFFER_TOO_SMALL;
        }
        break;
    }
    default:
        KdPrintEx((DPFLTR_IHVDRIVER_ID,  DPFLTR_INFO_LEVEL, "IOCTL: invalid action %d\n", action));
        status = STATUS_INVALID_PARAMETER;
        *outputSize = 0;
        break;
    }

    return status;
}
