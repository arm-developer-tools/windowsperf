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

#define DMC_COUNTER_BASE(val)			(0x10 + val * 0x28)
#define DMC_COUNTER_CTL_OFFSET			(0x10)
#define DMC_COUNTER_VAL_OFFSET			(0x20)
#define DMC_CTL_BIT_ENABLE				(0x01 << 0)
#define DMC_CTL_BIT_EMUX				(0x1F << 2)
#define DMC_CLKDIV2_NUMGPC				8
#define DMC_CLK_NUMGPC					2

enum dmc_clkdiv2_event
{
#define WPERF_DMC_CLKDIV2_EVENTS(a,b,c) DMC_CLKDIV2_##a,
#include "wperf-common\dmc-clkdiv2-events.def"
#undef WPERF_DMC_CLKDIV2_EVENTS
    DMC_CLKDIV2_EVT_NUM,
};

enum dmc_clk_event
{
#define WPERF_DMC_CLK_EVENTS(a,b,c) DMC_CLK_##a,
#include "wperf-common\dmc-clk-events.def"
#undef WPERF_DMC_CLK_EVENTS
    DMC_CLK_EVT_NUM,
};
