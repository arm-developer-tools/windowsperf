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

#include "wperf-common\public.h"
#include "queue.h"
//
// The device context performs the same job as
// a WDM device extension in the driver frameworks
//
typedef struct _DEVICE_EXTENSION
{
    ULONG PrivateDeviceData;  // just a placeholder
    USHORT InUse;
    USHORT AskedToRemove;
    PQUEUE_CONTEXT  pQueContext;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

//
// This macro will generate an inline function called GetDeviceExtension
// which will be used to get a pointer to the device context memory
// in a type safe manner.
//
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_EXTENSION, GetDeviceExtension)


//
// Function to initialize the device and its callbacks
//
NTSTATUS
WindowsPerfDeviceCreate(
    PWDFDEVICE_INIT DeviceInit
    );

VOID multiplex_dpc(struct _KDPC* dpc, PVOID ctx, PVOID sys_arg1, PVOID sys_arg2);

VOID overflow_dpc(struct _KDPC* dpc, PVOID ctx, PVOID sys_arg1, PVOID sys_arg2);

VOID reset_dpc(struct _KDPC* dpc, PVOID ctx, PVOID sys_arg1, PVOID sys_arg2);

VOID arm64pmc_enable_default(struct _KDPC* dpc, PVOID ctx, PVOID sys_arg1, PVOID sys_arg2);

VOID free_pmu_resource(VOID);

NTSTATUS get_pmu_resource(VOID);
