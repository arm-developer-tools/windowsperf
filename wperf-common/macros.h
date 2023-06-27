#pragma once
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

#include <limits.h>

//
// Macros used by various parts of the solution
//
#define ALL_CORE                            _UI32_MAX
#define ALL_DMC_CHANNEL                     _UI8_MAX
#define CYCLE_EVENT_IDX                     _UI32_MAX

#define CYCLE_COUNTER_IDX                   31
#define INVALID_COUNTER_IDX                 32

#define MAX_PMU_CTL_CORES_COUNT             128

#define MAX_MANAGED_CORE_EVENTS             128
#define MAX_MANAGED_DSU_EVENTS              32

#define MAX_MANAGED_DMC_CLK_EVENTS          4
#define MAX_MANAGED_DMC_CLKDIV2_EVENTS      8

#define AARCH64_MAX_HWC_SUPP                31

#define SAMPLE_CHAIN_BUFFER_SIZE            128

#define MAX_PROCESSES					1024

#define FILTER_BIT_EXCL_EL1					(1U << 31)

#define CYCLE_EVT_IDX                       0xffffffffU

#define FILTER_BIT_EXCL_EL1                 (1U << 31)

// Define how many fixed counters are now handled
// Currently we are having "cycles" as 1 (only) fixed counter
#define FIXED_COUNTERS_NO                   1

// From handy wdm.h macros
#define CTL_CODE( DeviceType, Function, Method, Access ) (                 \
    ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) \
)

#define METHOD_BUFFERED 0
#define FILE_READ_DATA            ( 0x0001 )    // file & pipe
#define FILE_WRITE_DATA           ( 0x0002 )    // file & pipe

#define WPERF_TYPE 40004

#define DEFINE_CUSTOM_IOCTL(X) CTL_CODE( WPERF_TYPE, X, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA  )
#define DEFINE_CUSTOM_IOCTL_RUNTIME(TO,FROM) {    \
    (TO) = ((WPERF_TYPE) << 16) | ((FILE_READ_DATA | FILE_WRITE_DATA) << 14) | ((FROM) << 2) | (METHOD_BUFFERED);    \
}
