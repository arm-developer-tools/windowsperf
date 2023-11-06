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


#include "driver.h"
#include "device.h"
#if !defined DBG
#include "utilities.tmh"
#endif
#include "sysregs.h"



extern UINT64* last_fpc_read;
extern UINT32 counter_idx_map[AARCH64_MAX_HWC_SUPP + 1];
// Just update last_fpc_read, this is the fixed counter equivalent to CoreCounterReset
void update_last_fixed_counter(UINT64 core_idx)
{
    last_fpc_read[core_idx] = _ReadStatusReg(PMCCNTR_EL0);
}


#define WRITE_COUNTER(N) case N: _WriteStatusReg(PMEVCNTR##N##_EL0, (__int64)val); break;
VOID core_write_counter(UINT32 counter_idx, __int64 val)
{
    counter_idx = counter_idx_map[counter_idx];
    switch (counter_idx)
    {
        WRITE_COUNTER(0);
        WRITE_COUNTER(1);
        WRITE_COUNTER(2);
        WRITE_COUNTER(3);
        WRITE_COUNTER(4);
        WRITE_COUNTER(5);
        WRITE_COUNTER(6);
        WRITE_COUNTER(7);
        WRITE_COUNTER(8);
        WRITE_COUNTER(9);
        WRITE_COUNTER(10);
        WRITE_COUNTER(11);
        WRITE_COUNTER(12);
        WRITE_COUNTER(13);
        WRITE_COUNTER(14);
        WRITE_COUNTER(15);
        WRITE_COUNTER(16);
        WRITE_COUNTER(17);
        WRITE_COUNTER(18);
        WRITE_COUNTER(19);
        WRITE_COUNTER(20);
        WRITE_COUNTER(21);
        WRITE_COUNTER(22);
        WRITE_COUNTER(23);
        WRITE_COUNTER(24);
        WRITE_COUNTER(25);
        WRITE_COUNTER(26);
        WRITE_COUNTER(27);
        WRITE_COUNTER(28);
        WRITE_COUNTER(29);
        WRITE_COUNTER(30);
    default:
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL,
            "WindowsPerf: Warn: Invalid PMEVTYPE index: %d\n", counter_idx));
        break;
    }
}
