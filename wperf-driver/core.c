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

#include <ntddk.h>
#include "trace.h"
#include "core.h"
#if !defined DBG
#include "core.tmh"
#endif
#include "sysregs.h"

static UINT8 has_long_event_support = 0;

VOID CpuHasLongEventSupportSet(UINT8 has_long_event_support_) {
    has_long_event_support = has_long_event_support_;
};

UINT32 CorePmcrGet(VOID)
{
    return (UINT32)_ReadStatusReg(PMCR_EL0);
}

VOID CorePmcrSet(UINT32 val)
{
    val &= ARMV8_PMCR_MASK;
    __isb(_ARM64_BARRIER_SY);
    _WriteStatusReg(PMCR_EL0, (__int64)val);
}

VOID CoreCounterDisable(UINT32 mask)
{
    _WriteStatusReg(PMCNTENCLR_EL0, (__int64)mask);
    __isb(_ARM64_BARRIER_SY);
}

VOID CoreCounterEnable(UINT32 mask)
{
    __isb(_ARM64_BARRIER_SY);
    _WriteStatusReg(PMCNTENSET_EL0, (__int64)mask);
}

VOID CoreCounterIrqDisable(UINT32 mask)
{
    _WriteStatusReg(PMINTENCLR_EL1, (__int64)mask);
    __isb(_ARM64_BARRIER_SY);
    _WriteStatusReg(PMOVSCLR_EL0, (__int64)mask);
    __isb(_ARM64_BARRIER_SY);
}

VOID CoreCounterReset(VOID)
{
    // Removed (ARMV8_PMCR_C | ARMV8_PMCR_LC) to avoid reseting the 
    // fixed counter as some architectures make use of it for system measurements.
    UINT32 pmcr = ARMV8_PMCR_P;
    if (has_long_event_support)
        pmcr |= ARMV8_PMCR_LP;

    CorePmcrSet(pmcr);
}

VOID CoreCounterStart(VOID)
{
    CorePmcrSet(CorePmcrGet() | ARMV8_PMCR_E);
}

VOID CoreCounterStop(VOID)
{
    CorePmcrSet(CorePmcrGet() & ~ARMV8_PMCR_E);
}

VOID CoreCounterEnableIrq(UINT32 mask)
{
    _WriteStatusReg(PMINTENSET_EL1, (__int64)mask);
}

#define SET_COUNTER_TYPE(N) case N: _WriteStatusReg(PMEVTYPER##N##_EL0, evtype_val); break
VOID CoreCouterSetType(UINT32 counter_idx, __int64 evtype_val)
{
    evtype_val &= ARMV8_EVTYPE_MASK;
    // can't do lookup table as _WriteStatusReg takes contant register number
    switch (counter_idx)
    {
        SET_COUNTER_TYPE(0);
        SET_COUNTER_TYPE(1);
        SET_COUNTER_TYPE(2);
        SET_COUNTER_TYPE(3);
        SET_COUNTER_TYPE(4);
        SET_COUNTER_TYPE(5);
        SET_COUNTER_TYPE(6);
        SET_COUNTER_TYPE(7);
        SET_COUNTER_TYPE(8);
        SET_COUNTER_TYPE(9);
        SET_COUNTER_TYPE(10);
        SET_COUNTER_TYPE(11);
        SET_COUNTER_TYPE(12);
        SET_COUNTER_TYPE(13);
        SET_COUNTER_TYPE(14);
        SET_COUNTER_TYPE(15);
        SET_COUNTER_TYPE(16);
        SET_COUNTER_TYPE(17);
        SET_COUNTER_TYPE(18);
        SET_COUNTER_TYPE(19);
        SET_COUNTER_TYPE(20);
        SET_COUNTER_TYPE(21);
        SET_COUNTER_TYPE(22);
        SET_COUNTER_TYPE(23);
        SET_COUNTER_TYPE(24);
        SET_COUNTER_TYPE(25);
        SET_COUNTER_TYPE(26);
        SET_COUNTER_TYPE(27);
        SET_COUNTER_TYPE(28);
        SET_COUNTER_TYPE(29);
        SET_COUNTER_TYPE(30);
    default:
        KdPrintEx((DPFLTR_IHVDRIVER_ID,  DPFLTR_TRACE_LEVEL, "Warn: Invalid PMEVTYPE index: %d\n", counter_idx));
        break;
    }
}

#define READ_COUNTER(N) case N: return (UINT64)_ReadStatusReg(PMEVCNTR##N##_EL0)
UINT64 CoreReadCounter(UINT32 counter_idx)
{
    switch (counter_idx)
    {
        READ_COUNTER(0);
        READ_COUNTER(1);
        READ_COUNTER(2);
        READ_COUNTER(3);
        READ_COUNTER(4);
        READ_COUNTER(5);
        READ_COUNTER(6);
        READ_COUNTER(7);
        READ_COUNTER(8);
        READ_COUNTER(9);
        READ_COUNTER(10);
        READ_COUNTER(11);
        READ_COUNTER(12);
        READ_COUNTER(13);
        READ_COUNTER(14);
        READ_COUNTER(15);
        READ_COUNTER(16);
        READ_COUNTER(17);
        READ_COUNTER(18);
        READ_COUNTER(19);
        READ_COUNTER(20);
        READ_COUNTER(21);
        READ_COUNTER(22);
        READ_COUNTER(23);
        READ_COUNTER(24);
        READ_COUNTER(25);
        READ_COUNTER(26);
        READ_COUNTER(27);
        READ_COUNTER(28);
        READ_COUNTER(29);
        READ_COUNTER(30);
    default:
        KdPrintEx((DPFLTR_IHVDRIVER_ID,  DPFLTR_TRACE_LEVEL, "Warn: Invalid PMEVTYPE index: %d\n", counter_idx));
        break;
    }

    return 0xcafedead;
}
