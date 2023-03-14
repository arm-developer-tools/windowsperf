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

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, WindowsPerfQueueInitialize)
#pragma alloc_text (PAGE, WindowsPerfTimerCreate)
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

    queueConfig.EvtIoRead   = WindowsPerfEvtIoRead;
    queueConfig.EvtIoWrite  = WindowsPerfEvtIoWrite;

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
    queueContext->Timer = NULL;

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

    //
    // Create the Queue timer
    //
    status = WindowsPerfTimerCreate(&queueContext->Timer, TIMER_PERIOD, queue);
    if (!NT_SUCCESS(status)) {
        KdPrint(("Error creating timer 0x%x\n",status));
        return status;
    }

    return status;
}


NTSTATUS
WindowsPerfTimerCreate(
    IN WDFTIMER*       Timer,
    IN ULONG           Period,
    IN WDFQUEUE        Queue
    )
/*++

Routine Description:

    Subroutine to create periodic timer. By associating the timerobject with
    the queue, we are basically telling the framework to serialize the queue
    callbacks with the dpc callback. By doing so, we don't have to worry
    about protecting queue-context structure from multiple threads accessing
    it simultaneously.

Arguments:


Return Value:

    NTSTATUS

--*/
{
    NTSTATUS Status;
    WDF_TIMER_CONFIG       timerConfig;
    WDF_OBJECT_ATTRIBUTES  timerAttributes;

    PAGED_CODE();

    //
    // Create a periodic timer.
    //
    // WDF_TIMER_CONFIG_INIT_PERIODIC sets AutomaticSerialization to TRUE by default.
    //
    WDF_TIMER_CONFIG_INIT_PERIODIC(&timerConfig, WindowsPerfEvtTimerFunc, Period);

    WDF_OBJECT_ATTRIBUTES_INIT(&timerAttributes);
    timerAttributes.ParentObject = Queue; // Synchronize with the I/O Queue

    Status = WdfTimerCreate(&timerConfig,
                             &timerAttributes,
                             Timer     // Output handle
                             );

    return Status;
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

VOID
WindowsPerfEvtIoRead(
    IN WDFQUEUE   Queue,
    IN WDFREQUEST Request,
    IN size_t      Length
    )
/*++

Routine Description:

    This event is called when the framework receives IRP_MJ_READ request.
    It will copy the content from the queue-context buffer to the request buffer.
    If the driver hasn't received any write request earlier, the read returns zero.

Arguments:

    Queue -  Handle to the framework queue object that is associated with the
             I/O request.

    Request - Handle to a framework request object.

    Length  - number of bytes to be read.
              The default property of the queue is to not dispatch
              zero lenght read & write requests to the driver and
              complete is with status success. So we will never get
              a zero length request.

Return Value:

    VOID

--*/
{
    NTSTATUS Status;
    PQUEUE_CONTEXT queueContext = QueueGetContext(Queue);
    WDFMEMORY memory;

    _Analysis_assume_(Length > 0);

    KdPrint(("WindowsPerfEvtIoRead Called! Queue 0x%p, Request 0x%p Length %Iu\n",
             Queue,Request,Length));
    //
    // No data to read
    //
    if( (queueContext->Buffer == NULL)  ) {
        WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, (ULONG_PTR)0L);
        return;
    }
    _Analysis_assume_(queueContext->Length > 0);

    //
    // Read what we have
    //
    if( queueContext->Length < Length ) {
        Length = queueContext->Length;
    }

    //
    // Get the request memory
    //
    Status = WdfRequestRetrieveOutputMemory(Request, &memory);
    if( !NT_SUCCESS(Status) ) {
        KdPrint(("WindowsPerfEvtIoRead Could not get request memory buffer 0x%x\n", Status));
        WdfVerifierDbgBreakPoint();
        WdfRequestCompleteWithInformation(Request, Status, 0L);
        return;
    }

    // Copy the memory out
    Status = WdfMemoryCopyFromBuffer( memory, // destination
                             0,      // offset into the destination memory
                             queueContext->Buffer,
                             Length );
    if( !NT_SUCCESS(Status) ) {
        KdPrint(("WindowsPerfEvtIoRead: WdfMemoryCopyFromBuffer failed 0x%x, Buffer=0x%p, Length=%zu \n",
            Status, queueContext->Buffer, Length));
        WdfRequestComplete(Request, Status);
        return;
    }

    // Set transfer information
    WdfRequestSetInformation(Request, (ULONG_PTR)Length);

    // Mark the request is cancelable
    WdfRequestMarkCancelable(Request, WindowsPerfEvtRequestCancel);

    // Defer the completion to another thread from the timer dpc
    queueContext->CurrentRequest = Request;
    queueContext->CurrentStatus  = Status;

    return;
}

NTSTATUS deviceControl(
    _In_    PVOID   pBuffer,
    _In_    ULONG   inputSize,
    _Out_   PULONG  outputSize,
    _Inout_ PQUEUE_CONTEXT queueContext
);

VOID
WindowsPerfEvtIoWrite(
    IN WDFQUEUE   Queue,
    IN WDFREQUEST Request,
    IN size_t     Length
)
/*++

Routine Description:

    This event is invoked when the framework receives IRP_MJ_WRITE request.
    This routine allocates memory buffer, copies the data from the request to it,
    and stores the buffer pointer in the queue-context with the length variable
    representing the buffers length. The actual completion of the request
    is defered to the periodic timer dpc.

Arguments:

    Queue -  Handle to the framework queue object that is associated with the
             I/O request.

    Request - Handle to a framework request object.

    Length  - number of bytes to be read.
              The default property of the queue is to not dispatch
              zero lenght read & write requests to the driver and
              complete is with status success. So we will never get
              a zero length request.

Return Value:

    VOID

--*/
{
    NTSTATUS Status;
    WDFMEMORY memory;
    PQUEUE_CONTEXT queueContext = QueueGetContext(Queue);

    _Analysis_assume_(Length > 0);

    KdPrint(("WindowsPerfEvtIoWrite Called! Queue 0x%p, Request 0x%p Length %Iu\n",
        Queue, Request, Length));

    if (Length > MAX_WRITE_LENGTH) {
        KdPrint(("WindowsPerfEvtIoWrite Buffer Length to big %Iu, Max is %d\n",
            Length, MAX_WRITE_LENGTH));
        WdfRequestCompleteWithInformation(Request, STATUS_BUFFER_OVERFLOW, 0L);
        return;
    }

    // Get the memory buffer
    Status = WdfRequestRetrieveInputMemory(Request, &memory);
    if (!NT_SUCCESS(Status)) {
        KdPrint(("WindowsPerfEvtIoWrite Could not get request memory buffer 0x%x\n",
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

    queueContext->Buffer = ExAllocatePool2(POOL_FLAG_NON_PAGED, MAX_WRITE_LENGTH, 'sam1');
    // Set completion status information 
    if (queueContext->Buffer == NULL) {
        KdPrint(("WindowsPerfEvtIoWrite: Could not allocate %Iu byte buffer\n", Length));
        WdfRequestComplete(Request, STATUS_INSUFFICIENT_RESOURCES);
        return;
    }

    // Copy the memory in
    Status = WdfMemoryCopyToBuffer(memory,
        0,  // offset into the source memory
        queueContext->Buffer,
        Length);
    if (!NT_SUCCESS(Status)) {
        KdPrint(("WindowsPerfEvtIoWrite WdfMemoryCopyToBuffer failed 0x%x\n", Status));
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
    Status = deviceControl(queueContext->Buffer, (ULONG)Length, &outputSize, queueContext);
    if (!NT_SUCCESS(Status)) {
        KdPrint(("WindowsPerfEvtIoWrite deviceControl failed 0x%x\n", Status));
        WdfVerifierDbgBreakPoint();

        ExFreePool(queueContext->Buffer);
        queueContext->Buffer = NULL;
        queueContext->Length = 0L;

        WdfRequestComplete(Request, Status);
        return;
    }
    KdPrint(("WindowsPerfEvtIoWrite deviceControl outputSize=%lu", outputSize));
    //
    // Port End
    //

    queueContext->Length = (ULONG)outputSize; /*Length*/

    // Set transfer information
    WdfRequestSetInformation(Request, (ULONG_PTR)outputSize); /*Length*/

    // Specify the request is cancelable
    WdfRequestMarkCancelable(Request, WindowsPerfEvtRequestCancel);

    // Defer the completion to another thread from the timer dpc
    queueContext->CurrentRequest = Request;
    queueContext->CurrentStatus  = Status;

    return;
}


VOID
WindowsPerfEvtTimerFunc(
    IN WDFTIMER     Timer
    )
/*++

Routine Description:

    This is the TimerDPC the driver sets up to complete requests.
    This function is registered when the WDFTIMER object is created, and
    will automatically synchronize with the I/O Queue callbacks
    and cancel routine.

Arguments:

    Timer - Handle to a framework Timer object.

Return Value:

    VOID

--*/
{
    NTSTATUS      Status;
    WDFREQUEST     Request;
    WDFQUEUE queue;
    PQUEUE_CONTEXT queueContext;

    queue = WdfTimerGetParentObject(Timer);
    queueContext = QueueGetContext(queue);

    //
    // DPC is automatically synchronized to the Queue lock,
    // so this is race free without explicit driver managed locking.
    //
    Request = queueContext->CurrentRequest;
    if (Request != NULL) {

        //
        // Attempt to remove cancel status from the request.
        //
        // The request is not completed if it is already cancelled
        // since the WindowsPerfEvtIoCancel function has run, or is about to run
        // and we are racing with it.
        //
        Status = WdfRequestUnmarkCancelable(Request);
        if (Status != STATUS_CANCELLED) {

            queueContext->CurrentRequest = NULL;
            Status = queueContext->CurrentStatus;

            KdPrint(("CustomTimerDPC Completing request 0x%p, Status 0x%x \n", Request, Status));

            WdfRequestComplete(Request, Status);
        }
        else {
            KdPrint(("CustomTimerDPC Request 0x%p is STATUS_CANCELLED, not completing\n",
                Request));
        }
    }

    return;
}
