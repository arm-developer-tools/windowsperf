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
#if defined ENABLE_TRACING
#include "utilities.tmh"
#endif




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
