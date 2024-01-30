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
#include <poclass.h>


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


PCHAR DbgStatusStr(NTSTATUS status)
{
    switch (status)
    {
        // Note: values are used where the Win98 DDK doesn't define a name

    case STATUS_SUCCESS:                    return "STATUS_SUCCESS";                    // 0x00000000
    case STATUS_TIMEOUT:                    return "STATUS_TIMEOUT";                    // 0x00000102
    case STATUS_PENDING:                    return "STATUS_PENDING";                    // 0x00000103
    case STATUS_UNSUCCESSFUL:               return "STATUS_UNSUCCESSFUL";               // 0xC0000001
    case STATUS_NOT_IMPLEMENTED:            return "STATUS_NOT_IMPLEMENTED";            // 0xC0000002
    case STATUS_INVALID_PARAMETER:          return "STATUS_INVALID_PARAMETER";          // 0xC000000D
    case STATUS_NO_SUCH_DEVICE:             return "STATUS_NO_SUCH_DEVICE";             // 0xC000000E
    case STATUS_INVALID_DEVICE_REQUEST:     return "STATUS_INVALID_DEVICE_REQUEST";     // 0xC0000010
    case STATUS_MORE_PROCESSING_REQUIRED:   return "STATUS_MORE_PROCESSING_REQUIRED";   // 0xC0000016
    case STATUS_ACCESS_DENIED:              return "STATUS_ACCESS_DENIED";              // 0xC0000022
    case STATUS_BUFFER_TOO_SMALL:           return "STATUS_BUFFER_TOO_SMALL";           // 0xC0000023
    case STATUS_OBJECT_NAME_NOT_FOUND:      return "STATUS_OBJECT_NAME_NOT_FOUND";      // 0xC0000034
    case STATUS_OBJECT_NAME_COLLISION:      return "STATUS_OBJECT_NAME_COLLISION";      // 0xC0000035
    case STATUS_NOT_SUPPORTED:              return "STATUS_NOT_SUPPORTED";              // 0xC00000BB
    case STATUS_DELETE_PENDING:             return "STATUS_DELETE_PENDING";             // 0xC0000056
    case STATUS_INSUFFICIENT_RESOURCES:     return "STATUS_INSUFFICIENT_RESOURCES";     // 0xC000009A
    case STATUS_DEVICE_DATA_ERROR:          return "STATUS_DEVICE_DATA_ERROR";          // 0xC000009C
    case STATUS_DEVICE_NOT_CONNECTED:       return "STATUS_DEVICE_NOT_CONNECTED";       // 0xC000009D
    case STATUS_DEVICE_NOT_READY:           return "STATUS_DEVICE_NOT_READY";           // 0xC00000A3
    case STATUS_CANCELLED:                  return "STATUS_CANCELLED";                  // 0xC0000120
    case 0xC00002D3:                        return "STATUS_POWER_STATE_INVALID";        // 0xC00002D3
    default:                                return "???";
    }
}


VOID  DbgPrintHexDump(UCHAR* p, SIZE_T len, UCHAR width)
{
    char    s[16 + 1 + 64 * (1 + 2) + 1];   // 210
    char    s2[4];
    char    s3[64 + 1];
    UCHAR      i, n;
    SIZE_T  cur;

    {
        size_t  arraysize = 30;

        CHAR   pszDest[30];

        size_t cbDest = arraysize * sizeof(CHAR);

        LPCSTR   pszFormat = "%s %d + %d = %d.";
        CHAR* pszTxt = "The answer is";

        RtlStringCbPrintfA(pszDest, cbDest, pszFormat, pszTxt, 1, 2, 3);
    }

    // Check width and adjust if necessary
    if (width < 1) width = 1; else
        if (width > 64) width = 64;

    i = 0;
    cur = 0;
    while (len)
    {
        if (i == 0)
        {
            RtlStringCbPrintfA(s, 210, "%c", 0x20);
            s3[0] = 0;
        }

        n = p[cur];

        RtlStringCbPrintfA(s2, 4, " %02X", n);
        RtlStringCbCatA(s, 210, s2);

        if ((n < 32) || (n > 126)) n = '.';
        RtlStringCbPrintfA(s2, 4, "%c", n);
        RtlStringCbCatA(s3, 65, s2);

        i++;
        if (i == width)
        {
            i = 0;
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "%s \n", s));
        }

        cur++;
        len--;
    }

    if (i)
    {
        while (i < width)
        {
            RtlStringCbCatA(s, 210, "   ");
            i++;
        }
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "%s %s\n", s, s3));
    }

    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "\n \n \n"));
}





NTSTATUS
GenericCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
)
{
    PKEVENT Event = (PKEVENT)Context;

    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Irp);

    KeSetEvent(Event, IO_NO_INCREMENT, FALSE);

    return(STATUS_MORE_PROCESSING_REQUIRED);

}

PMEMORY Get620MemoryResource(ULONG linkNum)
{
    UNICODE_STRING	  ObjectName;
    NTSTATUS			      status;
    PFILE_OBJECT		  pFileObject;
    PDEVICE_OBJECT	  pDeviceObject;
    PIRP				          pIRP;
    WCHAR                   linkNameC[256];
    PMEMORY               pMemory = NULL;

    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, " : Get620MemoryResource\n"));

    RtlStringCchPrintfW(linkNameC, 256, L"\\DosDevices\\Linaro620_%d", linkNum);

    RtlInitUnicodeString(&ObjectName, linkNameC);

    status = IoGetDeviceObjectPointer(&ObjectName,
        GENERIC_WRITE | GENERIC_READ,
        &pFileObject,
        &pDeviceObject);

    if (status != STATUS_SUCCESS)
    {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, " : Get620MemoryResource : IoGetDeviceObjectPointer() failed with status %s\n", DbgStatusStr(status)));
    }
    else
    {
        pIRP = IoAllocateIrp(pDeviceObject->StackSize + 2, FALSE);

        if (!pIRP)
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, " : Get620MemoryResource : IoAllocateIrp() failed with status %s\n", DbgStatusStr(status)));
        }
        else
        {
            KEVENT				            event;
            PIO_STACK_LOCATION	pNextStack;

            pMemory = ExAllocatePool2(NonPagedPool, sizeof(MEMORY), 'meMp');

            if (pMemory)
            {
                KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, " : Get620MemoryResource : ExAllocatePool2() failed\n"));
            }
            else
            {
                KeInitializeEvent(&event, NotificationEvent, FALSE);

                KeResetEvent(&event);

                pNextStack = IoGetNextIrpStackLocation(pIRP);

                pNextStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_GET_DEVICE_INFO;

                pNextStack->MajorFunction = IRP_MJ_DEVICE_CONTROL;

                pIRP->AssociatedIrp.SystemBuffer = pMemory;

                pNextStack->Parameters.DeviceIoControl.InputBufferLength = sizeof(MEMORY);

                IoSetCompletionRoutine(pIRP, GenericCompletion, &event, TRUE, TRUE, TRUE);

                status = IoCallDriver(pDeviceObject, pIRP);

                if (status == STATUS_PENDING)
                {
                    KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, 0);
                }

                KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, " : Get620MemoryResource : IoCallDriver() returned %s\n", DbgStatusStr(status)));
            }

            IoFreeIrp(pIRP);
        }

        ObDereferenceObject(pFileObject);
    }
    return pMemory;
}

BOOLEAN check_cores_in_pmu_ctl_hdr_p( struct pmu_ctl_hdr* ctl_req)
{
    if (!ctl_req)
        return FALSE;

    size_t cores_count = ctl_req->cores_idx.cores_count;

    if (cores_count >= MAX_PMU_CTL_CORES_COUNT)
        return FALSE;

    for (auto k = 0; k < cores_count; k++)
        if (ctl_req->cores_idx.cores_no[k] >= MAX_PMU_CTL_CORES_COUNT)
            return FALSE;
    return TRUE;
}

