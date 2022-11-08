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
#include "dmc.h"

VOID DmcCounterStart(UINT8 ch_idx, UINT8 counter_idx, struct dmcs_desc *dmc_array)
{
    UINT64 op_base = dmc_array->dmcs[ch_idx].iomem_start;

    __int32 value = __iso_volatile_load32((volatile __int32*)(op_base + DMC_COUNTER_BASE(counter_idx) + DMC_COUNTER_CTL_OFFSET));
    __dmb(_ARM64_BARRIER_LD);

    value |= DMC_CTL_BIT_ENABLE;

    __dmb(_ARM64_BARRIER_ST);
    __iso_volatile_store32((volatile __int32*)(op_base + DMC_COUNTER_BASE(counter_idx) + DMC_COUNTER_CTL_OFFSET), value);
}

VOID DmcCounterStop(UINT8 ch_idx, UINT8 counter_idx, struct dmcs_desc *dmc_array)
{
    UINT64 op_base = dmc_array->dmcs[ch_idx].iomem_start;

    __int32 value = __iso_volatile_load32((volatile __int32*)(op_base + DMC_COUNTER_BASE(counter_idx) + DMC_COUNTER_CTL_OFFSET));
    __dmb(_ARM64_BARRIER_LD);

    value &= ~((__int32)DMC_CTL_BIT_ENABLE);

    __dmb(_ARM64_BARRIER_ST);
    __iso_volatile_store32((volatile __int32*)(op_base + DMC_COUNTER_BASE(counter_idx) + DMC_COUNTER_CTL_OFFSET), value);
}

VOID DmcCounterReset(UINT8 ch_idx, UINT8 counter_idx, struct dmcs_desc *dmc_array)
{
    UINT64 op_base = dmc_array->dmcs[ch_idx].iomem_start;

    __int32 value = 0;
    __dmb(_ARM64_BARRIER_ST);
    __iso_volatile_store32((volatile __int32*)(op_base + DMC_COUNTER_BASE(counter_idx) + DMC_COUNTER_VAL_OFFSET), value);
}

UINT64 DmcCounterRead(UINT8 ch_idx, UINT16 counter_idx, struct dmcs_desc *dmc_array)
{
    UINT64 op_base = dmc_array->dmcs[ch_idx].iomem_start;

    __int32 value = __iso_volatile_load32((volatile __int32*)(op_base + DMC_COUNTER_BASE(counter_idx) + DMC_COUNTER_VAL_OFFSET));
    __dmb(_ARM64_BARRIER_LD);

    return (UINT64)((UINT32)value);
}

VOID DmcChannelIterator(UINT8 ch_base, UINT8 ch_end, VOID(*do_func)(UINT8, UINT8, struct dmcs_desc*), struct dmcs_desc *dmc_array)
{
    for (UINT8 ch_idx = ch_base; ch_idx < ch_end; ch_idx++)
    {
        struct dmc_desc* dmc = dmc_array->dmcs + ch_idx;

        for (UINT8 counter_idx = 0; counter_idx < dmc->clk_events_num; counter_idx++)
            do_func(ch_idx, DMC_CLKDIV2_NUMGPC + counter_idx, dmc_array);
        for (UINT8 counter_idx = 0; counter_idx < dmc->clkdiv2_events_num; counter_idx++)
            do_func(ch_idx, counter_idx, dmc_array);
    }
}

VOID DmcEnableEvent(UINT8 ch_idx, UINT32 counter_idx, UINT16 event_idx, struct dmcs_desc *dmc_array)
{
    UINT64 op_base = dmc_array->dmcs[ch_idx].iomem_start;

    __int32 value = __iso_volatile_load32((volatile __int32*)(op_base + DMC_COUNTER_BASE(counter_idx) + DMC_COUNTER_CTL_OFFSET));
    __dmb(_ARM64_BARRIER_LD);

    value &= ~((__int32)DMC_CTL_BIT_EMUX);
    value |= (event_idx << 2) & DMC_CTL_BIT_EMUX;

    __dmb(_ARM64_BARRIER_ST);
    __iso_volatile_store32((volatile __int32*)(op_base + DMC_COUNTER_BASE(counter_idx) + DMC_COUNTER_CTL_OFFSET), value);
}
