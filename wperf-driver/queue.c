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
#include "wperf-common/iorequest.h"
#include "queue.tmh"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, WindowsPerfQueueInitialize)
#pragma alloc_text (PAGE, WindowsPerfTimerCreate)
#endif

NTSTATUS deviceControl(
    _In_    PVOID   pBuffer,
    _In_    ULONG   inputSize,
    _Out_   PULONG  outputSize,
    _Inout_ PQUEUE_CONTEXT queueContext
);

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
        KdPrint(("WdfIoQueueCreate failed 0x%x\n",status));
        return status;
    }

    // Get our Driver Context memory from the returned Queue handle
    queueContext = QueueGetContext(queue);

    queueContext->Buffer = NULL;

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
        KdPrint(("WdfWorkItemCreate failed 0x%x", wiStatus));
    }

    PWORK_ITEM_CTXT workItemCtxt;
    workItemAttributes.ParentObject = NULL;
    wiStatus = WdfObjectAllocateContext(queueContext->WorkItem, &workItemAttributes, &workItemCtxt);
    if (!NT_SUCCESS(wiStatus))
    {
        KdPrint(("WdfObjectAllocateContext failed 0x%x", wiStatus));
    }

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
    PQUEUE_CONTEXT queueContext = QueueGetContext(Queue);

    KdPrint(("%!FUNC! %!LINE! Queue 0x%p Request 0x%p OutputBufferLength %llu InputBufferLength %llu IoControlCode %lu",
        Queue, Request, OutputBufferLength, InputBufferLength, IoControlCode));

    if (OutputBufferLength > MAX_WRITE_LENGTH) {
        KdPrint(("%!FUNC! %!LINE!  Buffer Length too big %Iu, Max is %d\n",
            OutputBufferLength, MAX_WRITE_LENGTH));
        WdfRequestCompleteWithInformation(Request, STATUS_BUFFER_OVERFLOW, 0L);
        return;
    }

    // Get the memory buffer
    Status = WdfRequestRetrieveInputMemory(Request, &memory);
    if (!NT_SUCCESS(Status)) {
        KdPrint(("%!FUNC! %!LINE! Could not get request memory buffer 0x%x\n",
            Status));
        WdfVerifierDbgBreakPoint();
        WdfRequestComplete(Request, Status);
        return;
    }

    // Release previous buffer if set
    if (queueContext->Buffer != NULL) {
        ExFreePool(queueContext->Buffer);
        queueContext->Buffer = NULL;
        queueContext->Length = 0L;
    }

    queueContext->Buffer = ExAllocatePool2(POOL_FLAG_NON_PAGED, max(OutputBufferLength, InputBufferLength), 'sam1');
    // Set completion status information 
    if (queueContext->Buffer == NULL) {
        KdPrint(("%!FUNC! %!LINE!: Could not allocate %Iu byte buffer\n", max(OutputBufferLength, InputBufferLength)));
        WdfRequestComplete(Request, STATUS_INSUFFICIENT_RESOURCES);
        return;
    }

    // Copy the memory in
    Status = WdfMemoryCopyToBuffer(memory,
        0,  // offset into the source memory
        queueContext->Buffer,
        InputBufferLength);
    if (!NT_SUCCESS(Status)) {
        KdPrint(("%!FUNC! %!LINE! WdfMemoryCopyToBuffer failed 0x%x\n", Status));
        WdfVerifierDbgBreakPoint();

        ExFreePool(queueContext->Buffer);
        queueContext->Buffer = NULL;
        queueContext->Length = 0L;

        WdfRequestComplete(Request, Status);
        return;
    }

    //
    // Port Begin
    //
    ULONG outputSize;
    Status = deviceControl(queueContext->Buffer, (ULONG)InputBufferLength, &outputSize, queueContext);
    if (!NT_SUCCESS(Status)) {
        KdPrint(("%!FUNC! %!LINE! deviceControl failed 0x%x\n", Status));
        WdfVerifierDbgBreakPoint();

        ExFreePool(queueContext->Buffer);
        queueContext->Buffer = NULL;
        queueContext->Length = 0L;

        WdfRequestComplete(Request, Status);
        return;
    }
    KdPrint(("%!FUNC! %!LINE! deviceControl outputSize=%lu", outputSize));
    //
    // Port End
    //

    queueContext->Length = (ULONG)outputSize;
    if (queueContext->Length > OutputBufferLength)
    {
        KdPrint(("%!FUNC! %!LINE! outputSize bigger than OutputBufferLenght"));
        WdfVerifierDbgBreakPoint();
        ExFreePool(queueContext->Buffer);
        queueContext->Buffer = NULL;
        queueContext->Length = 0L;
        WdfRequestComplete(Request, STATUS_INSUFFICIENT_RESOURCES);
        return;
    }
    queueContext->CurrentRequest = Request;
    queueContext->CurrentStatus = Status;

    //
    // No data to read
    //
    if ((queueContext->Buffer == NULL) || OutputBufferLength == 0 || queueContext->Length == 0) {
        WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, (ULONG_PTR)0L);
        return;
    }

    //
    // Read what we have
    //
    if (queueContext->Length < OutputBufferLength) {
        OutputBufferLength = queueContext->Length;
    }

    //
    // Get the request memory
    //
    Status = WdfRequestRetrieveOutputMemory(Request, &memory);
    if (!NT_SUCCESS(Status)) {
        KdPrint(("%!FUNC! %!LINE! Could not get request memory buffer 0x%x\n", Status));
        WdfVerifierDbgBreakPoint();
        WdfRequestCompleteWithInformation(Request, Status, 0L);
        return;
    }

    // Copy the memory out
    Status = WdfMemoryCopyFromBuffer(memory, // destination
        0,      // offset into the destination memory
        queueContext->Buffer,
        OutputBufferLength);
    if (!NT_SUCCESS(Status)) {
        KdPrint(("%!FUNC! %!LINE!: WdfMemoryCopyFromBuffer failed 0x%x, Buffer=0x%p, Length=%llu \n",
            Status, queueContext->Buffer, OutputBufferLength));
        WdfRequestComplete(Request, Status);
        return;
    }

    // Set transfer information
    WdfRequestSetInformation(Request, (ULONG_PTR)OutputBufferLength);
#if 0
    // Mark the request is cancelable
    WdfRequestMarkCancelable(Request, WindowsPerfEvtRequestCancel);

    // Defer the completion to another thread from the timer dpc
    queueContext->CurrentRequest = Request;
    queueContext->CurrentStatus = Status;
#endif
    queueContext->CurrentRequest = NULL;
    KdPrint(("%!FUNC! %!LINE! Completing request 0x%p, Status 0x%x \n", Request, Status));
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

    //
    // Release any resources pointed to in the queue context.
    //
    // The body of the queue context will be released after
    // this callback handler returns
    //

    //
    // If Queue context has an I/O buffer, release it
    //
    if( queueContext->Buffer != NULL ) {
        ExFreePool(queueContext->Buffer);
    }

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

    KdPrint(("WindowsPerfEvtRequestCancel called on Request 0x%p\n",  Request));

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
