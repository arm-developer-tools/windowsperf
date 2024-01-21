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

#include "dsu.h"

static inline VOID
DSUWritePMCR(UINT32 val)
{
    _WriteStatusReg(CLUSTERPMCR_EL1, (INT64)val);
    __isb(_ARM64_BARRIER_SY);
}

static inline VOID
DSUCounterSetType(INT counter, UINT32 event)
{
    DSUSelectCounter(counter);
    _WriteStatusReg(CLUSTERPMXEVTYPER_EL1, event);
    __isb(_ARM64_BARRIER_SY);
}

static inline VOID
DSUCounterDisable(INT counter)
{
    _WriteStatusReg(CLUSTERPMCNTENCLR_EL1, 1LL << counter);
    __isb(_ARM64_BARRIER_SY);
}

static inline VOID
DSUCounterEnable(INT counter)
{
    _WriteStatusReg(CLUSTERPMCNTENSET_EL1, 1LL << counter);
    __isb(_ARM64_BARRIER_SY);
}

UINT64
DSUEventGetCounting(struct pmu_event_kernel* event)
{
    if (event->counter_idx == CYCLE_COUNTER_IDX)
        return DSUReadPMCCNTR();

    return DSUReadCounter(event->counter_idx);
}

VOID
DSUCounterStart(VOID)
{
    UINT32 pmcr = DSUReadPMCR();
    pmcr |= CLUSTERPMCR_E;
    DSUWritePMCR(pmcr);
}

VOID
DSUCounterStop(VOID)
{
    UINT32 pmcr = DSUReadPMCR();
    pmcr &= ~CLUSTERPMCR_E;
    DSUWritePMCR(pmcr);
}

VOID
DSUCounterReset(VOID)
{
    UINT32 pmcr = CLUSTERPMCR_P | CLUSTERPMCR_C;
    DSUWritePMCR(pmcr);
}

VOID
DSUEventEnable(struct pmu_event_kernel* event)
{
    DSUCounterDisable(event->counter_idx);

    if (event->event_idx != CYCLE_EVENT_IDX)
        DSUCounterSetType(event->counter_idx, event->event_idx);

    DSUCounterEnable(event->counter_idx);
}

VOID
DSUProbePMU(OUT UINT8 *dsu_numGPC, OUT UINT32 *dsu_evt_mask_lo, OUT UINT32 *dsu_evt_mask_hi)
{
    *dsu_numGPC = (DSUReadPMCR() >> CLUSTERPMCR_N_SHIFT) & CLUSTERPMCR_N_MASK;
    *dsu_evt_mask_lo = DSUReadPMCEID(0);
    *dsu_evt_mask_hi = DSUReadPMCEID(1);
}

 VOID
 DSUUpdateDSUCounting(IN CoreInfo* core)
{
    UINT32 events_num = core->dsu_events_num;
    struct pmu_event_pseudo* events = core->dsu_events;

    DSUCounterStop();

    for (UINT32 i = 0; i < events_num; i++)
    {
        events[i].value += DSUEventGetCounting((struct pmu_event_kernel*)&events[i]);
        events[i].scheduled += 1;
    }

    DSUCounterReset();
    DSUCounterStart();
}
