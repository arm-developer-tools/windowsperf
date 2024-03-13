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

VOID EvtWorkItemFunc(WDFWORKITEM WorkItem);

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, WperfDriver_TIOInitialize)
#endif



// This file containe the 'queue' initialisation and since the queue sets the entry point for IOCTLS, the IOCTL handler is in this file too. 
// In the same vein, device.c creates the device, and it the device sets the open and close entry points, so those are in that file
// IOCTL handlers tend to be quite big, especially so since often the Irp needs handling on the way back up from a lower device, 
//  so there is twice as much code, one section sends it down, the other handles it on the way up, as the lower driver, usually a PDO, sets some values in the Irp



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

    // store a reference to the queue context in our device extension
    devExt->queContext = queueContext;

    WDF_WORKITEM_CONFIG workItemConfig;
    WDF_OBJECT_ATTRIBUTES workItemAttributes;

    WDF_WORKITEM_CONFIG_INIT(&workItemConfig, EvtWorkItemFunc);
    workItemConfig.AutomaticSerialization = FALSE;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&workItemAttributes, WORK_ITEM_CTXT);
    workItemAttributes.ParentObject = queue;

    NTSTATUS wiStatus = WdfWorkItemCreate(&workItemConfig, &workItemAttributes, &queueContext->WorkItem);
    if (!NT_SUCCESS(wiStatus))
    {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "WdfWorkItemCreate failed 0x%x\n", wiStatus));
        return status;
    }

    PWORK_ITEM_CTXT workItemCtxt;
    workItemAttributes.ParentObject = NULL;
    wiStatus = WdfObjectAllocateContext(queueContext->WorkItem, &workItemAttributes, &workItemCtxt);
    if (!NT_SUCCESS(wiStatus))
    {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "WdfObjectAllocateContext failed 0x%x\n", wiStatus));
        return status;
    }

    return status;
}

static UINT16 armv8_arch_core_events[] =
{
#define WPERF_ARMV8_ARCH_EVENTS(n,a,b,c,d) b,
#include "wperf-common\armv8-arch-events.def"
#undef WPERF_ARMV8_ARCH_EVENTS
};


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
            LONG oldval = InterlockedExchange(&devExt->current_status.pmu_held, 1);
            if (oldval == 0)
                get_pmu_resource(devExt);

            SetMeBusyForce(devExt, IoControlCode, fileObject);
        }
        else if (in->flag == LOCK_GET)
        {
            if (SetMeBusy(devExt, IoControlCode, fileObject)) // returns failure if the lock is already held by another process
            {
                LONG oldval = InterlockedExchange(&devExt->current_status.pmu_held, 1);
                if (oldval == 0)
                    get_pmu_resource(devExt);
                out = STS_LOCK_AQUIRED;
                // Note: else STS_BUSY;
            }
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
            if (SetMeIdle(devExt, fileObject)) // returns failure if this process doesnt own the lock 
            {
                out = STS_IDLE;         // All went well and we went IDLE
                LONG oldval = InterlockedExchange(&devExt->current_status.pmu_held, 0);
                if (oldval == 1)
                    free_pmu_resource(devExt);
            }
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

    case IOCTL_PMU_CTL_QUERY_HW_CFG:
    {
        // does our process own the lock?
        if (!AmILocking(devExt, IoControlCode, fileObject))
        {
            status = STATUS_INVALID_DEVICE_STATE;
            break;
        }

        if (InputBufferLength != sizeof(enum pmu_ctl_action))
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IOCTL: invalid inputsize %ld for PMU_CTL_QUERY_HW_CFG\n", InputBufferLength));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL_PMU_CTL_QUERY_HW_CFG\n"));

        struct hw_cfg* out = (struct hw_cfg*)inBuffer;
        out->core_num = (UINT16)devExt->numCores;
        out->fpc_num = numFPC;
        out->gpc_num = devExt->numFreeGPC;
        out->total_gpc_num = devExt->numGPC;
        out->pmu_ver = (devExt->dfr0_value >> 8) & 0xf;
        out->vendor_id = (devExt->midr_value >> 24) & 0xff;
        out->variant_id = (devExt->midr_value >> 20) & 0xf;
        out->arch_id = (devExt->midr_value >> 16) & 0xf;
        out->rev_id = devExt->midr_value & 0xf;
        out->part_id = (devExt->midr_value >> 4) & 0xfff;
        out->midr_value = devExt->midr_value;
        out->id_aa64dfr0_value = devExt->id_aa64dfr0_el1_value;
        RtlCopyMemory(out->counter_idx_map, devExt->counter_idx_map, sizeof(devExt->counter_idx_map));

        {   // Setup HW_CFG capability string for this driver:
            const wchar_t device_id_str[] =
                WPERF_HW_CFG_CAPS_CORE_DMC;

            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "*outputSize > OutBufSize\n"));

            RtlSecureZeroMemory(out->device_id_str, sizeof(out->device_id_str));
            RtlCopyMemory(out->device_id_str, device_id_str, sizeof(device_id_str));
        }

        retDataSize = sizeof(struct hw_cfg);
        if (retDataSize > OutputBufferLength)
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "retDataSize > OutputBufferLength\n"));
            status = STATUS_BUFFER_TOO_SMALL;
        }
        else
        {
            RtlCopyMemory(outBuffer, out, sizeof(struct hw_cfg));
        }
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

        VOID(*dmc_func)(UINT8, UINT8, dmcs_desc*) = NULL;

        if (IoControlCode == IOCTL_PMU_CTL_START)
            dmc_func = DmcCounterStart;
        if (IoControlCode == IOCTL_PMU_CTL_STOP)
            dmc_func = DmcCounterStop;
        if (IoControlCode == IOCTL_PMU_CTL_RESET)
            dmc_func = DmcCounterReset;



        PWORK_ITEM_CTXT context;
        context = WdfObjectGet_WORK_ITEM_CTXT(queueContext->WorkItem);
        context->IoCtl = IoControlCode;
        context->do_func = NULL;
        context->do_func2 = NULL;
        context->devExt = devExt;
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "%!FUNC! %!LINE! enqueuing for IOCTL %s\n", GetIoctlStr(IoControlCode)));
        WdfWorkItemEnqueue(queueContext->WorkItem);


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

                //core->prof_core = PROF_DISABLED;
                //core->prof_dsu = PROF_DISABLED;
                core->prof_dmc = PROF_DISABLED;

                UINT8 do_multiplex = 0;

                if (i == dmc_core_idx)
                {
                    core->prof_dmc = PROF_NORMAL;
                    core->dmc_ch = dmc_idx;
                }

                if (core->prof_dmc == PROF_DISABLED)
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
                dpc->SystemArgument2 = devExt;
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
                ppmu_event_pseudo events = &core->events[0];
                UINT32 events_num = core->events_num;
                for (UINT32 j = 0; j < events_num; j++)
                {
                    events[j].value = 0;
                    events[j].scheduled = 0;
                }

                ppmu_event_pseudo dsu_events = &core->dsu_events[0];
                UINT32 dsu_events_num = core->dsu_events_num;
                for (UINT32 j = 0; j < dsu_events_num; j++)
                {
                    dsu_events[j].value = 0;
                    dsu_events[j].scheduled = 0;
                }
                KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL_PMU_CTL_RESET calling insert dpc  loop k is %d core index is %lld\n", k, core->idx));
                KeInsertQueueDpc(&core->dpc_reset, (VOID*)cores_count, devExt);  // cores_count has been validated so the DPC will always be called prior to the code below waiting on its completion
            }
            LARGE_INTEGER li;
            li.QuadPart = 0;
            KeWaitForSingleObject(&devExt->sync_reset_dpc, Executive, KernelMode, 0, &li); // wait for all dpcs to complete
            KeClearEvent(&devExt->sync_reset_dpc);
            devExt->cpunos = 0;


            for (UINT8 ch_idx = dmc_ch_base; ch_idx < dmc_ch_end; ch_idx++)
            {
                pdmc_desc dmc = devExt->dmc_array.dmcs + ch_idx;
                ppmu_event_pseudo events = dmc->clk_events;
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

        retDataSize = 0;
        break;
    }

    case IOCTL_PMU_CTL_ASSIGN_EVENTS:
    {
        // does our process own the lock?
        if (!AmILocking(devExt, IoControlCode, fileObject))
        {
            status = STATUS_INVALID_DEVICE_STATE;
            break;
        }

        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL_PMU_CTL_ASSIGN_EVENTS\n"));

        struct pmu_ctl_evt_assign_hdr* ctl_req = (struct pmu_ctl_evt_assign_hdr*)inBuffer;
        //UINT64 filter_bits = ctl_req->filter_bits;
        UINT32 core_idx = ctl_req->core_idx;
        UINT32 core_base, core_end;

        if (core_idx == ALL_CORE)
        {
            core_base = 0;
            core_end = devExt->numCores;
        }
        else
        {
            core_base = core_idx;
            core_end = core_idx + 1;
        }

        ULONG avail_sz = (ULONG)InputBufferLength - sizeof(struct pmu_ctl_evt_assign_hdr);
        UINT8* payload_addr = (UINT8*)inBuffer + sizeof(struct pmu_ctl_evt_assign_hdr);

        for (ULONG consumed_sz = 0; consumed_sz < avail_sz;)
        {
            struct evt_hdr* hdr = (struct evt_hdr*)(payload_addr + consumed_sz);
            enum evt_class evt_class = hdr->evt_class;
            UINT16 evt_num = hdr->num;
            UINT16* raw_evts = (UINT16*)(hdr + 1);


            if (evt_class == EVT_DMC_CLK || evt_class == EVT_DMC_CLKDIV2)
            {
                UINT8 dmc_idx = ctl_req->dmc_idx;
                UINT8 ch_base, ch_end;

                if (dmc_idx == ALL_DMC_CHANNEL)
                {
                    ch_base = 0;
                    ch_end = devExt->dmc_array.dmc_num;
                }
                else
                {
                    ch_base = dmc_idx;
                    ch_end = dmc_idx + 1;
                }

                for (UINT8 ch_idx = ch_base; ch_idx < ch_end; ch_idx++)
                {
                    pdmc_desc  dmc = devExt->dmc_array.dmcs + ch_idx;
                    dmc->clk_events_num = 0;
                    dmc->clkdiv2_events_num = 0;
                    RtlSecureZeroMemory(dmc->clk_events, sizeof(pmu_event_pseudo) * MAX_MANAGED_DMC_CLK_EVENTS);
                    RtlSecureZeroMemory(dmc->clkdiv2_events, sizeof(pmu_event_pseudo) * MAX_MANAGED_DMC_CLKDIV2_EVENTS);
                    ppmu_event_pseudo to_events;
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

                    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL_PMU_CTL_ASSIGN_EVENTS event class invalid %d\n",
                        evt_class));

                    for (UINT16 i = 0; i < evt_num; i++)
                    {
                        ppmu_event_pseudo to_event = to_events + i;
                        to_event->event_idx = raw_evts[i];
                        // clk event counters start after clkdiv2 counters
                        to_event->counter_idx = counter_adj + i;
                        to_event->value = 0;
                        DmcEnableEvent(ch_idx, counter_adj + i, raw_evts[i], &devExt->dmc_array);
                    }
                }
            }
            else
            {
                KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL: assign %d dmc_%s_events (no-multiplexing)\n",
                    evt_num, evt_class == EVT_DMC_CLK ? "clk" : "clkdiv2"));
                status = STATUS_INVALID_PARAMETER;
            }

            consumed_sz += sizeof(struct evt_hdr) + evt_num * sizeof(UINT16);
        }

        retDataSize = 0;
        break;
    }


    case IOCTL_PMU_CTL_QUERY_SUPP_EVENTS:
    {
        // does our process own the lock?
        if (!AmILocking(devExt, IoControlCode, fileObject))
        {
            status = STATUS_INVALID_DEVICE_STATE;
            break;
        }

        if (InputBufferLength != sizeof(enum pmu_ctl_action))
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IOCTL: invalid inputsize %ld for PMU_CTL_QUERY_SUPP_EVENTS\n", InputBufferLength));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL: QUERY_SUPP_EVENTS\n"));

        struct evt_hdr* hdr = (struct evt_hdr*)outBuffer;
        hdr->evt_class = EVT_CORE;
        UINT16 core_evt_num = sizeof(armv8_arch_core_events) / sizeof(armv8_arch_core_events[0]);
        hdr->num = core_evt_num;
        int return_size = 0;

        UINT16* out = (UINT16*)((UINT8*)outBuffer + sizeof(struct evt_hdr));
        for (int i = 0; i < core_evt_num; i++)
            out[i] = armv8_arch_core_events[i];

        out = out + core_evt_num;
        return_size = sizeof(struct evt_hdr);
        return_size += sizeof(armv8_arch_core_events);

        if (devExt->dsu_numGPC)
        {
            hdr = (struct evt_hdr*)out;
            hdr->evt_class = EVT_DSU;
            UINT16 dsu_evt_num = 0;

            out = (UINT16*)((UINT8*)out + sizeof(struct evt_hdr));

            for (UINT16 i = 0; i < 32; i++)
            {
                if (devExt->dsu_evt_mask_lo & (1 << i))
                    out[dsu_evt_num++] = i;
            }

            for (UINT16 i = 0; i < 32; i++)
            {
                if (devExt->dsu_evt_mask_hi & (1 << i))
                    out[dsu_evt_num++] = 32 + i;
            }

            hdr->num = dsu_evt_num;

            return_size += sizeof(struct evt_hdr);
            return_size += dsu_evt_num * sizeof(UINT16);

            out = out + dsu_evt_num;
        }

        if (devExt->dmc_array.dmcs)
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

        retDataSize = return_size;
        if (retDataSize > OutputBufferLength)
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "retDataSize > OutputBufferLength\n"));
            status = STATUS_BUFFER_TOO_SMALL;
        }
        break;
    }
    case IOCTL_DMC_CTL_INIT:
    {
        // does our process own the lock?
        if (!AmILocking(devExt, IoControlCode, fileObject))
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
            devExt->dmc_array.dmcs = (pdmc_desc)ExAllocatePool2(POOL_FLAG_NON_PAGED, sizeof(dmc_desc) * ctl_req->dmc_num, 'DMCR');
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
            pdmc_desc dmc = devExt->dmc_array.dmcs + i;
            DMCReadOut* out = (DMCReadOut*)((UINT8*)outBuffer + sizeof(DMCReadOut) * (i - dmc_ch_base));
            UINT8 clk_events_num = dmc->clk_events_num;
            UINT8 clkdiv2_events_num = dmc->clkdiv2_events_num;
            out->clk_events_num = clk_events_num;
            out->clkdiv2_events_num = clkdiv2_events_num;

            struct pmu_event_usr* to_events = out->clk_events;
            ppmu_event_pseudo from_events = dmc->clk_events;
            for (UINT8 j = 0; j < clk_events_num; j++)
            {
                ppmu_event_pseudo from_event = from_events + j;
                struct pmu_event_usr* to_event = to_events + j;
                to_event->event_idx = from_event->event_idx;
                to_event->scheduled = from_event->scheduled;
                to_event->value = from_event->value;
            }

            to_events = out->clkdiv2_events;
            from_events = dmc->clkdiv2_events;
            for (UINT8 j = 0; j < clkdiv2_events_num; j++)
            {
                ppmu_event_pseudo from_event = from_events + j;
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
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL: invalid %s\n", GetIoctlStr(IoControlCode)));
        status = STATUS_INVALID_PARAMETER;
        retDataSize = 0;
        break;
    }

    WdfRequestCompleteWithInformation(Request, status, retDataSize);

    return;
}
