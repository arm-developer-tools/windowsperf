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
#include "debug.h"
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
    HRESULT hr = E_FAIL;
    BOOL bRet = TRUE;

    cr = CM_Get_Device_Interface_List_Size(
                &deviceInterfaceListLength,
                InterfaceGuid,
                NULL,
                CM_GET_DEVICE_INTERFACE_LIST_PRESENT);
    if (cr != CR_SUCCESS) {
        WindowsPerfDbgPrint("Error: 0x%x retrieving device interface list size.\n", cr);
        goto clean0;
    }

    if (deviceInterfaceListLength <= 1) {
        bRet = FALSE;
        WindowsPerfDbgPrint("Error: No active device interfaces found.\n"
                            "       Is the driver loaded? You can get the latest driver from https://gitlab.com/Linaro/WindowsPerf/windowsperf/-/releases");
        goto clean0;
    }

    deviceInterfaceList = (PWSTR)malloc(deviceInterfaceListLength * sizeof(WCHAR));
    if (deviceInterfaceList == NULL) {
        WindowsPerfDbgPrint("Error: Allocating memory for device interface list.\n");
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
        WindowsPerfDbgPrint("Error: 0x%x retrieving device interface list.\n", cr);
        goto clean0;
    }

    PWSTR deviceInterface;

    for (deviceInterface = deviceInterfaceList; *deviceInterface; deviceInterface += wcslen(deviceInterface) + 1)
    {
        DEVPROPTYPE devicePropertyType;
        ULONG deviceHWIdListLength = 256;
        WCHAR deviceHWIdList[256];
        CONFIGRET cr1 = CR_SUCCESS;

        WindowsPerfDbgPrint("Found device  %S\n\n", deviceInterface);

        ULONG deviceInstanceIdLength = MAX_DEVICE_ID_LEN;
        WCHAR deviceInstanceId[MAX_DEVICE_ID_LEN];

        cr1 = CM_Get_Device_Interface_Property(
            deviceInterface,
            &DEVPKEY_Device_InstanceId,
            &devicePropertyType,
            (PBYTE)deviceInstanceId,
            &deviceInstanceIdLength,
            0);
        if (cr1 != CR_SUCCESS)
        {
            UINT err = GetLastError();
            WindowsPerfDbgPrint("CM_Get_Device_Interface_Property DEVPKEY_Device_InstanceId failed with 0x%X %d\n\n", cr1, err);
            continue;
        }
        WindowsPerfDbgPrint("Found instance %S\n\n", deviceInstanceId);

        DEVINST deviceInstanceHandle;

        cr1 = CM_Locate_DevNode(&deviceInstanceHandle, deviceInstanceId, CM_LOCATE_DEVNODE_NORMAL);
        if (cr1 != CR_SUCCESS)
        {
            UINT err = GetLastError();
            WindowsPerfDbgPrint("CM_Locate_DevNode  failed with 0x%X %d\n\n", cr1, err);
            continue;
        }

        cr1 = CM_Get_DevNode_Property(
            deviceInstanceHandle,
            &DEVPKEY_Device_HardwareIds,
            &devicePropertyType,
            (PBYTE)deviceHWIdList,
            &deviceHWIdListLength,
            0);
        if(cr1  != CR_SUCCESS)
        {
            UINT err = GetLastError();
            WindowsPerfDbgPrint("CM_Get_Device_Interface_Property DEVPKEY_Device_HardwareIds failed with 0x%X %d\n\n", cr1, err);
            continue;
        }

        // hardware IDs is a null sperated list of strings per device.
        PWSTR hwId;
        for (hwId = deviceHWIdList; *hwId; hwId += wcslen(hwId) + 1)
        {
            WindowsPerfDbgPrint("Found HW ID  %S\n\n", hwId);

            if (wcscmp(hwId, L"Root\\WPERFDRIVER") == 0)
            {
                hr = StringCchCopy(DevicePath, BufLen, deviceInterface);
                if (FAILED(hr)) {
                    bRet = FALSE;
                    WindowsPerfDbgPrint("Error: StringCchCopy failed with HRESULT 0x%x", hr);
                }
                goto clean0;
            }
        }

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
