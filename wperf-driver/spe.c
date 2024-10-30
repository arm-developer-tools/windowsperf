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
#include "driver.h"
#include "spe.h"
#include "sysregs.h"
#if defined ENABLE_TRACING
#include "spe.tmh"
#endif

SpeInfo* spe_info = NULL;
static __declspec(align(64)) unsigned char SpeMemoryBuffer[SPE_MEMORY_BUFFER_SIZE];
static __declspec(align(64)) unsigned char* SpeMemoryBufferLimit = NULL;
static unsigned char* lastCopiedPtr = NULL;

size_t spe_bytesToCopy = 0;

static ULONG totalCores = 0;

#define START_WORK_ON_CORE(CORE_IDX)                                    \
    GROUP_AFFINITY old_affinity, new_affinity;                          \
    PROCESSOR_NUMBER ProcNumber;                                        \
    RtlSecureZeroMemory(&new_affinity, sizeof(GROUP_AFFINITY));         \
    RtlSecureZeroMemory(&old_affinity, sizeof(GROUP_AFFINITY));         \
    RtlSecureZeroMemory(&ProcNumber, sizeof(PROCESSOR_NUMBER));         \
    KeGetProcessorNumberFromIndex((CORE_IDX), &ProcNumber);             \
    new_affinity.Group = ProcNumber.Group;                              \
    new_affinity.Mask = 1ULL << (ProcNumber.Number);                    \
    KeSetSystemGroupAffinityThread(&new_affinity, &old_affinity);                                                                          

#define STOP_WORK_ON_CORE()  KeRevertToUserGroupAffinityThread(&old_affinity);

VOID SPEWorkItemFunc(WDFWORKITEM WorkItem)
{
	PSPE_WORK_ITEM_CTXT context;
	context = WdfObjectGet_SPE_WORK_ITEM_CTXT(WorkItem);

    switch (context->action)
    {
        case PMU_CTL_SPE_START:
        {
            RtlSecureZeroMemory(SpeMemoryBuffer, sizeof(SpeMemoryBuffer));

            START_WORK_ON_CORE(context->core_idx);
            
            _WriteStatusReg(PMBPTR_EL1, (UINT64)SpeMemoryBuffer);
           
            if(context->operation_filter)
            {
                _WriteStatusReg(PMSFCR_EL1, PMSFCR_EL1_FT | ((UINT64)context->operation_filter << 16));
            }
            else {
                _WriteStatusReg(PMSFCR_EL1, 0);
            }

            /*
            * Writing to PMSIRR_EL1 and PMSICR_EL1 seems to be innefective for some reason.
            * When PMSIRR_EL1 is written to its value just goes to 0 and PMSICR_EL1 seems to be unchanged.
            * At least this is what can be seen in the logs.
            * This looks like an unexpected behaviour as the documentation seems to imply that 
            * PMSCIR_EL1 needs to be zeroed before sampling starts and PMSIRR_EL1.Interval needs to be set. 
            _WriteStatusReg(PMSICR_EL1, 0);
            if (context->config_flags & SPE_CTL_FLAG_RND)
            {
                _WriteStatusReg(PMSIRR_EL1, PMSIRR_EL1_RND | ((UINT64)context->interval << 8));
            }
            else {
                _WriteStatusReg(PMSIRR_EL1, (UINT64)context->interval << 8);
            }
            */

            if (context->config_flags & SPE_CTL_FLAG_TS)
            {
                // Enable timestamps with ts_enable filter:
                _WriteStatusReg(PMSICR_EL1, _ReadStatusReg(PMSICR_EL1) | BIT(5));   // TS, bit [5]
                KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "SPE: ts_enable=1 PMSICR_EL1=0x%llX\n", _ReadStatusReg(PMSICR_EL1)));
            }

            _WriteStatusReg(PMBSR_EL1, _ReadStatusReg(PMBSR_EL1) & (~PMBSR_EL1_S)); // Clear PMBSR_EL1.S
            //PMBPTR_EL1[63:56] must equal PMBLIMITR_EL1.LIMIT[63:56]
            _WriteStatusReg(PMBLIMITR_EL1, (UINT64)SpeMemoryBufferLimit | PMBLIMITR_EL1_E); // Enable PMBLIMITR_ELI1.E
            _WriteStatusReg(PMSCR_EL1, _ReadStatusReg(PMSCR_EL1) | PMSCR_EL1_E0SPE_E1SPE); // Enable PMSCR_EL1.{E0SPE,E1SPE}

            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Statistical Profiling Extension: memory buffer 0x%llX\n", _ReadStatusReg(PMBPTR_EL1)));
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Statistical Profiling Extension: memory buffer limit address %llX\n", _ReadStatusReg(PMBLIMITR_EL1)));
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Statistical Profiling Extension: memory buffer real address %llX\n", (UINT64)SpeMemoryBuffer));
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Statistical Profiling Extension: memory buffer real limit address %llX\n", (UINT64)SpeMemoryBufferLimit));
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Statistical Profiling Extension: sampling profile ID register %llX\n", _ReadStatusReg(PMSIDR_EL1)));
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Statistical Profiling Extension: PMSFCR_EL1 0x%llX\n", _ReadStatusReg(PMSFCR_EL1)));
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Statistical Profiling Extension: PMSEVFR_EL1 0x%llX\n", _ReadStatusReg(PMSEVFR_EL1)));
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Statistical Profiling Extension: PMSICR_EL1 0x%llX\n", _ReadStatusReg(PMSICR_EL1)));
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Statistical Profiling Extension: PMSIRR_EL1 0x%llX\n", _ReadStatusReg(PMSIRR_EL1)));

            STOP_WORK_ON_CORE();
            break;
        }
        case PMU_CTL_SPE_GET_SIZE:
        {
            START_WORK_ON_CORE(context->core_idx);
            UINT64 currentBufferPtr = _ReadStatusReg(PMBPTR_EL1);

            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Statistical Profiling Extension: PMSICR_EL1 0x%llX\n", _ReadStatusReg(PMSICR_EL1)));

            STOP_WORK_ON_CORE();

            spe_bytesToCopy += (currentBufferPtr - (UINT64)lastCopiedPtr);
            break;
        }
        case PMU_CTL_SPE_STOP:
        {
            START_WORK_ON_CORE(context->core_idx);

            _WriteStatusReg(PMBLIMITR_EL1, 0); // Disable PMBLIMITR_ELI1.E
            _WriteStatusReg(PMSCR_EL1, _ReadStatusReg(PMSCR_EL1) & (~PMSCR_EL1_E0SPE_E1SPE)); // Disable PMSCR_EL1.{E0SPE,E1SPE}

            _WriteStatusReg(PMBSR_EL1, _ReadStatusReg(PMBSR_EL1) & (~PMBSR_EL1_S)); // Clear PMBSR_EL1.S
            
            STOP_WORK_ON_CORE();

            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Statistical Profiling Extension: memory buffer 0x%llX\n", _ReadStatusReg(PMBPTR_EL1)));
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Statistical Profiling Extension: memory buffer limit address %llX\n", _ReadStatusReg(PMBLIMITR_EL1)));
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Statistical Profiling Extension: profiling buffer status/syndrome %llX\n", _ReadStatusReg(PMBSR_EL1)));

            break;
        }
    }
}

// mitigating lack of interrupt handler for buffer full event.
static VOID dpc_spe_overflow(struct _KDPC* dpc, PVOID ctx, PVOID sys_arg1, PVOID sys_arg2)
{
    UNREFERENCED_PARAMETER(dpc);
    UNREFERENCED_PARAMETER(sys_arg1);
    UNREFERENCED_PARAMETER(sys_arg2);

    if (ctx == NULL) return;

    SpeInfo* spu = (SpeInfo*)ctx;
    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "SPE_DPC core_idx %u running at %u\n", spu->idx, KeGetCurrentProcessorNumberEx(NULL)));
    if(spu->profiling_running == TRUE)
    {
        UINT64 currentBufferPtr = _ReadStatusReg(PMBPTR_EL1);
        if (currentBufferPtr - (UINT64)SpeMemoryBufferLimit <= SPE_BUFFER_THRESHOLD)
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "SPE_DPC profiling buffer full\n"));
            //Disable sampling
            _WriteStatusReg(PMBLIMITR_EL1, 0); // Disable PMBLIMITR_ELI1.E
            _WriteStatusReg(PMBSR_EL1, _ReadStatusReg(PMBSR_EL1) & (~PMBSR_EL1_S)); // Clear PMBSR_EL1.S
            _WriteStatusReg(PMSCR_EL1, _ReadStatusReg(PMSCR_EL1) & (~PMSCR_EL1_E0SPE_E1SPE)); // Disable PMSCR_EL1.{E0SPE,E1SPE}
            spu->profiling_running = FALSE;
        }
    }
}

NTSTATUS spe_setup(ULONG numCores)
{
#ifdef ENABLE_SPE
    spe_info = (SpeInfo*)ExAllocatePool2(POOL_FLAG_NON_PAGED, sizeof(SpeInfo) * numCores, 'SPE');
    if (spe_info == NULL)
    {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "ExAllocatePoolWithTag: failed \n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlSecureZeroMemory(spe_info, sizeof(SpeInfo) * numCores);
    
    totalCores = numCores;

    SpeMemoryBufferLimit = (unsigned char*)(((UINT64)SpeMemoryBuffer + SPE_MEMORY_BUFFER_SIZE) & PMBLIMITR_EL1_LIMIT_MASK);

    for (ULONG i = 0; i < numCores; i++)
    {
        SpeInfo* spu = &spe_info[i];
        spu->profiling_running = FALSE;
        spu->timer_running = FALSE;
        spu->idx = i;

        PROCESSOR_NUMBER ProcNumber;
        NTSTATUS status = KeGetProcessorNumberFromIndex(i, &ProcNumber);
        if (status != STATUS_SUCCESS)
            return status;

        PRKDPC dpc_overflow = &spu->dpc_overflow;

        KeInitializeDpc(dpc_overflow, dpc_spe_overflow, spu);
        KeSetTargetProcessorDpcEx(dpc_overflow, &ProcNumber);
        KeSetImportanceDpc(dpc_overflow, HighImportance);
    }
#else
    UNREFERENCED_PARAMETER(numCores);
#endif
    return STATUS_SUCCESS;
}

void spe_destroy()
{
#ifdef ENABLE_SPE
    if (spe_info)
    {
        for (ULONG i = 0; i < totalCores; i++)
        {
            SpeInfo* spu = &spe_info[i];
            KeCancelTimer(&spu->timer);
            spu->timer_running = FALSE;

            KeRemoveQueueDpc(&spu->dpc_overflow);
        }
        ExFreePoolWithTag(spe_info, 'SPE');
    }
#endif
}

void spe_get_size(WDFWORKITEM* workItem, UINT32 core_idx)
{
#ifdef ENABLE_SPE
    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL: PMU_CTL_SPE_GET_SIZE\n"));

    PSPE_WORK_ITEM_CTXT context;
    context = WdfObjectGet_SPE_WORK_ITEM_CTXT(*workItem);
    context->action = PMU_CTL_SPE_GET_SIZE;
    context->core_idx = core_idx;
    WdfWorkItemEnqueue(*workItem);
    WdfWorkItemFlush(*workItem);

    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL: PMU_CTL_SPE_GET_SIZE sending %llu\n", spe_bytesToCopy));
#else
    UNREFERENCED_PARAMETER(workItem);
    UNREFERENCED_PARAMETER(core_idx);
#endif
}

void spe_get_buffer(WDFWORKITEM* workItem, UINT32 core_idx, PVOID target, UINT64 size)
{
#ifdef ENABLE_SPE
    UNREFERENCED_PARAMETER(workItem);
    UNREFERENCED_PARAMETER(core_idx);
    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL: PMU_CTL_SPE_GET_BUFFER target %llX %llu\n", (UINT64)target, size));
    RtlCopyMemory(target, lastCopiedPtr, sizeof(unsigned char)*size);
    lastCopiedPtr += size;
#else
    UNREFERENCED_PARAMETER(workItem);
    UNREFERENCED_PARAMETER(core_idx);
    UNREFERENCED_PARAMETER(target);
    UNREFERENCED_PARAMETER(size);
#endif
}

void spe_init(WDFWORKITEM* workItem)
{
#ifdef ENABLE_SPE
    UNREFERENCED_PARAMETER(workItem);

    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL: PMU_CTL_SPE_INIT\n"));

    lastCopiedPtr = SpeMemoryBuffer;
    spe_bytesToCopy = 0;
#else
    UNREFERENCED_PARAMETER(workItem);
#endif
}

void spe_start(WDFWORKITEM* workItem, struct spe_ctl_hdr *req)
{
    UINT32 core_idx = req->cores_idx.cores_no[0];

#ifdef ENABLE_SPE
    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL: PMU_CTL_SPE_START core_idx %u\n", core_idx));

    if (spe_info[core_idx].timer_running == TRUE) KeCancelTimer(&spe_info[core_idx].timer);

    KeInitializeTimer(&spe_info[core_idx].timer);
    
    const LONGLONG ns100 = -10000; // negative, the expiration time is relative to the current system time
    LARGE_INTEGER DueTime;
    LONG Period = SPE_TIMER_PERIOD;
    DueTime.QuadPart = Period * ns100;

    KeSetTimerEx(&spe_info[core_idx].timer, DueTime, Period, &spe_info[core_idx].dpc_overflow);
    
    spe_info[core_idx].profiling_running = TRUE;
    spe_info[core_idx].timer_running = TRUE;
    
    PSPE_WORK_ITEM_CTXT context;
    context = WdfObjectGet_SPE_WORK_ITEM_CTXT(*workItem);
    context->action = PMU_CTL_SPE_START;
    context->core_idx = core_idx;
    context->event_filter = req->event_filter;
    context->operation_filter = req->operation_filter;
    context->config_flags = req->config_flags;
    context->interval = req->interval;
    WdfWorkItemEnqueue(*workItem);
    WdfWorkItemFlush(*workItem);
#else
    UNREFERENCED_PARAMETER(workItem);
    UNREFERENCED_PARAMETER(core_idx);
#endif
}

void spe_stop(WDFWORKITEM* workItem, UINT32 core_idx)
{
#ifdef ENABLE_SPE
    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL: PMU_CTL_SPE_STOP core_idx %u\n", core_idx));

    PSPE_WORK_ITEM_CTXT context;
    context = WdfObjectGet_SPE_WORK_ITEM_CTXT(*workItem);
    context->action = PMU_CTL_SPE_STOP;
    context->core_idx = core_idx;
    WdfWorkItemEnqueue(*workItem);
    WdfWorkItemFlush(*workItem);
    
    spe_info[core_idx].profiling_running = FALSE;
    spe_info[core_idx].timer_running = FALSE;

    KeCancelTimer(&spe_info[core_idx].timer);
#else
    UNREFERENCED_PARAMETER(workItem);
    UNREFERENCED_PARAMETER(core_idx);
#endif
}