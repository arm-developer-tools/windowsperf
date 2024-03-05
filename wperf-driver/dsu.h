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
#include "wperf-common\macros.h"
#include "pmu.h"
#include "coreinfo.h"
#include "sysregs.h"

/*
    DSU device description STRING, used by user space to distinguish devices in driver(s).
*/
// DSU device for wperf `stat` command supports `dsu` prefixed events.
#define WPERF_HW_CFG_CAPS_CORE_DSU      L"dsu.stat=dsu"

// DSU
#define CLUSTERPMCR_E               0x01
#define CLUSTERPMCR_P               0x02
#define CLUSTERPMCR_C               0x04
#define CLUSTERPMCR_N_SHIFT         0x0B
#define CLUSTERPMCR_N_MASK          0x1F

//
// DSU public interface
//
VOID DSUCounterStart(VOID);
VOID DSUCounterStop(VOID);
VOID DSUCounterReset(VOID);
VOID DSUEventEnable(struct pmu_event_kernel* event);
UINT64 DSUEventGetCounting(struct pmu_event_kernel* event);
VOID DSUProbePMU(OUT UINT8* dsu_numGPC, OUT UINT32* dsu_evt_mask_lo, OUT UINT32* dsu_evt_mask_hi);
VOID DSUUpdateDSUCounting(IN CoreInfo* core);

//
// Inline functions
//
inline UINT32
DSUReadPMCR(VOID)
{
    return (UINT32)_ReadStatusReg(CLUSTERPMCR_EL1);
}

inline UINT32
DSUReadPMCEID(INT n)
{
    switch (n)
    {
    case 0: return (UINT32)_ReadStatusReg(CLUSTERPMCEID0_EL1);
    case 1: return (UINT32)_ReadStatusReg(CLUSTERPMCEID1_EL1);
    default:
        return 0;
    }
}

inline VOID
DSUSelectCounter(INT counter)
{
    _WriteStatusReg(CLUSTERPMSELR_EL1, counter);
    __isb(_ARM64_BARRIER_SY);
}

inline UINT64
DSUReadCounter(INT counter)
{
    DSUSelectCounter(counter);
    return _ReadStatusReg(CLUSTERPMXEVCNTR_EL1);
}

inline UINT64
DSUReadPMCCNTR(VOID)
{
    return _ReadStatusReg(CLUSTERPMCCNTR_EL1);
}

