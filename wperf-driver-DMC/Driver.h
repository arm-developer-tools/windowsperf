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

#include <ntddk.h>
#include <wdf.h>
#include <initguid.h>
#include "..\wperf-common\iorequest.h"
#include "..\wperf-common\macros.h"
#include "..\wperf-common\public.h"
#include "device.h"
#include "IO.h"
#include "utilities.h"
#include "dmc.h"
#include "core.h"
#include "sysregs.h"

#if defined ENABLE_TRACING
#include "trace.h"
#endif


//
// WDFDRIVER Events
//

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD WperfDriver_TEvtDeviceAdd;
EVT_WDF_DRIVER_UNLOAD WperfDriver_TEvtDriverUnload;


VOID multiplex_dpc(struct _KDPC* dpc, PVOID ctx, PVOID sys_arg1, PVOID sys_arg2);

VOID overflow_dpc(struct _KDPC* dpc, PVOID ctx, PVOID sys_arg1, PVOID sys_arg2);

VOID reset_dpc(struct _KDPC* dpc, PVOID ctx, PVOID sys_arg1, PVOID sys_arg2);

VOID arm64pmc_enable_default(struct _KDPC* dpc, PVOID ctx, PVOID sys_arg1, PVOID sys_arg2);

VOID event_enable(PDEVICE_EXTENSION devExt, ppmu_event_kernel evt);

VOID free_pmu_resource(PDEVICE_EXTENSION devExt);

NTSTATUS get_pmu_resource(PDEVICE_EXTENSION devExt);