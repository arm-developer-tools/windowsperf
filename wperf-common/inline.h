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

#include "wperf-common\macros.h"
#include "wperf-common\iorequest.h"

#ifndef __cplusplus
#define bool                _Bool
#define true                TRUE
#define false               FALSE
#endif

/// <summary>
/// Check if structure `pmu_ctl_cores_count_hdr` stores correct
/// number of cores and correct core indexes.
/// Both values are defined with MAX_PMU_CTL_CORES_COUNT.
/// </summary>
/// <param name="ctl_req">Pointer to structure to check</param>
/// <returns>TRUE if cores_count and cores_no are in range</returns>
bool check_cores_in_pmu_ctl_hdr_p(const struct pmu_ctl_hdr* ctl_req)
{
    if (!ctl_req)
        return false;

    size_t cores_count = ctl_req->cores_idx.cores_count;

    if (cores_count >= MAX_PMU_CTL_CORES_COUNT)
        return false;

    for (auto k = 0; k < cores_count; k++)
        if (ctl_req->cores_idx.cores_no[k] >= MAX_PMU_CTL_CORES_COUNT)
            return false;
    return true;
}
