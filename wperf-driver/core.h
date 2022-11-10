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

#include "wperf-common\macros.h"
#include "pmu.h"

#define ARMV8_PMCR_MASK         0xff
#define ARMV8_PMCR_E            (1 << 0) /*  Enable all counters */
#define ARMV8_PMCR_P            (1 << 1) /*  Reset all counters */
#define ARMV8_PMCR_C            (1 << 2) /*  Cycle counter reset */
#define ARMV8_PMCR_LC           (1 << 6) /*  Overflow on 64-bit cycle counter*/
#define ARMV8_PMCR_LP           (1 << 7) /*  Long event counter enable */
#define	FILTER_EXCL_EL1	(1U << 31)
#define ARMV8_EVTYPE_MASK   0xc800ffff  // Mask for writable bits

VOID CpuHasLongEventSupportSet(UINT8 has_long_event_support_);
UINT32 CorePmcrGet(VOID);
VOID CorePmcrSet(UINT32 val);
VOID CoreCounterDisable(UINT32 mask);
VOID CoreCounterEnable(UINT32 mask);
VOID CoreCounterReset(VOID);
VOID CoreCounterIrqDisable(UINT32 mask);
VOID CoreCounterStart(VOID);
VOID CoreCounterStop(VOID);
VOID CoreCouterSetType(UINT32 counter_idx, __int64 evtype_val);
UINT64 CoreReadCounter(UINT32 counter_idx);