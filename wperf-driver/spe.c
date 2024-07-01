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
static __declspec(align(64)) unsigned char* SpeMemoryBufferLimit = 0;
static unsigned char* lastCopiedPtr = NULL;

size_t spe_bytesToCopy = 0;

static ULONG totalCores = 0;

#define BIT(nr)                 (1ULL << (nr))
#define PMSCR_EL1_E0SPE_E1SPE   0b11
#define PMBLIMITR_EL1_E         1ull
#define PMBSR_EL1_S             BIT(17)

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

VOID SetSPECountRegister(UINT32 count1, UINT32 count2)
{
    _WriteStatusReg(PMSICR_EL1, count1 | ((UINT64)count2 << 56)); // Set count register
}

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
            //PMBPTR_EL1[63:56] must equal PMBLIMITR_EL1.LIMIT[63:56]
            _WriteStatusReg(PMBLIMITR_EL1, (UINT64)SpeMemoryBufferLimit | PMBLIMITR_EL1_E); // Enable PMBLIMITR_ELI1.E

            _WriteStatusReg(PMSICR_EL1, 0); // Set count register

            _WriteStatusReg(PMBSR_EL1, _ReadStatusReg(PMBSR_EL1) & (~PMBSR_EL1_S)); // Clear PMBSR_EL1.S
            _WriteStatusReg(PMSCR_EL1, _ReadStatusReg(PMSCR_EL1) | PMSCR_EL1_E0SPE_E1SPE); // Enable PMSCR_EL1.{E0SPE,E1SPE}
            SetSPECountRegister(1024, 1024);

            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Statistical Profiling Extension: memory buffer 0x%llX\n", _ReadStatusReg(PMBPTR_EL1)));
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Statistical Profiling Extension: memory buffer limit address %llX\n", _ReadStatusReg(PMBLIMITR_EL1)));
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Statistical Profiling Extension: memory buffer real address %llX\n", (UINT64)SpeMemoryBuffer));
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Statistical Profiling Extension: memory buffer real limit address %llX\n", (UINT64)SpeMemoryBufferLimit));
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Statistical Profiling Extension: sampling profile ID register %llX\n", _ReadStatusReg(PMSIDR_EL1)));
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Statistical Profiling Extension: PMSFCR_EL1 0x%llX\n", _ReadStatusReg(PMSFCR_EL1)));
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Statistical Profiling Extension: PMSEVFR_EL1 0x%llX\n", _ReadStatusReg(PMSEVFR_EL1)));

            STOP_WORK_ON_CORE();
            break;
        }
        case PMU_CTL_SPE_GET_SIZE:
        {
            START_WORK_ON_CORE(context->core_idx);
            UINT64 currentBufferPtr = _ReadStatusReg(PMBPTR_EL1);
            STOP_WORK_ON_CORE();

            spe_bytesToCopy += (currentBufferPtr - (UINT64)lastCopiedPtr);
            break;
        }
        case PMU_CTL_SPE_STOP:
        {
            START_WORK_ON_CORE(context->core_idx);

            _WriteStatusReg(PMBLIMITR_EL1, 0); // Disable PMBLIMITR_ELI1.E
            _WriteStatusReg(PMBSR_EL1, _ReadStatusReg(PMBSR_EL1) & (~PMBSR_EL1_S)); // Clear PMBSR_EL1.S
            _WriteStatusReg(PMSCR_EL1, _ReadStatusReg(PMSCR_EL1) & (~PMSCR_EL1_E0SPE_E1SPE)); // Disable PMSCR_EL1.{E0SPE,E1SPE}
            
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

    SpeMemoryBufferLimit = (unsigned char*)(((UINT64)SpeMemoryBuffer + 2 * SPE_MEMORY_BUFFER_SIZE) & (~0xfff));

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

void spe_start(WDFWORKITEM* workItem, UINT32 core_idx)
{
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