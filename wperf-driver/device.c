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
#include "device.tmh"
#endif

#include "dmc.h"
#include "core.h"
#include "spe.h"
#include "coreinfo.h"
#include "sysregs.h"
#include "utilities.h"

//
// Device events
//
EVT_WDF_DEVICE_SELF_MANAGED_IO_INIT WindowsPerfEvtDeviceSelfManagedIoStart;
EVT_WDF_DEVICE_SELF_MANAGED_IO_SUSPEND WindowsPerfEvtDeviceSelfManagedIoSuspend;


#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, WindowsPerfDeviceCreate)
#pragma alloc_text (PAGE, WindowsPerfEvtDeviceSelfManagedIoSuspend)
#endif

//
// Port Begin
//


struct dmcs_desc dmc_array = { 0 };

UINT8 dsu_numGPC = 0;
UINT16 dsu_numCluster = 0;
UINT16 dsu_sizeCluster = 0;

// bit N set if evt N is supported, not used at the moment, but should.
UINT32 dsu_evt_mask_lo = 0;
UINT32 dsu_evt_mask_hi = 0;
LONG volatile cpunos = 0;
ULONG numCores = 0;
UINT8 numGPC = 0;
UINT8 numFreeGPC = 0;
UINT64 dfr0_value = 0;
UINT64 midr_value = 0;
UINT64 id_aa64dfr0_el1_value = 0;
HANDLE pmc_resource_handle = NULL;
UINT32 counter_idx_map[AARCH64_MAX_HWC_SUPP + 1];
CoreInfo* core_info = NULL;
// Use this array to calculate the value for fixed counters via a delta approach as we are no longer resetting it.
// See comment on CoreCounterReset() for an explanation.
UINT64* last_fpc_read = NULL;
extern KEVENT sync_reset_dpc;

enum
{
#define WPERF_ARMV8_ARCH_EVENTS(n,a,b,c,d) PMU_EVENT_##a = b,
#include "wperf-common\armv8-arch-events.def"
#undef WPERF_ARMV8_ARCH_EVENTS
};


//
//    Data list
//
KSPIN_LOCK  B2BLock;
B2BData         b2b_data[MAX_PMU_CTL_CORES_COUNT];

//////////////////////////////////////////////////////////////////
//
//    I S R  functions
//
//

#define PMOVSCLR_VALID_BITS_MASK 0xffffffffULL
static UINT64 arm64_clear_ov_flags(void)
{
    UINT64 pmov_value = _ReadStatusReg(PMOVSCLR_EL0);
    pmov_value &= PMOVSCLR_VALID_BITS_MASK;
    _WriteStatusReg(PMOVSCLR_EL0, (__int64)pmov_value);
    __isb(_ARM64_BARRIER_SY);
    return pmov_value;
}

typedef VOID (*PMIHANDLER)(PKTRAP_FRAME TrapFrame);

VOID arm64_pmi_ISR(PKTRAP_FRAME pTrapFrame)
{
    ULONG core_idx = KeGetCurrentProcessorNumberEx(NULL);
    CoreInfo* core = core_info + core_idx;
    UINT64 ov_flags = arm64_clear_ov_flags();
    ov_flags &= core->ov_mask;

    if (!ov_flags)
        return;

    core->sample_generated++;

    if (!KeTryToAcquireSpinLockAtDpcLevel(&core->SampleLock))
    {
        core->sample_dropped++;
        return;
    }

    if (core->sample_idx == SAMPLE_CHAIN_BUFFER_SIZE)
    {

        KeReleaseSpinLockFromDpcLevel(&core->SampleLock);
        core->sample_dropped++;
        return;
    }
    else
    {
        CoreCounterStop();

        core->samples[core->sample_idx].lr = pTrapFrame->Lr;
        core->samples[core->sample_idx].pc = pTrapFrame->Pc;
        core->samples[core->sample_idx].ov_flags = ov_flags;
        core->sample_idx++;

        KeReleaseSpinLockFromDpcLevel(&core->SampleLock);

        for (int i = 0; i < 32; i++)
        {
            if (!(ov_flags & (1ULL << i)))
                continue;

            UINT32 val = 0xFFFFFFFF - core->sample_interval[i];

            if (i == 31)
                _WriteStatusReg(PMCCNTR_EL0, (__int64)val);
            else
                core_write_counter(i, (__int64)val);
        }
        CoreCounterStart();
    }
}





////////////////////////////////////////////////////////////////////////////////////////
//
//
//   D E V I C E   Related functions
//
//


// Default events that will be assigned to counters when driver loaded.
struct pmu_event_kernel default_events[AARCH64_MAX_HWC_SUPP + numFPC] =
{
    {CYCLE_EVENT_IDX,                   FILTER_EXCL_EL1, CYCLE_COUNTER_IDX, 0},
    {PMU_EVENT_INST_RETIRED,            FILTER_EXCL_EL1, 0,                 0},
    {PMU_EVENT_STALL_FRONTEND,          FILTER_EXCL_EL1, 1,                 0},
    {PMU_EVENT_STALL_BACKEND,           FILTER_EXCL_EL1, 2,                 0},
    {PMU_EVENT_L1I_CACHE_REFILL,        FILTER_EXCL_EL1, 3,                 0},
    {PMU_EVENT_L1I_CACHE,               FILTER_EXCL_EL1, 4,                 0},
    {PMU_EVENT_L1D_CACHE_REFILL,        FILTER_EXCL_EL1, 5,                 0},
    {PMU_EVENT_L1D_CACHE,               FILTER_EXCL_EL1, 6,                 0},
    {PMU_EVENT_BR_RETIRED,              FILTER_EXCL_EL1, 7,                 0},
    {PMU_EVENT_BR_MIS_PRED_RETIRED,     FILTER_EXCL_EL1, 8,                 0},
    {PMU_EVENT_INST_SPEC,               FILTER_EXCL_EL1, 9,                 0},
    {PMU_EVENT_ASE_SPEC,                FILTER_EXCL_EL1, 10,                0},
    {PMU_EVENT_VFP_SPEC,                FILTER_EXCL_EL1, 11,                0},
    {PMU_EVENT_BUS_ACCESS,              FILTER_EXCL_EL1, 12,                0},
    {PMU_EVENT_BUS_CYCLES,              FILTER_EXCL_EL1, 13,                0},
    {PMU_EVENT_LDST_SPEC,               FILTER_EXCL_EL1, 14,                0},
    {PMU_EVENT_DP_SPEC,                 FILTER_EXCL_EL1, 15,                0},
    {PMU_EVENT_CRYPTO_SPEC,             FILTER_EXCL_EL1, 16,                0},
    {PMU_EVENT_STREX_FAIL_SPEC,         FILTER_EXCL_EL1, 17,                0},
    {PMU_EVENT_BR_IMMED_SPEC,           FILTER_EXCL_EL1, 18,                0},
    {PMU_EVENT_BR_RETURN_SPEC,          FILTER_EXCL_EL1, 19,                0},
    {PMU_EVENT_BR_INDIRECT_SPEC,        FILTER_EXCL_EL1, 20,                0},
    {PMU_EVENT_L2I_CACHE,               FILTER_EXCL_EL1, 21,                0},
    {PMU_EVENT_L2I_CACHE_REFILL,        FILTER_EXCL_EL1, 22,                0},
    {PMU_EVENT_L2D_CACHE,               FILTER_EXCL_EL1, 23,                0},
    {PMU_EVENT_L2D_CACHE_REFILL,        FILTER_EXCL_EL1, 24,                0},
    {PMU_EVENT_L1I_TLB,                 FILTER_EXCL_EL1, 25,                0},
    {PMU_EVENT_L1I_TLB_REFILL,          FILTER_EXCL_EL1, 26,                0},
    {PMU_EVENT_L1D_TLB,                 FILTER_EXCL_EL1, 27,                0},
    {PMU_EVENT_L1D_TLB_REFILL,          FILTER_EXCL_EL1, 28,                0},
    {PMU_EVENT_L2I_TLB,                 FILTER_EXCL_EL1, 29,                0},
    {PMU_EVENT_L2I_TLB_REFILL,          FILTER_EXCL_EL1, 30,                0},
};



VOID free_pmu_resource(VOID)
{
    if (pmc_resource_handle == NULL)
        return;

    NTSTATUS status = HalFreeHardwareCounters(pmc_resource_handle);

    pmc_resource_handle = NULL;

    if (status != STATUS_SUCCESS)
    {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HalFreeHardwareCounters: failed 0x%x\n", status));
    }
    else
    {
        KdPrintEx((DPFLTR_IHVDRIVER_ID,  DPFLTR_INFO_LEVEL, "HalFreeHardwareCounters: success\n"));
    }
}













VOID WindowsPerfDeviceUnload()
{
    free_pmu_resource();

    for (ULONG i = 0; i < numCores; i++)
    {
        if (core_info[i].b2b_timer_running)
            KeCancelTimer(&core_info[i].b2b_timer);
        core_info[i].b2b_timer_running = FALSE;

        if (core_info[i].timer_running)
            KeCancelTimer(&core_info[i].timer);
        core_info[i].timer_running = FALSE;
    }

    if (core_info)
        ExFreePoolWithTag(core_info, 'CORE');

    if (last_fpc_read)
        ExFreePoolWithTag(last_fpc_read, 'LAST');

    if (dmc_array.dmcs)
    {
        for (UINT8 i = 0; i < dmc_array.dmc_num; i++)
            MmUnmapIoSpace((PVOID)dmc_array.dmcs[i].iomem_start, dmc_array.dmcs[i].iomem_len);

        ExFreePoolWithTag(dmc_array.dmcs, 'DMCR');
        dmc_array.dmcs = NULL;
    }

    // Uninstall PMI isr
    PMIHANDLER isr = NULL;
    if (HalSetSystemInformation(HalProfileSourceInterruptHandler, sizeof(PMIHANDLER), (PVOID)&isr) != STATUS_SUCCESS)
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "uninstalling sampling isr failed \n"));
}


//
// Port End
//

/// <summary>
/// Worker routine called to create a device and its software resources.
/// </summary>
/// <param name="DeviceInit">Pointer to an opaque init structure. Memory
/// for this structure will be freed by the framework when the
/// WdfDeviceCreate succeeds. So don't access the structure after that
/// point.</param>
/// <returns>NTSTATUS</returns>
NTSTATUS
WindowsPerfDeviceCreate(
    PWDFDEVICE_INIT DeviceInit
)
{
    WDF_OBJECT_ATTRIBUTES   deviceAttributes;
    PDEVICE_CONTEXT deviceContext;
    WDF_PNPPOWER_EVENT_CALLBACKS    pnpPowerCallbacks;
    WDFDEVICE device;
    NTSTATUS status;

    PAGED_CODE();

    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);

    //
    // Register pnp/power callbacks so that we can start and stop the timer as the device
    // gets started and stopped.
    //
    pnpPowerCallbacks.EvtDeviceSelfManagedIoInit = WindowsPerfEvtDeviceSelfManagedIoStart;
    pnpPowerCallbacks.EvtDeviceSelfManagedIoSuspend = WindowsPerfEvtDeviceSelfManagedIoSuspend;

    //
    // Function used for both Init and Restart Callbacks
    //
#pragma warning(suppress: 28024)
    pnpPowerCallbacks.EvtDeviceSelfManagedIoRestart = WindowsPerfEvtDeviceSelfManagedIoStart;

    //
    // Set exclusive to TRUE so that no more than one app can talk to the
    // control device at any time.
    //
    WdfDeviceInitSetExclusive(DeviceInit, TRUE);

    WdfDeviceInitSetPowerPageable(DeviceInit);

    //
    // Register the PnP and power callbacks. Power policy related callbacks will be registered
    // later in SotwareInit.
    //
    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpPowerCallbacks);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceAttributes, DEVICE_CONTEXT);

    status = WdfDeviceCreate(&DeviceInit, &deviceAttributes, &device);

    if (NT_SUCCESS(status)) {
        //
        // Get the device context and initialize it. WdfObjectGet_DEVICE_CONTEXT is an
        // inline function generated by WDF_DECLARE_CONTEXT_TYPE macro in the
        // device.h header file. This function will do the type checking and return
        // the device context. If you pass a wrong object  handle
        // it will return NULL and assert if run under framework verifier mode.
        //
        deviceContext = WdfObjectGet_DEVICE_CONTEXT(device);
        deviceContext->PrivateDeviceData = 0;

        //
        // Create a device interface so that application can find and talk
        // to us.
        //
        status = WdfDeviceCreateDeviceInterface(
            device,
            &GUID_DEVINTERFACE_WINDOWSPERF,
            NULL // ReferenceString
        );

        if (NT_SUCCESS(status)) {
            //
            // Initialize the I/O Package and any Queues
            //
            status = WindowsPerfQueueInitialize(device);
        }
    }

    //
    // Port Begin
    //

    dfr0_value = _ReadStatusReg(ID_DFR0_EL1);
    int pmu_ver = (dfr0_value >> 8) & 0xf;

    if (pmu_ver == 0x0)
    {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "PMUv3 not supported by hardware\n"));
        return STATUS_FAIL_CHECK;
    }

    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "PMU version %d\n", pmu_ver));

    midr_value = _ReadStatusReg(MIDR_EL1);
    UINT8 implementer = (midr_value >> 24) & 0xff;
    UINT8 variant = (midr_value >> 20) & 0xf;
    UINT8 arch_num = (midr_value >> 16) & 0xf;
    UINT16 part_num = (midr_value >> 4) & 0xfff;
    UINT8 revision = midr_value & 0xf;
    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "arch: %d, implementer %d, variant: %d, part_num: %d, revision: %d\n",
        arch_num, implementer, variant, part_num, revision));

    if (pmu_ver == 0x6 || pmu_ver == 0x7)
        CpuHasLongEventSupportSet(1);

    // Arm Statistical Profiling Extensions (SPE) detection
    id_aa64dfr0_el1_value = _ReadStatusReg(ID_AA64DFR0_EL1);
    UINT8 aa64_pms_ver = ID_AA64DFR0_EL1_PMSVer(id_aa64dfr0_el1_value);
    UINT8 aa64_pmu_ver = ID_AA64DFR0_EL1_PMUVer(id_aa64dfr0_el1_value);
    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "AArch64 Debug Feature Register 0: 0x%llX\n", id_aa64dfr0_el1_value));
    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "pmu_ver: 0x%x, pms_ver: 0x%u\n", aa64_pmu_ver, aa64_pms_ver));

    {   // Print SPE feature version
        char* spe_str = "unknown SPE configuration!";
        switch (aa64_pms_ver)
        {
        case 0b000: spe_str = "not implemented."; break;
        case 0b001: spe_str = "FEAT_SPE"; break;
        case 0b010: spe_str = "FEAT_SPEv1p1"; break;
        case 0b011: spe_str = "FEAT_SPEv1p2"; break;
        case 0b100: spe_str = "FEAT_SPEv1p3"; break;
        }
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Statistical Profiling Extension: %s\n", spe_str));
    }

    {   // Print PMU feature version
        char* pmu_str = "unknown PMU configuration!";
        switch (aa64_pmu_ver)
        {
        case 0b0000: pmu_str = "not implemented."; break;
        case 0b0001: pmu_str = "FEAT_PMUv3"; break;
        case 0b0100: pmu_str = "FEAT_PMUv3p1"; break;
        case 0b0101: pmu_str = "FEAT_PMUv3p4"; break;
        case 0b0110: pmu_str = "FEAT_PMUv3p5"; break;
        case 0b0111: pmu_str = "FEAT_PMUv3p7"; break;
        case 0b1000: pmu_str = "FEAT_PMUv3p8"; break;
        }
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Performance Monitors Extension: %s\n", pmu_str));
    }

    UINT32 pmcr = CorePmcrGet();
    numGPC = (pmcr >> ARMV8_PMCR_N_SHIFT) & ARMV8_PMCR_N_MASK;
    KdPrintEx((DPFLTR_IHVDRIVER_ID,  DPFLTR_INFO_LEVEL, "%d general purpose hardware counters detected\n", numGPC));

    numCores = KeQueryActiveProcessorCountEx(ALL_PROCESSOR_GROUPS);
    KdPrintEx((DPFLTR_IHVDRIVER_ID,  DPFLTR_INFO_LEVEL, "%d cores detected\n", numCores));

    // Finally, alloc PMU counters
    // 1) Query for free PMU counters
    const size_t counter_idx_map_size = sizeof(UINT32) * (AARCH64_MAX_HWC_SUPP + 1);
    RtlSecureZeroMemory(counter_idx_map, counter_idx_map_size);

    PHYSICAL_COUNTER_RESOURCE_LIST TmpCounterResourceList = { 0 };
    TmpCounterResourceList.Count = 1;
    TmpCounterResourceList.Descriptors[0].Type = ResourceTypeSingle;
    UINT8 numFreeCounters = 0;
    for (UINT32 i = 0; i < numGPC; i++)
    {
        TmpCounterResourceList.Descriptors[0].u.CounterIndex = i;
        status = HalAllocateHardwareCounters(NULL, 0, &TmpCounterResourceList, &pmc_resource_handle);
        if (status == STATUS_SUCCESS)
        {
            counter_idx_map[numFreeCounters] = i;
            numFreeCounters++;
            HalFreeHardwareCounters(pmc_resource_handle);
        }
    }
    if (numFreeCounters == 0)
    {
        KdPrintEx((DPFLTR_IHVDRIVER_ID,  DPFLTR_INFO_LEVEL, "HAL: counters allocated by other kernel modules\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    KdPrintEx((DPFLTR_IHVDRIVER_ID,  DPFLTR_INFO_LEVEL, "%d free general purpose hardware counters detected\n", numFreeCounters));

    counter_idx_map[CYCLE_COUNTER_IDX] = CYCLE_COUNTER_IDX;

#ifdef _DEBUG
    for (UINT8 i = 0; i < numGPC; i++)
    {
        i %= AARCH64_MAX_HWC_SUPP;
        KdPrintEx((DPFLTR_IHVDRIVER_ID,  DPFLTR_INFO_LEVEL, "counter_idx_map[%u] => %u\n", i, counter_idx_map[i]));
    }
#endif

    // 2) Alloc PMU counters that are free
    size_t AllocationSize = FIELD_OFFSET(PHYSICAL_COUNTER_RESOURCE_LIST, Descriptors[numFreeCounters]);
    PPHYSICAL_COUNTER_RESOURCE_LIST CounterResourceList = (PPHYSICAL_COUNTER_RESOURCE_LIST)ExAllocatePool2(POOL_FLAG_NON_PAGED, AllocationSize, 'CRCL');
    if (CounterResourceList == NULL)
    {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "ExAllocatePoolWithTag: failed \n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlSecureZeroMemory(CounterResourceList, AllocationSize);
    CounterResourceList->Count = numFreeCounters;
    for (UINT32 i = 0; i < numFreeCounters; i++)
    {
        CounterResourceList->Descriptors[i].u.CounterIndex = counter_idx_map[i];
        CounterResourceList->Descriptors[i].Type = ResourceTypeSingle;
    }

    status = HalAllocateHardwareCounters(NULL, 0, CounterResourceList, &pmc_resource_handle);
    ExFreePoolWithTag(CounterResourceList, 'CRCL');
    if (status == STATUS_INSUFFICIENT_RESOURCES)
    {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HAL: counters allocated by other kernel modules\n"));
        return status;
    }

    if (status != STATUS_SUCCESS)
    {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HAL: allocate failed 0x%x\n", status));
        return status;
    }
    numFreeGPC = numFreeCounters;

    // This driver expose private APIs (IOCTL commands), but also enable ThreadProfiling APIs.
    HARDWARE_COUNTER counter_descs[AARCH64_MAX_HWC_SUPP] = {0};
    RtlSecureZeroMemory(&counter_descs, sizeof(counter_descs));
    for (int i = 0; i < numFreeGPC; i++)
    {
        counter_descs[i].Type = PMCCounter;
        counter_descs[i].Index = counter_idx_map[i];

        status = KeSetHardwareCounterConfiguration(&counter_descs[i], 1);
        if (status == STATUS_WMI_ALREADY_ENABLED)
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "KeSetHardwareCounterConfiguration: counter %d already enabled for ThreadProfiling\n", i));
        }
        else if (status != STATUS_SUCCESS)
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "KeSetHardwareCounterConfiguration: counter %d failed 0x%x\n", i, status));
            return status;
        }
    }

    core_info = (CoreInfo*)ExAllocatePool2(POOL_FLAG_NON_PAGED, sizeof(CoreInfo) * numCores, 'CORE');
    if (core_info == NULL)
    {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "ExAllocatePoolWithTag: failed \n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlSecureZeroMemory(core_info, sizeof(CoreInfo) * numCores);

    last_fpc_read = (UINT64*)ExAllocatePool2(POOL_FLAG_NON_PAGED, sizeof(UINT64) * numCores, 'LAST');
    if (!last_fpc_read)
    {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "%s:%d - ExAllocatePool2: failed\n", __FUNCTION__, __LINE__));
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlSecureZeroMemory(last_fpc_read, sizeof(UINT64)* numCores);

    for (ULONG i = 0; i < numCores; i++)
    {
        CoreInfo* core = &core_info[i];        
        core->idx = i;

        PROCESSOR_NUMBER ProcNumber;
        status = KeGetProcessorNumberFromIndex(i, &ProcNumber);
        if (status != STATUS_SUCCESS)
            return status;

        core->events_num = numFPC + numFreeGPC;
        for (UINT32 k = 0; k < core->events_num; k++)
            RtlCopyMemory(core->events + k, default_events + k, sizeof(struct pmu_event_kernel));

        // Initialize fields for sampling;
        KeInitializeSpinLock(&core->SampleLock);

        // Enable  events and counters
        PRKDPC dpc = &core_info[i].dpc_queue;
        KeInitializeDpc(dpc, arm64pmc_enable_default, NULL);
        status = KeSetTargetProcessorDpcEx(dpc, &ProcNumber);
        if (status != STATUS_SUCCESS)
            return status;
        KeSetImportanceDpc(dpc, HighImportance);
        KeInsertQueueDpc(dpc, NULL, NULL);

        //Initialize DPCs for counting
        PRKDPC dpc_overflow = &core_info[i].dpc_overflow;
        PRKDPC dpc_multiplex = &core_info[i].dpc_multiplex;
        PRKDPC dpc_reset = &core_info[i].dpc_reset;
        PRKDPC dpc_b2b = &core_info[i].dpc_b2b;

        KeInitializeDpc(dpc_overflow, overflow_dpc, &core_info[i]);
        KeInitializeDpc(dpc_multiplex, multiplex_dpc, &core_info[i]);
        KeInitializeDpc(dpc_reset, reset_dpc, &core_info[i]);
        KeInitializeDpc(dpc_b2b, b2b_dpc, &core_info[i]);

        KeSetTargetProcessorDpcEx(dpc_overflow, &ProcNumber);
        KeSetTargetProcessorDpcEx(dpc_multiplex, &ProcNumber);
        KeSetTargetProcessorDpcEx(dpc_reset, &ProcNumber);
        KeSetTargetProcessorDpcEx(dpc_b2b, &ProcNumber);

        KeSetImportanceDpc(dpc_overflow, HighImportance);
        KeSetImportanceDpc(dpc_multiplex, HighImportance);
        KeSetImportanceDpc(dpc_reset, HighImportance);
        KeSetImportanceDpc(dpc_b2b, HighImportance);
    }

    KeInitializeEvent(&sync_reset_dpc, NotificationEvent, FALSE);

    PMIHANDLER isr = arm64_pmi_ISR;
    status = HalSetSystemInformation(HalProfileSourceInterruptHandler, sizeof(PMIHANDLER), (PVOID)&isr);
    if (status != STATUS_SUCCESS)
    {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "register sampling isr failed \n"));
        return status;
    }

    KdPrintEx((DPFLTR_IHVDRIVER_ID,  DPFLTR_INFO_LEVEL, "loaded\n"));
    //
    // Port End
    //


    // initialise locked data for storing b2b time line data.  It is read by the driver periodically, 
    // and then the app reads it off the driver, so it needs buffering
    KeInitializeSpinLock(&B2BLock);

    //  and finally do a reset on the hardware to make sure it is in a known state.  The reset dpc sets the event, so we have 
    // to call this after the event is initialised of couse
    for (ULONG i = 0; i < numCores; i++)
    {
        CoreInfo* core = &core_info[i];
        core->timer_round = 0;


        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Calling calling reset dpc in loop, i is %d core index is %lld\n", i, core->idx));
        KeInsertQueueDpc(&core->dpc_reset, (VOID*)numCores, NULL);
    }

    return status;
}

/// <summary>
/// This event is called by the Framework when the device is started
/// or restarted after a suspend operation.
///
/// This function is not marked pageable because this function is in the
/// device power up path.When a function is marked pagable and the code
/// section is paged out, it will generate a page fault which could impact
/// the fast resume behavior because the client driver will have to wait
/// until the system drivers can service this page fau
/// </summary>
/// <param name="Device">Handle to a framework device object.</param>
/// <returns>NTSTATUS - Failures will result in the device stack being torn down.</returns>
NTSTATUS
WindowsPerfEvtDeviceSelfManagedIoStart(
    IN  WDFDEVICE Device
    )
{
    KdPrintEx((DPFLTR_IHVDRIVER_ID,  DPFLTR_INFO_LEVEL, "--> WindowsPerfEvtDeviceSelfManagedIoInit\n"));

    //
    // Restart the queue and the periodic timer. We stopped them before going
    // into low power state.
    //
    WdfIoQueueStart(WdfDeviceGetDefaultQueue(Device));

    KdPrintEx((DPFLTR_IHVDRIVER_ID,  DPFLTR_INFO_LEVEL,  "<-- WindowsPerfEvtDeviceSelfManagedIoInit\n"));

    return STATUS_SUCCESS;
}

/// <summary>
/// This event is called by the Framework when the device is stopped
/// for resource rebalance or suspended when the system is entering
///  Sx state.
/// </summary>
/// <param name="Device">Handle to a framework device object.</param>
/// <returns>NTSTATUS - The driver is not allowed to fail this function.  If it does, the
///  device stack will be torn down.</returns>
NTSTATUS
WindowsPerfEvtDeviceSelfManagedIoSuspend(
    IN  WDFDEVICE Device
    )
{
    PAGED_CODE();

    KdPrintEx((DPFLTR_IHVDRIVER_ID,  DPFLTR_INFO_LEVEL, "--> WindowsPerfEvtDeviceSelfManagedIoSuspend\n"));

    //
    // Before we stop the timer we should make sure there are no outstanding
    // i/o. We need to do that because framework cannot suspend the device
    // if there are requests owned by the driver. There are two ways to solve
    // this issue: 1) We can wait for the outstanding I/O to be complete by the
    // periodic timer 2) Register EvtIoStop callback on the queue and acknowledge
    // the request to inform the framework that it's okay to suspend the device
    // with outstanding I/O. In this sample we will use the 1st approach
    // because it's pretty easy to do. We will restart the queue when the
    // device is restarted.
    //
    WdfIoQueueStopSynchronously(WdfDeviceGetDefaultQueue(Device));

    KdPrintEx((DPFLTR_IHVDRIVER_ID,  DPFLTR_INFO_LEVEL,  "<-- WindowsPerfEvtDeviceSelfManagedIoSuspend\n"));

    return STATUS_SUCCESS;
}
