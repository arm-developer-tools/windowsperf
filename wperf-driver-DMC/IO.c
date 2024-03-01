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
#include "IO.tmh"
#endif
#include "..\wperf-common\inline.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, WperfDriver_TIOInitialize)
#endif

NTSTATUS
WperfDriver_TIOInitialize(
    _In_ WDFDEVICE Device,
    _In_ PDEVICE_EXTENSION devExt
    )
/*++

Routine Description:

     The I/O dispatch callbacks for the frameworks device object
     are configured in this function.

     A single default I/O Queue is configured for parallel request
     processing, and a driver context memory allocation is created
     to hold our structure QUEUE_CONTEXT.

Arguments:

    Device - Handle to a framework device object.

Return Value:

    VOID

--*/
{
    WDFQUEUE queue;
    NTSTATUS status;
    WDF_IO_QUEUE_CONFIG    queueConfig;
    WDF_OBJECT_ATTRIBUTES  queueAttributes;
    PQUEUE_CONTEXT queueContext;


    PAGED_CODE();

    //
    // Configure a default queue so that requests that are not
    // configure-fowarded using WdfDeviceConfigureRequestDispatching to goto
    // other queues get dispatched here.
    //
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(
         &queueConfig,
        WdfIoQueueDispatchParallel
        );

    queueConfig.EvtIoDeviceControl = WperfDriver_TEvtIoDeviceControl;
    queueConfig.EvtIoStop = WperfDriver_TEvtIoStop;


    // Fill in our QUEUE_CONTEXT size
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&queueAttributes, QUEUE_CONTEXT);

    // Set synchronization scope so only one call to the queue at a time is allowed
    queueAttributes.SynchronizationScope = WdfSynchronizationScopeQueue;


    status = WdfIoQueueCreate(
                 Device,
                 &queueConfig,
                 &queueAttributes,
                 &queue
                 );

    if(!NT_SUCCESS(status)) 
    {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "WdfIoQueueCreate failed 0x%X %S", status, DbgStatusStr(status)));
        return status;
    }

    queueContext = GetQueueContext(queue);
    queueContext->devExt = devExt;

    return status;
}



//static VOID(*core_ctl_funcs[3])(VOID) = { CoreCounterStart, CoreCounterStop, CoreCounterReset };
//static VOID(*dsu_ctl_funcs[3])(VOID) = { DSUCounterStart, DSUCounterStop, DSUCounterReset };
static VOID(*dmc_ctl_funcs[3])(UINT8, UINT8, struct dmcs_desc*) = { DmcCounterStart, DmcCounterStop, DmcCounterReset };


VOID
WperfDriver_TEvtIoDeviceControl(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode
)
/*++

Routine Description:

    This event is invoked when the framework receives IRP_MJ_DEVICE_CONTROL request.

Arguments:

    Queue -  Handle to the framework queue object that is associated with the
             I/O request.

    Request - Handle to a framework request object.

    OutputBufferLength - Size of the output buffer in bytes

    InputBufferLength - Size of the input buffer in bytes

    IoControlCode - I/O control code.

Return Value:

    VOID

--*/
{
    NTSTATUS                   status = STATUS_SUCCESS;
    PQUEUE_CONTEXT      queueContext = GetQueueContext(Queue);
    PDEVICE_EXTENSION devExt = queueContext->devExt;
    WDFFILEOBJECT         fileObject = WdfRequestGetFileObject(Request);
    WDFMEMORY              memory;
    PVOID                        inBuffer = 0;
    PVOID                        outBuffer = 0;
    ULONGLONG               retDataSize = 0;

    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "WperfDriver_TEvtIoDeviceControl : %s \n", GetIoctlStr(IoControlCode)));



    if (InputBufferLength)
    {
        status = WdfRequestRetrieveInputMemory(Request, &memory);
        if (!NT_SUCCESS(status))
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "%!FUNC! %!LINE! Could not get request in memory buffer 0x%x\n",
                status));
            WdfRequestComplete(Request, status);
            return;
        }
        inBuffer = WdfMemoryGetBuffer(memory, NULL);
    }


    if (OutputBufferLength)
    {
        status = WdfRequestRetrieveOutputMemory(Request, &memory);
        if (!NT_SUCCESS(status))
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "%!FUNC! %!LINE! Could not get request in memory buffer 0x%x\n",
                status));
            WdfRequestComplete(Request, status);
            return;
        }
        outBuffer = WdfMemoryGetBuffer(memory, NULL);
    }

    switch (IoControlCode)
    {
    case IOCTL_PMU_CTL_LOCK_ACQUIRE:
    {
        if (InputBufferLength != sizeof(struct lock_request))
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "invalid inputsize %ld for IOCTL_PMU_CTL_LOCK_ACQUIRE\n", InputBufferLength));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        if (OutputBufferLength != sizeof(enum status_flag))
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "invalid outputsize %ld for IOCTL_PMU_CTL_LOCK_ACQUIRE\n", OutputBufferLength));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL_PMU_CTL_LOCK_ACQUIRE for file object %p\n", fileObject));

        struct lock_request* in = (struct lock_request*)inBuffer;
        enum status_flag out = STS_BUSY;

        if (in->flag == LOCK_GET_FORCE)
        {
            out = STS_LOCK_AQUIRED;
            SetMeBusyForce(devExt, IoControlCode, fileObject);
        }
        else if (in->flag == LOCK_GET)
        {
            if (SetMeBusy(devExt, IoControlCode, fileObject)) // returns failure if the lock is already held by another process
                out = STS_LOCK_AQUIRED;
            // Note: else STS_BUSY;
        }
        else
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "invalid flag %d for IOCTL_PMU_CTL_LOCK_ACQUIRE\n", in->flag));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        *((enum status_flag*)outBuffer) = out;
        retDataSize = sizeof(enum status_flag);
        break;
    }

    case IOCTL_PMU_CTL_LOCK_RELEASE:
    {
        if (InputBufferLength != sizeof(struct lock_request))
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "invalid inputsize %ld for IOCTL_PMU_CTL_LOCK_RELEASE\n", InputBufferLength));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        if (OutputBufferLength != sizeof(enum status_flag))
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "invalid outputsize %ld for IOCTL_PMU_CTL_LOCK_RELEASE\n", OutputBufferLength));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL_PMU_CTL_LOCK_RELEASE for fileObject %p\n", fileObject));

        struct lock_request* in = (struct lock_request*)inBuffer;
        enum status_flag out = STS_BUSY;

        if (in->flag == LOCK_RELEASE)
        {
            if (SetMeIdle(devExt, fileObject)) // returns fialure if this process doesnt own the lock 
                out = STS_IDLE;         // All went well and we went IDLE
            // Note: else out = STS_BUSY;     // This is illegal, as we are not IDLE
        }
        else
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "invalid flag %d for IOCTL_PMU_CTL_LOCK_RELEASE\n", in->flag));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        *((enum status_flag*)outBuffer) = out;
        retDataSize = sizeof(enum status_flag);
        break;
    }
    case IOCTL_PMU_CTL_START:
    case IOCTL_PMU_CTL_STOP:
    case IOCTL_PMU_CTL_RESET:
    {
        struct pmu_ctl_hdr* ctl_req = (struct pmu_ctl_hdr*)inBuffer;
        UINT8 dmc_ch_base = 0, dmc_ch_end = 0, dmc_idx = ALL_DMC_CHANNEL;
        size_t cores_count = ctl_req->cores_idx.cores_count;
        int dmc_core_idx = ALL_CORE;
        ULONG funcsIdx = 0;

        // does our process own the lock?
        if (!AmILocking(devExt, IoControlCode, fileObject))
        {
            status = STATUS_INVALID_DEVICE_STATE;
            break;
        }

        if (InputBufferLength != sizeof(struct pmu_ctl_hdr))
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IOCTL: invalid inputsize %ld for IoControlCode %d\n", InputBufferLength, GetIoctlStr(IoControlCode)));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        if (cores_count == 0 || cores_count >= MAX_PMU_CTL_CORES_COUNT)
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IOCTL: invalid cores_count=%llu (must be 1-%d) for IoControlCode %d\n",
                cores_count, MAX_PMU_CTL_CORES_COUNT, GetIoctlStr(IoControlCode)));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        if (!(ctl_req->flags & CTL_FLAG_DMC))
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IOCTL: invalid flags  0x%X for IoControlCode %d\n",
                ctl_req->flags, GetIoctlStr(IoControlCode)));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        if (!check_cores_in_pmu_ctl_hdr_p(ctl_req))
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IOCTL: invalid cores_no for IoControlCode %d\n", GetIoctlStr(IoControlCode)));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL: IoControlCode %d\n", GetIoctlStr(IoControlCode)));

        VOID(*core_func)(VOID) = NULL;
        VOID(*dsu_func)(VOID) = NULL;
        VOID(*dmc_func)(UINT8, UINT8, struct dmcs_desc*) = NULL;
        if (IoControlCode == IOCTL_PMU_CTL_START)
            funcsIdx = 0;
        if (IoControlCode == IOCTL_PMU_CTL_STOP)
            funcsIdx = 1;
        if (IoControlCode == IOCTL_PMU_CTL_RESET)
            funcsIdx = 2;



        PWORK_ITEM_CTXT context;
        context = WdfObjectGet_WORK_ITEM_CTXT(queueContext->WorkItem);
        context->action = PMU_CTL_START;
        context->do_func = core_func;
        context->do_func2 = dsu_func;
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "%!FUNC! %!LINE! enqueuing for action %d\n", context->action));
        WdfWorkItemEnqueue(queueContext->WorkItem);

        //if (ctl_req->flags & CTL_FLAG_DMC) // we have to be DMC
        {
            dmc_func = dmc_ctl_funcs[funcsIdx];
            dmc_idx = ctl_req->dmc_idx;

            if (dmc_idx == ALL_DMC_CHANNEL)
            {
                dmc_ch_base = 0;
                dmc_ch_end = devExt->dmc_array.dmc_num;
            }
            else
            {
                dmc_ch_base = dmc_idx;
                dmc_ch_end = dmc_idx + 1;
            }

            DmcChannelIterator(dmc_ch_base, dmc_ch_end, dmc_func, &devExt->dmc_array);
            // Use last core from all used.
            dmc_core_idx = ctl_req->cores_idx.cores_no[cores_count - 1];
        }

        if (IoControlCode == IOCTL_PMU_CTL_START)
        {

            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL_PMU_CTL_START cores_count %lld\n", cores_count));
            for (auto k = 0; k < cores_count; k++)
            {
                int i = ctl_req->cores_idx.cores_no[k];
                CoreInfo* core = &devExt->core_info[i];
                if (core->timer_running)
                {
                    KeCancelTimer(&core->timer);
                    core->timer_running = 0;
                }

                core->prof_core = PROF_DISABLED;
                core->prof_dsu = PROF_DISABLED;
                core->prof_dmc = PROF_DISABLED;

                UINT8 do_multiplex = 0;

                if (ctl_req->flags & CTL_FLAG_CORE)
                {
                    UINT8 do_core_multiplex = !!(core->events_num > (UINT32)(devExt->numFreeGPC + numFPC));
                    core->prof_core = do_core_multiplex ? PROF_MULTIPLEX : PROF_NORMAL;
                    do_multiplex |= do_core_multiplex;
                }

                if (ctl_req->flags & CTL_FLAG_DSU)
                {
                    UINT8 dsu_cluster_head = !(i & (devExt->dsu_sizeCluster - 1));
                    if (cores_count != devExt->numCores || dsu_cluster_head)
                    {
                        UINT8 do_dsu_multiplex = !!(core->dsu_events_num > (UINT32)(devExt->dsu_numGPC + dsu_numFPC));
                        core->prof_dsu = do_dsu_multiplex ? PROF_MULTIPLEX : PROF_NORMAL;
                        do_multiplex |= do_dsu_multiplex;
                    }
                }

                if ((ctl_req->flags & CTL_FLAG_DMC) && i == dmc_core_idx)
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

                KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "%!FUNC! %!LINE! ctl_req->period = %d\n", ctl_req->period));
                KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "%!FUNC! %!LINE! count.period = %d\n", Period));
                KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "%!FUNC! %!LINE! DueTime.QuadPart = %lld\n", DueTime.QuadPart));

                PRKDPC dpc = do_multiplex ? &core->dpc_multiplex : &core->dpc_overflow;
                KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL_PMU_CTL_START calling set timer multiplex %s for loop k  %d core idx %lld\n", do_multiplex ? "TRUE" : "FALSE", k, core->idx));
                KeSetTimerEx(&core->timer, DueTime, Period, dpc);
            }
        }
        else if (IoControlCode == IOCTL_PMU_CTL_STOP)
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL_PMU_CTL_STOP\n"));
            for (auto k = 0; k < cores_count; k++)
            {
                int i = ctl_req->cores_idx.cores_no[k];
                CoreInfo* core = &devExt->core_info[i];
                if (core->timer_running)
                {
                    KeCancelTimer(&core->timer);
                    core->timer_running = 0;
                }
            }
        }
        else if (IoControlCode == IOCTL_PMU_CTL_RESET)
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL_PMU_CTL_RESET  cores_count %lld\n", cores_count));
            for (auto k = 0; k < cores_count; k++)
            {
                int i = ctl_req->cores_idx.cores_no[k];
                CoreInfo* core = &devExt->core_info[i];
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
                KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL_PMU_CTL_RESET calling insert dpc  loop k is %d core index is %lld\n", k, core->idx));
                KeInsertQueueDpc(&core->dpc_reset, (VOID*)cores_count, NULL);  // cores_count has been validated so the DPC will always be called prior to the code below waiting on its completion
            }
            LARGE_INTEGER li;
            li.QuadPart = 0;
            KeWaitForSingleObject(&devExt->sync_reset_dpc, Executive, KernelMode, 0, &li); // wait for all dpcs to complete
            KeClearEvent(&devExt->sync_reset_dpc);
            devExt->cpunos = 0;

            if (ctl_req->flags & CTL_FLAG_DMC)
            {
                for (UINT8 ch_idx = dmc_ch_base; ch_idx < dmc_ch_end; ch_idx++)
                {
                    struct dmc_desc* dmc = devExt->dmc_array.dmcs + ch_idx;
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

        retDataSize = 0;
        break;
    }

    case IOCTL_DMC_CTL_INIT:
    {
        // does our process own the lock?
        if (!AmILocking(devExt,  IoControlCode, fileObject))
        {
            status = STATUS_INVALID_DEVICE_STATE;
            break;
        }

        struct dmc_ctl_hdr* ctl_req = (struct dmc_ctl_hdr*)inBuffer;
        ULONG expected_size;

        devExt->dmc_array.dmc_num = ctl_req->dmc_num;
        expected_size = sizeof(struct dmc_ctl_hdr) + devExt->dmc_array.dmc_num * sizeof(UINT64) * 2;

        if (InputBufferLength != expected_size)
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IOCTL: invalid InputBufferLength %ld for DMC_CTL_INIT\n", InputBufferLength));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        if (!devExt->dmc_array.dmcs)
        {
            devExt->dmc_array.dmcs = (struct dmc_desc*)ExAllocatePool2(POOL_FLAG_NON_PAGED, sizeof(struct dmc_desc) * ctl_req->dmc_num, 'DMCR');
            if (!devExt->dmc_array.dmcs)
            {
                KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "DMC_CTL_INIT: allocate dmcs failed\n"));
                status = STATUS_INVALID_PARAMETER;
                break;
            }

            for (UINT8 i = 0; i < devExt->dmc_array.dmc_num; i++)
            {
                UINT64 iomem_len = ctl_req->addr[2 * i + 1] - ctl_req->addr[2 * i] + 1;
                PHYSICAL_ADDRESS phy_addr;
                phy_addr.QuadPart = ctl_req->addr[2 * i];
                devExt->dmc_array.dmcs[i].iomem_start = (UINT64)MmMapIoSpace(phy_addr, iomem_len, MmNonCached);
                if (!devExt->dmc_array.dmcs[i].iomem_start)
                {
                    ExFreePool(devExt->dmc_array.dmcs);
                    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IOCTL: MmMapIoSpace failed\n"));
                    status = STATUS_INVALID_PARAMETER;
                    break;
                }
                devExt->dmc_array.dmcs[i].iomem_len = iomem_len;
                devExt->dmc_array.dmcs[i].clk_events_num = 0;
                devExt->dmc_array.dmcs[i].clkdiv2_events_num = 0;
            }
            if (status != STATUS_SUCCESS)
                break;
        }

        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL: DMC_CTL_INIT\n"));

        struct dmc_cfg* out = (struct dmc_cfg*)outBuffer;
        out->clk_fpc_num = 0;
        out->clk_gpc_num = DMC_CLK_NUMGPC;
        out->clkdiv2_fpc_num = 0;
        out->clkdiv2_gpc_num = DMC_CLKDIV2_NUMGPC;
        retDataSize = sizeof(struct dmc_cfg);
        if (retDataSize > OutputBufferLength)
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "retDataSize > OutputBufferLength\n"));
            status = STATUS_BUFFER_TOO_SMALL;
        }
        break;
    }
    case IOCTL_DMC_CTL_READ_COUNTING:
    {
        // does our process own the lock?
        if (!AmILocking(devExt, IoControlCode, fileObject))
        {
            status = STATUS_INVALID_DEVICE_STATE;
            break;
        }

        struct pmu_ctl_hdr* ctl_req = (struct pmu_ctl_hdr*)inBuffer;
        UINT8 dmc_idx = ctl_req->dmc_idx;

        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL: DMC_CTL_READ_COUNTING\n"));

        if (InputBufferLength != sizeof(struct pmu_ctl_hdr))
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IOCTL: invalid InputBufferLength %ld for DMC_CTL_READ_COUNTING\n", InputBufferLength));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        if (!CTRL_FLAG_VALID(ctl_req->flags))
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IOCTL: invalid flags  0x%X \n", ctl_req->flags));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        if (dmc_idx != ALL_DMC_CHANNEL && dmc_idx >= devExt->dmc_array.dmc_num)
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IOCTL: invalid dmc_idx %d for DMC_CTL_READ_COUNTING\n", dmc_idx));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        ULONG outputSizeExpect, outputSizeReturned;

        if (dmc_idx == ALL_DMC_CHANNEL)
            outputSizeExpect = sizeof(DMCReadOut) * devExt->dmc_array.dmc_num;
        else
            outputSizeExpect = sizeof(DMCReadOut);

        UINT8 dmc_ch_base, dmc_ch_end;

        if (dmc_idx == ALL_DMC_CHANNEL)
        {
            dmc_ch_base = 0;
            dmc_ch_end = devExt->dmc_array.dmc_num;
        }
        else
        {
            dmc_ch_base = dmc_idx;
            dmc_ch_end = dmc_idx + 1;
        }

        outputSizeReturned = 0;

        for (UINT8 i = dmc_ch_base; i < dmc_ch_end; i++)
        {
            struct dmc_desc* dmc = devExt->dmc_array.dmcs + i;
            DMCReadOut* out = (DMCReadOut*)((UINT8*)outBuffer + sizeof(DMCReadOut) * (i - dmc_ch_base));
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

        retDataSize = outputSizeReturned;
        if (retDataSize > OutputBufferLength)
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "retDataSize > OutputBufferLength\n"));
            status = STATUS_BUFFER_TOO_SMALL;
        }
        break;
    }
    default:
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL: invalid %d\n", IoControlCode));
        status = STATUS_INVALID_PARAMETER;
        retDataSize = 0;
        break;
    }

    WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, retDataSize);

    return;
}

VOID
WperfDriver_TEvtIoStop(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ ULONG ActionFlags
)
/*++

Routine Description:

    This event is invoked for a power-managed queue before the device leaves the working state (D0).

Arguments:

    Queue -  Handle to the framework queue object that is associated with the
             I/O request.

    Request - Handle to a framework request object.

    ActionFlags - A bitwise OR of one or more WDF_REQUEST_STOP_ACTION_FLAGS-typed flags
                  that identify the reason that the callback function is being called
                  and whether the request is cancelable.

Return Value:

    VOID

--*/
{
    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL,
                "%!FUNC! Queue 0x%p, Request 0x%p ActionFlags %d", 
                Queue, Request, ActionFlags));

#if !defined DBG
    UNREFERENCED_PARAMETER(Queue);
    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(ActionFlags);
#endif

    //
    // In most cases, the EvtIoStop callback function completes, cancels, or postpones
    // further processing of the I/O request.
    //
    // Typically, the driver uses the following rules:
    //
    // - If the driver owns the I/O request, it calls WdfRequestUnmarkCancelable
    //   (if the request is cancelable) and either calls WdfRequestStopAcknowledge
    //   with a Requeue value of TRUE, or it calls WdfRequestComplete with a
    //   completion status value of STATUS_SUCCESS or STATUS_CANCELLED.
    //
    //   Before it can call these methods safely, the driver must make sure that
    //   its implementation of EvtIoStop has exclusive access to the request.
    //
    //   In order to do that, the driver must synchronize access to the request
    //   to prevent other threads from manipulating the request concurrently.
    //   The synchronization method you choose will depend on your driver's design.
    //
    //   For example, if the request is held in a shared context, the EvtIoStop callback
    //   might acquire an internal driver lock, take the request from the shared context,
    //   and then release the lock. At this point, the EvtIoStop callback owns the request
    //   and can safely complete or requeue the request.
    //
    // - If the driver has forwarded the I/O request to an I/O target, it either calls
    //   WdfRequestCancelSentRequest to attempt to cancel the request, or it postpones
    //   further processing of the request and calls WdfRequestStopAcknowledge with
    //   a Requeue value of FALSE.
    //
    // A driver might choose to take no action in EvtIoStop for requests that are
    // guaranteed to complete in a small amount of time.
    //
    // In this case, the framework waits until the specified request is complete
    // before moving the device (or system) to a lower power state or removing the device.
    // Potentially, this inaction can prevent a system from entering its hibernation state
    // or another low system power state. In extreme cases, it can cause the system
    // to crash with bugcheck code 9F.
    //

    return;
}
