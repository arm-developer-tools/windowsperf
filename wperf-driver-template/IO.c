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
#include "queue.tmh"
#endif

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
    NTSTATUS                   status              = STATUS_SUCCESS;
    PQUEUE_CONTEXT      queueContext   = GetQueueContext(Queue);
    PDEVICE_EXTENSION devExt               = queueContext->devExt;
    WDFFILEOBJECT         fileObject          = WdfRequestGetFileObject(Request);
    WDFMEMORY              memory;
    PVOID                        inBuffer             = 0;
    PVOID                        outBuffer           = 0;

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
            WdfRequestSetInformation(Request, (ULONG_PTR)sizeof(enum status_flag));
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

            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL_PMU_CTL_LOCK_RELEASE for file_object %p\n", fileObject));

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
            WdfRequestSetInformation(Request, (ULONG_PTR)sizeof(enum status_flag));
            break;
        }
    }



    WdfRequestComplete(Request, STATUS_SUCCESS);

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
