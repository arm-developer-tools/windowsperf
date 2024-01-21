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


#include "driver.h"
#include "device.h"
#if defined ENABLE_TRACING
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
extern LOCK_STATUS   current_status;

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
    case STATUS_POWER_STATE_INVALID:        return "STATUS_POWER_STATE_INVALID";        // 0xC00002D3
    default:                                return "unhandled status value";
	}
}

PCHAR GetIoctlStr(ULONG ioctl)
{
	switch (ioctl)
	{
    // Note: values are used where the Win98 DDK doesn't define a name
    case IOCTL_PMU_CTL_START:               return "IOCTL_PMU_CTL_START";
    case IOCTL_PMU_CTL_STOP:                return "IOCTL_PMU_CTL_STOP";
    case IOCTL_PMU_CTL_RESET:               return "IOCTL_PMU_CTL_RESET";
    case IOCTL_PMU_CTL_QUERY_HW_CFG:        return "IOCTL_PMU_CTL_QUERY_HW_CFG";
    case IOCTL_PMU_CTL_QUERY_SUPP_EVENTS:   return "IOCTL_PMU_CTL_QUERY_SUPP_EVENTS";
    case IOCTL_PMU_CTL_QUERY_VERSION:       return "IOCTL_PMU_CTL_QUERY_VERSION";
    case IOCTL_PMU_CTL_ASSIGN_EVENTS:       return "IOCTL_PMU_CTL_ASSIGN_EVENTS";
    case IOCTL_PMU_CTL_READ_COUNTING:       return "IOCTL_PMU_CTL_READ_COUNTING";
    case IOCTL_DSU_CTL_INIT:                return "IOCTL_DSU_CTL_INIT";
    case IOCTL_DSU_CTL_READ_COUNTING:       return "IOCTL_DSU_CTL_READ_COUNTING";
    case IOCTL_DMC_CTL_INIT:                return "IOCTL_DMC_CTL_INIT";
    case IOCTL_DMC_CTL_READ_COUNTING:       return "IOCTL_DMC_CTL_READ_COUNTING";
    case IOCTL_PMU_CTL_SAMPLE_SET_SRC:      return "IOCTL_PMU_CTL_SAMPLE_SET_SRC";
    case IOCTL_PMU_CTL_SAMPLE_START:        return "IOCTL_PMU_CTL_SAMPLE_START";
    case IOCTL_PMU_CTL_SAMPLE_STOP:         return "IOCTL_PMU_CTL_SAMPLE_STOP";
    case IOCTL_PMU_CTL_SAMPLE_GET:          return "IOCTL_PMU_CTL_SAMPLE_GET";
    case IOCTL_PMU_CTL_LOCK_ACQUIRE:        return "IOCTL_PMU_CTL_LOCK_ACQUIRE";
    case IOCTL_PMU_CTL_LOCK_RELEASE:        return "IOCTL_PMU_CTL_LOCK_RELEASE";
    default:                                return "unknown IOCTL!";
	}
}

VOID SetMeBusyForce(ULONG ioctl, WDFFILEOBJECT  file_object) // always suceeds
{
    KIRQL oldirql;

    KeAcquireSpinLock(&current_status.sts_lock, &oldirql);
    current_status.status = STS_BUSY;
    current_status.ioctl = ioctl;
    current_status.file_object = file_object;
    KeReleaseSpinLock(&current_status.sts_lock, oldirql);
}

BOOLEAN SetMeBusy(ULONG ioctl, WDFFILEOBJECT  file_object) // returns failure if the lock is held by another process
{
    BOOLEAN ret = FALSE;
    KIRQL oldirql;

    KeAcquireSpinLock(&current_status.sts_lock, &oldirql);
    if( (current_status.file_object != file_object)  // if we dont hold the lock, and the lock is busy
        &&
        (current_status.status == STS_BUSY) )
    {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "SetMeBusy : %s : device is busy with file object %p with ioctl %s active\n",
            GetIoctlStr(ioctl),
            current_status.file_object,
            GetIoctlStr(current_status.ioctl)));
        ret = FALSE;
    }
    else // else we hold the lock or the lock isnt held, so we get the lock and succeed
    {
        current_status.status = STS_BUSY;
        current_status.ioctl = ioctl;
        current_status.file_object = file_object;
        ret = TRUE;
    }
    KeReleaseSpinLock(&current_status.sts_lock, oldirql);
    return ret;
}

BOOLEAN AmILocking(ULONG ioctl, WDFFILEOBJECT  file_object)// returns TRUE if file objects match
{
    BOOLEAN  ret = FALSE;
    KIRQL oldirql;

#if!defined DBG
    UNREFERENCED_PARAMETER(ioctl);
#endif

    KeAcquireSpinLock(&current_status.sts_lock, &oldirql);
    if (file_object == current_status.file_object) // if we hold the lock, return TRUE, and also update the active ioctl
    {
        ret = TRUE;  
        current_status.ioctl = ioctl;
    }
    else
    {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "AmILocking : %s : device is busy with file object %p with ioctl %s active\n", 
                    GetIoctlStr(ioctl), 
                    current_status.file_object, 
                    GetIoctlStr(current_status.ioctl)));
    }
    KeReleaseSpinLock(&current_status.sts_lock, oldirql);
    return ret;
}

BOOLEAN SetMeIdle(WDFFILEOBJECT  file_object)// returns failure if the lock is not held by this process
{
    KIRQL oldirql;
    BOOLEAN ret = FALSE;

    KeAcquireSpinLock(&current_status.sts_lock, &oldirql);
    if (file_object == current_status.file_object)
    {
        current_status.status = STS_IDLE;
        current_status.ioctl = 0;
        current_status.file_object = 0;
        ret = TRUE;
    }
    else
    {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "SetMeIdle : lock is held by process %p,  not this one, %p\n",
            current_status.file_object,
            file_object));
        ret = FALSE;
    }
    KeReleaseSpinLock(&current_status.sts_lock, oldirql);
    return ret;
}
