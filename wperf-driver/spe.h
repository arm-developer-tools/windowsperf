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

//
// Arm Statistical Profiling Extensions (SPE)
//
#define BIT(nr)                             (1ULL << (nr))
#define PMSCR_EL1_E0SPE_E1SPE               0b11
#define PMBLIMITR_EL1_E                     1ULL
#define PMBSR_EL1_S                         BIT(17)
#define PMBLIMITR_EL1_LIMIT_MASK            (~((UINT64)0xFFF))  // PMBLIMITR.LIMIT, bits [63:12]

#define SPE_MEMORY_BUFFER_SIZE              (PAGE_SIZE*128)     // PAGE_SIZE is defined in WDM.h
#define SPE_TIMER_PERIOD                    500
#define SPE_BUFFER_THRESHOLD                256

typedef struct spe_info_
{
    KTIMER      timer;
    KDPC        dpc_overflow;
    ULONG       idx;

    BOOLEAN     profiling_running;
    BOOLEAN     timer_running;
} SpeInfo;

#define SPE_IOCTL                                                                                                       \
    case IOCTL_PMU_CTL_SPE_GET_SIZE:                                                                                    \
    {                                                                                                                   \
        struct pmu_ctl_hdr* ctl_req = (struct pmu_ctl_hdr*)pInBuffer;                                                   \
        spe_get_size(&queueContext->SpeWorkItem, ctl_req->cores_idx.cores_no[0]);                                       \
        *((size_t*)pOutBuffer) = spe_bytesToCopy;                                                                       \
        *outputSize = sizeof(spe_bytesToCopy);                                                                          \
        break;                                                                                                          \
    }                                                                                                                   \
    case IOCTL_PMU_CTL_SPE_GET_BUFFER:                                                                                  \
    {                                                                                                                   \
        struct spe_ctl_hdr* ctl_req = (struct spe_ctl_hdr*)pInBuffer;                                                   \
        spe_get_buffer(&queueContext->SpeWorkItem, ctl_req->cores_idx.cores_no[0], pOutBuffer, ctl_req->buffer_size);   \
        *outputSize = sizeof(char)*(ULONG)spe_bytesToCopy;                                                              \
        spe_bytesToCopy = 0;                                                                                            \
        break;                                                                                                          \
    }                                                                                                                   \
    case IOCTL_PMU_CTL_SPE_INIT:                                                                                        \
    {                                                                                                                   \
        spe_init(&queueContext->SpeWorkItem);                                                                           \
        *outputSize = 0;                                                                                                \
        break;                                                                                                          \
    }                                                                                                                   \
    case IOCTL_PMU_CTL_SPE_START:                                                                                       \
    {                                                                                                                   \
        struct pmu_ctl_hdr* ctl_req = (struct pmu_ctl_hdr*)pInBuffer;                                                   \
        spe_start(&queueContext->SpeWorkItem, ctl_req->cores_idx.cores_no[0]);                                          \
        *outputSize = 0;                                                                                                \
        break;                                                                                                          \
    }                                                                                                                   \
    case IOCTL_PMU_CTL_SPE_STOP:                                                                                        \
    {                                                                                                                   \
        struct pmu_ctl_hdr* ctl_req = (struct pmu_ctl_hdr*)pInBuffer;                                                   \
        spe_stop(&queueContext->SpeWorkItem, ctl_req->cores_idx.cores_no[0]);                                           \
        *outputSize = 0;                                                                                                \
        break;                                                                                                          \
    }

void spe_get_size(WDFWORKITEM* workItem, UINT32 core_idx);
void spe_get_buffer(WDFWORKITEM* workItem, UINT32 core_idx, PVOID target, UINT64 size);
void spe_init(WDFWORKITEM* workItem);
void spe_start(WDFWORKITEM* workItem, UINT32 core_idx);
void spe_stop(WDFWORKITEM* workItem, UINT32 core_idx);
void spe_destroy();

NTSTATUS spe_setup(ULONG numCores);
void spe_destroy();