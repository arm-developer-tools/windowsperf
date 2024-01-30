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

#define INITGUID

#include <ntddk.h>
#include <wdf.h>
#include <limits.h>
#include "queue.h"
#if defined ENABLE_TRACING
#include "trace.h"
#endif
#include <stdarg.h>
#include <ntstrsafe.h>
#include "Wperf_DriverETW_schema.h"
//
// Device
//
#define NT_DEVICE_NAME		L"\\Device\\WPERFDRIVER"
#define DOS_DEVICE_NAME		L"\\DosDevices\\WPERFDRIVER"

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

typedef struct _LOCK_STATUS
{
	enum status_flag status;
	ULONG ioctl;
	KSPIN_LOCK sts_lock;
	WDFFILEOBJECT  file_object;
} LOCK_STATUS;

//
// Retrieve framework version string
//
NTSTATUS
WindowsPerfPrintDriverVersion(
);

NTSTATUS deviceControl(
    _In_        WDFFILEOBJECT  file_object,
    _In_        ULONG   IoControlCode, 
    _In_        PVOID   pInBuffer,
    _In_        ULONG   InBufSize,
    _In_opt_    PVOID   pOutBuffer,
    _In_        ULONG   OutBufSize,
    _Out_       PULONG  outputSize,
    _Inout_     PQUEUE_CONTEXT queueContext);

static __inline VOID
__ETWTrace(
    IN  const CHAR* Prefix,
    IN  const CHAR* Format,
    ...
)
{
#define   OUT_SIZE   256
    va_list               Arguments;
    CHAR               out[OUT_SIZE];
    NTSTATUS        status = STATUS_SUCCESS;
    size_t               len = 0;

    RtlZeroMemory(out, OUT_SIZE);

    status = RtlStringCbLengthA(Prefix, OUT_SIZE, &len);
    if (NT_SUCCESS(status))
    {
        memcpy(out, Prefix, len);

        // prefix already has a space on the end so we can copy the rest straight on to it

        va_start(Arguments, Format);

        status = RtlStringCbVPrintfA(out + len, OUT_SIZE - len, Format, Arguments);

        va_end(Arguments);

        if (NT_SUCCESS(status)) {
            EventWriteCustomEventTraceInfo(NULL, out, 0);
        }
    }
}

#define ETWTrace(...)  \
        __ETWTrace(__FUNCTION__ ": ", __VA_ARGS__)

