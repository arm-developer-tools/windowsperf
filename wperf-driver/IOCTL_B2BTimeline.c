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
#if !defined DBG
#include "IOCTL_B2BTimeline.tmh"
#endif
#include "coreinfo.h"
#include "dmc.h"
#include "dsu.h"
#include "core.h"
#include "utilities.h"

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
static UINT16 armv8_arch_core_events[] =
{
#define WPERF_ARMV8_ARCH_EVENTS(n,a,b,c,d) b,
#include "wperf-common\armv8-arch-events.def"
#undef WPERF_ARMV8_ARCH_EVENTS
};

extern char has_dsu;
extern char has_dmc;




//    Data list
extern KSPIN_LOCK B2BLock;
extern B2BData         b2b_data[];

NTSTATUS deviceControlB2BTimeline(
    _In_    ULONG   IoCtlCode,
    _In_    PVOID   pInBuffer,
    _In_    ULONG   InBufSize,
    _In_opt_    PVOID   pOutBuffer,
    _In_    ULONG   OutBufSize,
    _Out_   PULONG  outputSize,
    _Inout_ PQUEUE_CONTEXT queueContext
)
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG action = (IoCtlCode >> 2) & 0xFFF;   // 12 bits are the 'Funciton'
    UNREFERENCED_PARAMETER(queueContext);
    UNREFERENCED_PARAMETER(OutBufSize);
    UNREFERENCED_PARAMETER(pOutBuffer);
    *outputSize = 0;

    switch (IoCtlCode)
    {

    case IOCTL_PMU_CTL_B2BTIMELINE_START:
    {
        struct pmu_b2b_timeline_hdr* ctl_req = (struct pmu_b2b_timeline_hdr*)pInBuffer;
        UINT8 dmc_ch_base = 0, dmc_ch_end = 0, dmc_idx = ALL_DMC_CHANNEL;
        UINT32 ctl_flags = ctl_req->flags;
        size_t cores_count = ctl_req->cores_idx.cores_count;
        int dmc_core_idx = ALL_CORE;

        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL_PMU_CTL_B2BTIMELINE_START cores_count %lld\n", cores_count));

        if (InBufSize != sizeof(struct pmu_b2b_timeline_hdr))
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

        if (!check_cores_in_pmu_ctl_hdr_p((struct pmu_ctl_hdr* )ctl_req))
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IOCTL: invalid cores_no for action %d\n", action));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        VOID(*core_func)(VOID) = NULL;
        VOID(*dsu_func)(VOID) = NULL;
        VOID(*dmc_func)(UINT8, UINT8, struct dmcs_desc*) = NULL;

        if (ctl_flags & CTL_FLAG_CORE)
            core_func = CoreCounterStart;
        if (ctl_flags & CTL_FLAG_DSU)
            dsu_func = DSUCounterStart;

        PWORK_ITEM_CTXT context;
        context = WdfObjectGet_WORK_ITEM_CTXT(queueContext->WorkItem);
        context->action = PMU_CTL_START;
        context->do_func = core_func;
        context->do_func2 = dsu_func;

        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "%!FUNC! %!LINE! enqueuing for action %d\n", context->action));
        WdfWorkItemEnqueue(queueContext->WorkItem);

        if ((ctl_flags & CTL_FLAG_DMC))
        {
            dmc_func = DmcCounterStart;
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


        for (auto k = 0; k < cores_count; k++)
        {
            int i = ctl_req->cores_idx.cores_no[k];
            CoreInfo* core = &core_info[i];
            if (core->timer_running)
            {
                KeCancelTimer(&core->timer);
                core->timer_running = 0;
            }


            if (core->b2b_timer_running)
            {
                KeCancelTimer(&core->b2b_timer);
                core->b2b_timer_running = 0;
            }

            core->prof_core = PROF_DISABLED;
            core->prof_dsu = PROF_DISABLED;
            core->prof_dmc = PROF_DISABLED;
            core->b2b_timeout = 0;

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
            
            core->b2b_timeout = ctl_req->b2b_timeout * 100; // each one is 100 ms


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

            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "%!FUNC! %!LINE! ctl_req->period = %d\n", ctl_req->period));
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "%!FUNC! %!LINE! count.period = %d\n", Period));
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "%!FUNC! %!LINE! DueTime.QuadPart = %lld\n", DueTime.QuadPart));

            PRKDPC dpc = do_multiplex ? &core->dpc_multiplex : &core->dpc_overflow;
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL_PMU_CTL_B2BTIMELINE_START calling set timer multiplex %s for loop k  %d core idx %lld\n", do_multiplex ? "TRUE" : "FALSE", k, core->idx));


            LARGE_INTEGER b2b_DueTime;
            b2b_DueTime.QuadPart = core->b2b_timeout * ns100;
            KeInitializeTimer(&core->b2b_timer);
            core->b2b_timer_running = 1;
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "%!FUNC! %!LINE! core->b2b_timeout total milliseconds = %lld\n", core->b2b_timeout));
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "%!FUNC! %!LINE! b2b_DueTime.QuadPart = %lld\n", b2b_DueTime.QuadPart));


            KeSetTimerEx(&core->b2b_timer, b2b_DueTime, (ULONG)core->b2b_timeout, &core->dpc_b2b);

            KeSetTimerEx(&core->timer, DueTime, Period, dpc);
        }

        *outputSize = 0;
        break;
    }
    
    case IOCTL_PMU_CTL_B2BTIMELINE_STOP:
    {
        struct pmu_ctl_hdr* ctl_req = (struct pmu_ctl_hdr*)pInBuffer;
        UINT8 dmc_ch_base = 0, dmc_ch_end = 0, dmc_idx = ALL_DMC_CHANNEL;
        UINT32 ctl_flags = ctl_req->flags;
        size_t cores_count = ctl_req->cores_idx.cores_count;
        int dmc_core_idx = ALL_CORE;

        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL_PMU_CTL_B2BTIMELINE_STOP\n"));

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


        VOID(*core_func)(VOID) = NULL;
        VOID(*dsu_func)(VOID) = NULL;
        VOID(*dmc_func)(UINT8, UINT8, struct dmcs_desc*) = NULL;

        if (ctl_flags & CTL_FLAG_CORE)
            core_func = CoreCounterStop;
        if (ctl_flags & CTL_FLAG_DSU)
            dsu_func = DSUCounterStop;

        PWORK_ITEM_CTXT context;
        context = WdfObjectGet_WORK_ITEM_CTXT(queueContext->WorkItem);
        context->action = PMU_CTL_START;
        context->do_func = core_func;
        context->do_func2 = dsu_func;

        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "%!FUNC! %!LINE! enqueuing for action %d\n", context->action));

        WdfWorkItemEnqueue(queueContext->WorkItem);

        if ((ctl_flags & CTL_FLAG_DMC))
        {
            dmc_func = DmcCounterStart;
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


        for (auto k = 0; k < cores_count; k++)
        {
            int i = ctl_req->cores_idx.cores_no[k];
            CoreInfo* core = &core_info[i];
            if (core->timer_running)
            {
                KeCancelTimer(&core->timer);
                core->timer_running = 0;
            }

            if (core->b2b_timer_running)
            {
                KeCancelTimer(&core->b2b_timer);
                core->b2b_timer_running = 0;
            }
        }
        *outputSize = 0;

        // finally clear down the data list
        break;
    }

    
    case IOCTL_PMU_CTL_B2BTIMELINE_GET:
    {
        struct pmu_ctl_hdr* ctl_req = (struct pmu_ctl_hdr*)pInBuffer;
        size_t cores_count = ctl_req->cores_idx.cores_count;
        UINT8 core_idx = ctl_req->cores_idx.cores_no[0];    // This query supports only 1 core at a time

        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL_PMU_CTL_B2BTIMELINE_GET\n"));

        if (InBufSize != sizeof(struct pmu_ctl_hdr))
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IOCTL: invalid inputsize %ld for PMU_CTL_B2BTIMELINE_GE\n", InBufSize));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        if (cores_count != 1)
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IOCTL: invalid cores_count=%llu (must be 1) for PMU_CTL_B2BTIMELINE_GE\n", cores_count));
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

        KIRQL oldirql;
        KeAcquireSpinLock(&B2BLock, &oldirql);

        for (UINT32 i = core_base; i < core_end; i++)
        {
            ReadOut* out = (ReadOut*)((UINT8*)pOutBuffer + sizeof(ReadOut) * (i - core_base));

            UINT32 events_num = b2b_data[i].evt_num;
            out->evt_num = events_num;
            out->round = b2b_data[i].round;

            struct pmu_event_usr* out_events = &out->evts[0];
            struct pmu_event_usr* events = &b2b_data[i].evts[0];

            for (UINT32 j = 0; j < events_num; j++)
            {
                struct pmu_event_usr* event = events + j;
                struct pmu_event_usr* out_event = out_events + j;
                out_event->event_idx = event->event_idx;
                out_event->filter_bits = event->filter_bits;
                out_event->scheduled = event->scheduled;
                out_event->value = event->value;
            }

            outputSizeReturned += sizeof(ReadOut);
        }

        KeReleaseSpinLock(&B2BLock, oldirql);

        *outputSize = outputSizeReturned;
        if (*outputSize > OutBufSize)
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "*outputSize > OutBufSize\n"));
            status = STATUS_BUFFER_TOO_SMALL;
        }
        break;
    }
    
    default:
        status = STATUS_INVALID_PARAMETER;
    }

return status;
}
