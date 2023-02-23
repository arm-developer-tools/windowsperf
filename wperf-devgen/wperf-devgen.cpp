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

#include <iostream>
#include <conio.h>
#include <windows.h>
#include <swdevice.h>
#include <newdev.h>
#include <sstream>

// Taken from wperf-driver INF file.
PCWSTR instanceId = L"WPERFDRIVER";
PCWSTR hardwareId = L"Root\\WPERFDRIVER";
PCWSTR compatibleIds = L"Root\\WPERFDRIVER";
PCWSTR devDescription = L"WPERFDRIVER Driver";

std::wstring *FullInfPath = nullptr;
const std::wstring InfName(L"wperf-driver.inf");

void GetFullInfPath()
{
    if(FullInfPath == nullptr)
    {

        /* Get full inf file path. */
        DWORD size = GetCurrentDirectory(0, NULL);
        wchar_t* buff = (wchar_t*)malloc(sizeof(wchar_t) * size);
        if (buff)
        {
            GetCurrentDirectory(size, buff);
            std::wstringstream wstream;
            wstream << std::wstring(buff) << L"\\" << InfName;
            FullInfPath = new std::wstring(wstream.str().c_str());
            free(buff);
        }
        else {
            std::cerr << "Error allocating buffer." << std::endl;
            exit(-1);
        }
    }
}

VOID WINAPI CreateDeviceCallback(
    _In_ HSWDEVICE hSwDevice,
    _In_ HRESULT hrCreateResult,
    _In_opt_ PVOID pContext,
    _In_opt_ PCWSTR pszDeviceInstanceId)
{
    if (pContext != nullptr)
    {
        HANDLE hEvent = *(HANDLE*)pContext;

        SetEvent(hEvent);
    }
}

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        std::cerr << "usage: app_name install|uninstall" << std::endl;
        return -1;
    }
    int errorCode = 0;

    GetFullInfPath();

    bool is_install;
    SW_DEVICE_CREATE_INFO swDevInfo = { 0 };
    HSWDEVICE device;

    HANDLE hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

    HRESULT status;
    DWORD waitResult;

    std::string user_command(argv[1]);
    std::cout << "Executing command: " << user_command << "." << std::endl;

    if ("install" == user_command)
    {
        std::cout << "Install requested." << std::endl;
        is_install = true;
    }
    else if ("uninstall" == user_command)
    {
        std::cout << "Uninstall requested." << std::endl;
        is_install = false;
    }
    else {
        std::cerr << "Unrecognized command " << user_command << "." << std::endl;
        errorCode = -1;
        goto clean_exit;
    }


    swDevInfo.cbSize = sizeof(SW_DEVICE_CREATE_INFO);
    swDevInfo.pszInstanceId = instanceId;
    swDevInfo.pszzHardwareIds = hardwareId;
    swDevInfo.pszzCompatibleIds = compatibleIds;
    swDevInfo.pszDeviceDescription = devDescription;
    swDevInfo.CapabilityFlags = SWDeviceCapabilitiesRemovable |
        SWDeviceCapabilitiesSilentInstall |
        SWDeviceCapabilitiesDriverRequired;

    status = SwDeviceCreate(
        L"WPERFDRIVER", //PCWSTR pszEnumeratorName
        L"HTREE\\ROOT\\0", //PCWST pszParentDeviceInstance
        &swDevInfo, //SW_DEVICE_CREATE_INFO *pCreateInfo
        0, //ULONG cPropertyCount
        nullptr, //DEVPROPERTY *pProperties
        CreateDeviceCallback, //SW_DEVICE_CREATE_CALLBACK pCallback
        &hEvent, //PVOID pContext
        &device //PHSWDEVICE
    );

    if (FAILED(status))
    {
        std::cerr << "Error creating device with code - " << std::hex << status << "." << std::endl;
        errorCode = -1;
        goto clean_exit;
    }

    if (hEvent == nullptr)
    {
        std::cerr << "Null context." << std::endl;
        errorCode = -1;
        goto clean_exit;
    }

    std::cout << "Waiting for device creation..." << std::endl;
    waitResult = WaitForSingleObject(hEvent, 10 * 1000);
    if (waitResult != WAIT_OBJECT_0)
    {
        std::cout << "Failed waiting for device creation." << std::endl;
        errorCode = -1;
        goto clean_exit;
    }

    if (is_install)
    {
        // Without setting the device lifetime to SWDeviceLifetimeParentPresent once 
        // SwDeviceClose is called the device is destroyed.
        status = SwDeviceSetLifetime(device, SWDeviceLifetimeParentPresent);

        if (FAILED(status))
        {
            std::cerr << "Error setting device lifetime." << std::endl;
            SwDeviceClose(device);
            errorCode = -1;
            goto clean_exit;
        }

        SwDeviceClose(device);
        std::cout << "Device installed successfully." << std::endl;

        std::cout << "Trying to install driver..." << std::endl;
        BOOL result;

#if 0
        /** [TODO] For some reason this is not working. Maybe a bug or a problem with the INF file? **/
        
        /* Check if the INF file is signed and valid. */
        SP_INF_SIGNER_INFO_W infInfo = { 0 };
        result = SetupVerifyInfFile(
            FullInfPath->c_str(), //PCWSTR InfName
            NULL, //PSP_ALTPLATFORM_INFO AltPlatformInfo
            &infInfo //PSP_INF_SIGNER_INFO_W InfSignedInfo
        );
        if (!result)
        {
            DWORD error = GetLastError();

            std::wcerr << L"Error: INF file " << *FullInfPath << L" is not valid or improperly signed." << std::endl;
            std::cerr << "Error code 0x" << std::hex << error << std::endl;

            errorCode = -1;
            goto clean_exit;
        }

        std::wcout << "INF file signature information" << std::endl
            << "Catalog file: " << std::wstring(infInfo.CatalogFile) << std::endl
            << "Digital signer: " << std::wstring(infInfo.DigitalSigner) << std::endl
            << "Digital signer version: " << std::wstring(infInfo.DigitalSignerVersion) << std::endl
            << "Signer score: " << infInfo.SignerScore << std::endl;
#endif 

        result = UpdateDriverForPlugAndPlayDevices(
            0, //HWND hwndParent
            hardwareId, //LPCWSTR HardwareId
            FullInfPath->c_str(), //LPCWSTR FullInfPath
            INSTALLFLAG_NONINTERACTIVE, //DWORD InstallFlags
            nullptr //PBOOL bRebootRequired
        );
        if (!result)
        {
            std::cout << "Error installing driver:";
            DWORD error = GetLastError();
            switch (error)
            {
            case ERROR_FILE_NOT_FOUND:
                std::wcerr << L"Error: " << *FullInfPath << L" not found." << std::endl;
                break;
            case ERROR_INVALID_FLAGS:
                std::cerr << "Error: Invalid flags" << std::endl;
                break;
            case ERROR_NO_SUCH_DEVINST:
                std::cerr << "Error: HardwareId not found or invalid." << std::endl;
                break;
            case ERROR_NO_MORE_ITEMS:
                std::cerr << "Info: Driver is not a better match to the current one already installed." << std::endl;
                break;
            }
        }
        else {
            std::cout << "Success installing driver." << std::endl;
        }
    }
    else {
        SW_DEVICE_LIFETIME lifetime;
        status = SwDeviceGetLifetime(device, &lifetime);

        if (FAILED(status))
        {
            std::cerr << "Error getting device lifetime." << std::endl;
            SwDeviceClose(device);
            errorCode = -1;
            goto clean_exit;
        }

        // If the lifetime is not SWDeviceLifetimeParentPresent there is no point in changing it back to SWDeviceLifetimeHandle, maybe there was no device installed?
        if (lifetime == SWDeviceLifetimeParentPresent)
        {
            // Forcing it back to SWDeviceLifetimeHandle makes sure it gets destroyed when SwDeviceClose is called.
            status = SwDeviceSetLifetime(device, SWDeviceLifetimeHandle);

            if (FAILED(status))
            {
                std::cerr << "Error setting device lifetime." << std::endl;
                SwDeviceClose(device);
                errorCode = -1;
                goto clean_exit;
            }
            SwDeviceClose(device);
        }
        std::cout << "Device uninstalled successfully." << std::endl;

        std::wcout << L"Trying to remove driver " << *FullInfPath << "." << std::endl;
        BOOL result = DiUninstallDriver(NULL, FullInfPath->c_str(), 0, NULL);
        if (!result)
        {
            std::wcerr << L"Error: Removing driver failed with " << std::hex << GetLastError() << L" trying to uninstall from " << *FullInfPath << std::endl;
            errorCode = -1;
            goto clean_exit;
        }
        std::cout << "Driver removed successfully." << std::endl;
    }
clean_exit:
    delete FullInfPath;
    return errorCode;
}
