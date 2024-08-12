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

#include <DriverSpecs.h>
_Analysis_mode_(_Analysis_code_type_user_code_)

#define INITGUID

#include <windows.h>
#include <stdlib.h>
#include <strsafe.h>
#include <psapi.h>
#include <cfgmgr32.h>
#include <devpkey.h>

#include "wperf.h"
#include "output.h"
#include "wperf-common\public.h"

#pragma comment (lib, "cfgmgr32.lib")
#pragma comment (lib, "User32.lib")
#pragma comment (lib, "ole32.lib")
#pragma comment (lib, "oleaut32.lib")


BOOL
GetDevicePath(
    _In_ LPGUID InterfaceGuid,
    _Out_writes_(BufLen) PWCHAR DevicePath,
    _In_ size_t BufLen
    )
{
    CONFIGRET cr = CR_SUCCESS;
    PWSTR deviceInterfaceList = NULL;
    ULONG deviceInterfaceListLength = 0;
    PWSTR nextInterface;
    HRESULT hr = E_FAIL;
    BOOL bRet = TRUE;

    cr = CM_Get_Device_Interface_List_Size(
                &deviceInterfaceListLength,
                InterfaceGuid,
                NULL,
                CM_GET_DEVICE_INTERFACE_LIST_PRESENT);
    if (cr != CR_SUCCESS) {
        m_out.GetErrorOutputStream() << L"error: " << std::hex << cr << L" retrieving device interface list size." << std::endl;
        goto clean0;
    }

    if (deviceInterfaceListLength <= 1) {
        bRet = FALSE;
        m_out.GetErrorOutputStream() << L"error: No active device interfaces found." << std::endl
            << L"Is the driver loaded? You can get the latest driver from https://gitlab.com/Linaro/WindowsPerf/windowsperf/-/releases" << std::endl;
        goto clean0;
    }

    deviceInterfaceList = (PWSTR)malloc(deviceInterfaceListLength * sizeof(WCHAR));
    if (deviceInterfaceList == NULL) {
        m_out.GetErrorOutputStream() << L"error: Allocating memory for device interface list." << std::endl;
        goto clean0;
    }
    ZeroMemory(deviceInterfaceList, deviceInterfaceListLength * sizeof(WCHAR));

    cr = CM_Get_Device_Interface_List(
                InterfaceGuid,
                NULL,
                deviceInterfaceList,
                deviceInterfaceListLength,
                CM_GET_DEVICE_INTERFACE_LIST_PRESENT);
    if (cr != CR_SUCCESS) {
        m_out.GetErrorOutputStream() << L"error: " << std::hex << cr << L" retrieving device interface list." << std::endl;
        goto clean0;
    }

    nextInterface = deviceInterfaceList + wcslen(deviceInterfaceList) + 1;
    if (*nextInterface != UNICODE_NULL) {
        m_out.GetErrorOutputStream() << L"warning: More than one device interface instance found." << std::endl
            << "Selecting first matching device." << std::endl;
    }

    hr = StringCchCopy(DevicePath, BufLen, deviceInterfaceList);
    if (FAILED(hr)) {
        bRet = FALSE;
        m_out.GetErrorOutputStream() << L"error: StringCchCopy failed with HRESULT=" << std::hex << hr << std::endl;
        goto clean0;
    }

clean0:
    if (deviceInterfaceList != NULL) {
        free(deviceInterfaceList);
    }
    if (CR_SUCCESS != cr) {
        bRet = FALSE;
    }

    return bRet;
}

double timestamps_to_duration(const SYSTEMTIME& timestamp_a, const SYSTEMTIME& timestamp_b) 
{
    ULARGE_INTEGER li_a, li_b;
    FILETIME time_a, time_b;

    SystemTimeToFileTime(&timestamp_a, &time_a);
    SystemTimeToFileTime(&timestamp_b, &time_b);

    li_a.u.LowPart = time_a.dwLowDateTime;
    li_a.u.HighPart = time_a.dwHighDateTime;
    li_b.u.LowPart = time_b.dwLowDateTime;
    li_b.u.HighPart = time_b.dwHighDateTime;

    double duration = (double)(li_b.QuadPart - li_a.QuadPart) / 10000000.0;

    return duration;
}