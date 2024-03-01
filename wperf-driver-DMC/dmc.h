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



VOID DmcCounterStart(UINT8 ch_idx, UINT8 counter_idx, struct dmcs_desc *dmc_array);
VOID DmcCounterStop(UINT8 ch_idx, UINT8 counter_idx, struct dmcs_desc *dmc_array);
VOID DmcCounterReset(UINT8 ch_idx, UINT8 counter_idx, struct dmcs_desc *dmc_array);

UINT64 DmcCounterRead(UINT8 ch_idx, UINT16 counter_idx, struct dmcs_desc *dmc_array);
VOID DmcChannelIterator(UINT8 ch_base, UINT8 ch_end, VOID(*do_func)(UINT8, UINT8, struct dmcs_desc*), struct dmcs_desc *dmc_array);
VOID DmcEnableEvent(UINT8 ch_idx, UINT32 counter_idx, UINT16 event_idx, struct dmcs_desc *dmc_array);

VOID UpdateDmcCounting(UINT8 dmc_ch, struct dmcs_desc *dmc_array);
