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

#define INITGUID

#include <ntddk.h>
#include <wdf.h>
#include <limits.h>
#include "queue.h"
#if !defined DBG
#include "trace.h"
#endif
#include <Ntstrsafe.h>

//
// Device
//
#define NT_DEVICE_NAME		L"\\Device\\WPERFDRIVER"
#define DOS_DEVICE_NAME		L"\\DosDevices\\WPERFDRIVER"


//
//  To get memory resource of 620 filter
//
#define IOCTL_GET_DEVICE_INFO 	CTL_CODE(FILE_DEVICE_UNKNOWN, 0xA00 , METHOD_BUFFERED, FILE_READ_DATA|FILE_WRITE_DATA)



//
// Driver unload function
//
extern VOID WindowsPerfDeviceUnload();

//
// WDFDRIVER Events
//
DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD WindowsPerfEvtDeviceAdd;
EVT_WDF_DRIVER_UNLOAD WindowsPerfEvtWdfDriverUnload;

//
//  Memory Resource Data
//

typedef struct  _MEMORY {
    ULONG Length;
    ULONG Alignment;
    PHYSICAL_ADDRESS MinimumAddress;
    PHYSICAL_ADDRESS MaximumAddress;
} MEMORY, * PMEMORY;


//
// Retrieve framework version string
//
NTSTATUS
WindowsPerfPrintDriverVersion(
);

NTSTATUS deviceControl(
    _In_    ULONG   IoControlCode, 
    _In_    PVOID   pInBuffer,
    _In_    ULONG   InBufSize,
    _In_opt_    PVOID   pOutBuffer,
    _In_    ULONG   OutBufSize,
    _Out_   PULONG  outputSize,
    _Inout_ PQUEUE_CONTEXT queueContext);

NTSTATUS deviceControlB2BTimeline(
    _In_    ULONG   IoControlCode,
    _In_    PVOID   pInBuffer,
    _In_    ULONG   InBufSize,
    _In_opt_    PVOID   pOutBuffer,
    _In_    ULONG   OutBufSize,
    _Out_   PULONG  outputSize,
    _Inout_ PQUEUE_CONTEXT queueContext);

typedef struct _B2BData
{
    LIST_ENTRY	List;
    UINT32 evt_num;
    UINT64 round;
    struct pmu_event_usr evts[MAX_MANAGED_CORE_EVENTS];
}B2BData, * PB2BData;


typedef struct _B2BDataHead
{
    LIST_ENTRY	List;
} B2BDataHead, * PB2BDataHead;


