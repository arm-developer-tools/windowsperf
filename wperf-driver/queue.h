#pragma once
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

#include "wperf-common/iorequest.h"

//
// Set max write length for testing
//
#define MAX_WRITE_LENGTH 1024*40

//
// Set timer period in ms
//
#define TIMER_PERIOD     16

//
// This is the context that can be placed per queue
// and would contain per queue information.
//
typedef struct _QUEUE_CONTEXT {

    // Here we allocate a buffer from a test write so it can be read back
    PVOID       inBuffer;
    PVOID       outBuffer;

    WDFWORKITEM WorkItem;

    // Virtual I/O
    enum pmu_ctl_action action;     // Current action

    WDFREQUEST  CurrentRequest;
    NTSTATUS    CurrentStatus;

} QUEUE_CONTEXT, *PQUEUE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(QUEUE_CONTEXT, QueueGetContext)

typedef struct WORK_ITEM_CTXT_
{
    UINT32 core_idx;
    UINT32 core_base, core_end, event_num;
    UINT32 ctl_flags;
    struct pmu_ctl_hdr* ctl_req;
    size_t cores_count;
    _Bool isDSU;
    int sample_src_num;
    PMUSampleSetSrcHdr* sample_req;
    enum pmu_ctl_action action;
    //struct pmu_event_pseudo* events;
    VOID(*do_func)(VOID);
    VOID(*do_func2)(VOID);
} WORK_ITEM_CTXT, *PWORK_ITEM_CTXT;

WDF_DECLARE_CONTEXT_TYPE(WORK_ITEM_CTXT)

NTSTATUS
WindowsPerfQueueInitialize(
    WDFDEVICE hDevice
    );

EVT_WDF_IO_QUEUE_CONTEXT_DESTROY_CALLBACK WindowsPerfEvtIoQueueContextDestroy;

//
// Events from the IoQueue object
//
EVT_WDF_REQUEST_CANCEL WindowsPerfEvtRequestCancel;
EVT_WDF_IO_QUEUE_IO_READ WindowsPerfEvtIoRead;
EVT_WDF_IO_QUEUE_IO_WRITE WindowsPerfEvtIoWrite;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL WindowsPerfEvtDeviceControl;
EVT_WDF_TIMER WindowsPerfEvtTimerFunc;
