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
#include "wperf-common/iorequest.h"
#if defined ENABLE_TRACING
#include "queue.tmh"
#endif
#include "device.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, WindowsPerfQueueInitialize)
#endif

VOID EvtWorkItemFunc(WDFWORKITEM WorkItem);

NTSTATUS
WindowsPerfQueueInitialize(
    WDFDEVICE Device
    )
/*++

Routine Description:


     The I/O dispatch callbacks for the frameworks device object
     are configured in this function.

     A single default I/O Queue is configured for serial request
     processing, and a driver context memory allocation is created
     to hold our structure QUEUE_CONTEXT.

     This memory may be used by the driver automatically synchronized
     by the Queue's presentation lock.

     The lifetime of this memory is tied to the lifetime of the I/O
     Queue object, and we register an optional destructor callback
     to release any private allocations, and/or resources.


Arguments:

    Device - Handle to a framework device object.

Return Value:

    NTSTATUS

--*/
{
    WDFQUEUE queue;
    NTSTATUS status;
    PQUEUE_CONTEXT queueContext;
    WDF_IO_QUEUE_CONFIG    queueConfig;
    WDF_OBJECT_ATTRIBUTES  queueAttributes;
    PDEVICE_EXTENSION  pDevExt = GetDeviceExtension(Device);

    PAGED_CODE();

    //
    // Configure a default queue so that requests that are not
    // configure-fowarded using WdfDeviceConfigureRequestDispatching to goto
    // other queues get dispatched here.
    //
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(
         &queueConfig,
        WdfIoQueueDispatchSequential
        );

    queueConfig.EvtIoDeviceControl = WindowsPerfEvtDeviceControl;

    //
    // Fill in a callback for destroy, and our QUEUE_CONTEXT size
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&queueAttributes, QUEUE_CONTEXT);

    //
    // Set synchronization scope on queue and have the timer to use queue as
    // the parent object so that queue and timer callbacks are synchronized
    // with the same lock.
    //
    queueAttributes.SynchronizationScope = WdfSynchronizationScopeQueue;

    queueAttributes.EvtDestroyCallback = WindowsPerfEvtIoQueueContextDestroy;

    status = WdfIoQueueCreate(
                 Device,
                 &queueConfig,
                 &queueAttributes,
                 &queue
                 );

    if( !NT_SUCCESS(status) ) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID,  DPFLTR_INFO_LEVEL, "WdfIoQueueCreate failed 0x%x\n",status));
        return status;
    }

    // Get our Driver Context memory from the returned Queue handle
    queueContext = QueueGetContext(queue);

    queueContext->inBuffer = NULL;
    queueContext->outBuffer = NULL;
    queueContext->CurrentRequest = NULL;
    queueContext->CurrentStatus = STATUS_INVALID_DEVICE_REQUEST;

    WDF_WORKITEM_CONFIG workItemConfig;
    WDF_OBJECT_ATTRIBUTES workItemAttributes;

    WDF_WORKITEM_CONFIG_INIT(&workItemConfig, EvtWorkItemFunc);
    workItemConfig.AutomaticSerialization = FALSE;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&workItemAttributes, WORK_ITEM_CTXT);
    workItemAttributes.ParentObject = queue;

    NTSTATUS wiStatus = WdfWorkItemCreate(&workItemConfig, &workItemAttributes, &queueContext->WorkItem);
    if (!NT_SUCCESS(wiStatus))
    {
        KdPrintEx((DPFLTR_IHVDRIVER_ID,  DPFLTR_INFO_LEVEL, "WdfWorkItemCreate failed 0x%x\n", wiStatus));
    }

    PWORK_ITEM_CTXT workItemCtxt;
    workItemAttributes.ParentObject = NULL;
    wiStatus = WdfObjectAllocateContext(queueContext->WorkItem, &workItemAttributes, &workItemCtxt);
    if (!NT_SUCCESS(wiStatus))
    {
        KdPrintEx((DPFLTR_IHVDRIVER_ID,  DPFLTR_INFO_LEVEL, "WdfObjectAllocateContext failed 0x%x\n", wiStatus));
    }

    // store a reference to the queue context in our device extension
    pDevExt->pQueContext = queueContext;

    return status;
}

VOID
WindowsPerfEvtDeviceControl(
    IN WDFQUEUE Queue,
    IN WDFREQUEST Request,
    IN size_t OutputBufferLength,
    IN size_t InputBufferLength,
    IN ULONG IoControlCode
)
{
    NTSTATUS Status;
    WDFMEMORY memory;
    size_t bufsize = 0;
    PQUEUE_CONTEXT queueContext = QueueGetContext(Queue);
    WDFFILEOBJECT  file_object = WdfRequestGetFileObject(Request);
    WDFDEVICE        device = WdfFileObjectGetDevice(file_object);
    PDEVICE_EXTENSION  pDevExt = GetDeviceExtension(device);


    queueContext->CurrentRequest = NULL;
    queueContext->inBuffer = NULL;
    queueContext->outBuffer = NULL;

    KdPrintEx((DPFLTR_IHVDRIVER_ID,  DPFLTR_INFO_LEVEL, "%!FUNC! %!LINE! Queue 0x%p Request 0x%p OutputBufferLength %llu InputBufferLength %llu IoControlCode %lu\n",
        Queue, Request, OutputBufferLength, InputBufferLength, IoControlCode));


    if (pDevExt->AskedToRemove)
    {
        WdfRequestComplete(Request, STATUS_INVALID_DEVICE_STATE);
        return;
    }

    //
    // Get the memory buffers
    //
    if (InputBufferLength > 0)
    {
        Status = WdfRequestRetrieveInputMemory(Request, &memory);
        if (!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "%!FUNC! %!LINE! Could not get request in memory buffer 0x%x\n",
                Status));
            //WdfVerifierDbgBreakPoint();
            queueContext->CurrentRequest = NULL;
            queueContext->inBuffer = NULL;
            queueContext->outBuffer = NULL;
            WdfRequestComplete(Request, Status);
            return;
        }
        queueContext->inBuffer = WdfMemoryGetBuffer(memory, &bufsize);  // this also returns the buffer size, so check it against the given size
        if (bufsize != InputBufferLength)
        {
            Status = STATUS_INVALID_BLOCK_LENGTH;
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "%!FUNC! %!LINE!  bufsize != InputBufferLength 0x%x\n",
                Status));
            //WdfVerifierDbgBreakPoint();
            queueContext->CurrentRequest = NULL;
            queueContext->inBuffer = NULL;
            queueContext->outBuffer = NULL;
            WdfRequestComplete(Request, Status);
            return;
        }
    }
    else
    {
        Status = STATUS_INVALID_BLOCK_LENGTH;
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "%!FUNC! %!LINE!  InputBufferLength is zero 0x%x\n",
            Status));
        //WdfVerifierDbgBreakPoint();
        queueContext->CurrentRequest = NULL;
        queueContext->inBuffer = NULL;
        queueContext->outBuffer = NULL;
        WdfRequestComplete(Request, Status);
        return;

    }

    if (OutputBufferLength > 0) // we might not always have an out buffer
    {
        Status = WdfRequestRetrieveOutputMemory(Request, &memory);
        if (!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "%!FUNC! %!LINE! Could not get request out memory buffer 0x%x\n",
                Status));
            //WdfVerifierDbgBreakPoint();
            queueContext->CurrentRequest = NULL;
            queueContext->inBuffer = NULL;
            queueContext->outBuffer = NULL;
            WdfRequestComplete(Request, Status);
            return;
        }
        queueContext->outBuffer = WdfMemoryGetBuffer(memory, &bufsize);// this also returns the buffer size, so check it against the given size
        if (bufsize != OutputBufferLength)
        {
            Status = STATUS_INVALID_BLOCK_LENGTH;
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "%!FUNC! %!LINE!  bufsize != OutputBufferLength 0x%x\n",
                Status));
            //WdfVerifierDbgBreakPoint();
            queueContext->CurrentRequest = NULL;
            queueContext->inBuffer = NULL;
            queueContext->outBuffer = NULL;
            WdfRequestComplete(Request, Status);
            return;
        }
    }

    //
    //  Sanity check values
    //
    if (InputBufferLength && (queueContext->inBuffer == NULL)) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "%!FUNC! %!LINE!  No input buffer\n"));
        WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, (ULONG_PTR)0L);
        queueContext->CurrentRequest = NULL;
        queueContext->inBuffer = NULL;
        queueContext->outBuffer = NULL;
        return;
    }
    if (OutputBufferLength && (queueContext->outBuffer == NULL)) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "%!FUNC! %!LINE!  No output  buffer\n"));
        WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, (ULONG_PTR)0L);
        queueContext->CurrentRequest = NULL;
        queueContext->inBuffer = NULL;
        queueContext->outBuffer = NULL;
        return;
    }

    if (OutputBufferLength > MAX_WRITE_LENGTH) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "%!FUNC! %!LINE!  Buffer Length too big %Iu, Max is %d\n",
            OutputBufferLength, MAX_WRITE_LENGTH));
        WdfRequestCompleteWithInformation(Request, STATUS_BUFFER_OVERFLOW, 0L);
        return;
    }

    queueContext->CurrentRequest = Request;

    //
    // Port Begin
    //
    ULONG outputDataSize;
    Status = deviceControl(file_object, IoControlCode, queueContext->inBuffer, (ULONG)InputBufferLength, queueContext->outBuffer, (ULONG)OutputBufferLength, &outputDataSize, queueContext);
    if (!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "%!FUNC! %!LINE! deviceControl failed 0x%x\n", Status));
        //WdfVerifierDbgBreakPoint();
        queueContext->CurrentRequest = NULL;
        queueContext->inBuffer = NULL;
        queueContext->outBuffer = NULL;
        WdfRequestComplete(Request, Status);
        return;
    }
    KdPrintEx((DPFLTR_IHVDRIVER_ID,  DPFLTR_INFO_LEVEL, "%!FUNC! %!LINE! deviceControl outputDataSize=%lu\n", outputDataSize));
    //
    // Port End
    //

   
    if (outputDataSize > OutputBufferLength)
    {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "%!FUNC! %!LINE! outputDataSize bigger than OutputBufferLenght\n"));
        //WdfVerifierDbgBreakPoint();
        queueContext->CurrentRequest = NULL;
        queueContext->inBuffer = NULL;
        queueContext->outBuffer = NULL;
        WdfRequestComplete(Request, STATUS_INSUFFICIENT_RESOURCES);
        return;
    }

    queueContext->CurrentStatus = Status;


    //
    // Set transfer information
    //
    if (outputDataSize < OutputBufferLength)
        OutputBufferLength = outputDataSize;

    WdfRequestSetInformation(Request, (ULONG_PTR)OutputBufferLength);

 #if 0
    // Mark the request is cancelable
    WdfRequestMarkCancelable(Request, WindowsPerfEvtRequestCancel);

    // Defer the completion to another thread from the timer dpc
    queueContext->CurrentRequest = Request;
    queueContext->CurrentStatus = Status;
#endif

    queueContext->CurrentRequest = NULL;
    queueContext->inBuffer = NULL;
    queueContext->outBuffer = NULL;
    KdPrintEx((DPFLTR_IHVDRIVER_ID,  DPFLTR_INFO_LEVEL, "%!FUNC! %!LINE! Completing request 0x%p, Status 0x%x \n", Request, Status));
    WdfRequestComplete(Request, Status);
}

VOID
WindowsPerfEvtIoQueueContextDestroy(
    WDFOBJECT Object
)
/*++

Routine Description:

    This is called when the Queue that our driver context memory
    is associated with is destroyed.

Arguments:

    Context - Context that's being freed.

Return Value:

    VOID

--*/
{
    PQUEUE_CONTEXT queueContext = QueueGetContext(Object);
    queueContext->inBuffer = NULL;
    queueContext->outBuffer = NULL;

    // The body of the queue context will be released after
    // this callback handler returns

    return;
}


VOID
WindowsPerfEvtRequestCancel(
    IN WDFREQUEST Request
    )
/*++

Routine Description:


    Called when an I/O request is cancelled after the driver has marked
    the request cancellable. This callback is automatically synchronized
    with the I/O callbacks since we have chosen to use frameworks Device
    level locking.

Arguments:

    Request - Request being cancelled.

Return Value:

    VOID

--*/
{
    PQUEUE_CONTEXT queueContext = QueueGetContext(WdfRequestGetIoQueue(Request));

    KdPrintEx((DPFLTR_IHVDRIVER_ID,  DPFLTR_INFO_LEVEL, "WindowsPerfEvtRequestCancel called on Request 0x%p\n",  Request));

    //
    // The following is race free by the callside or DPC side
    // synchronizing completion by calling
    // WdfRequestMarkCancelable(Queue, Request, FALSE) before
    // completion and not calling WdfRequestComplete if the
    // return status == STATUS_CANCELLED.
    //
    WdfRequestCompleteWithInformation(Request, STATUS_CANCELLED, 0L);

    //
    // This book keeping is synchronized by the common
    // Queue presentation lock
    //
    ASSERT(queueContext->CurrentRequest == Request);
    queueContext->CurrentRequest = NULL;

    return;
}
