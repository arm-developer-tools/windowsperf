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

#include <iostream>
#include <conio.h>
#include <windows.h>
#include <swdevice.h>
#include <newdev.h>
#include <sstream>
#include <strsafe.h>
#include <SetupAPI.h>
#include <initguid.h>
#include <devpropdef.h>
#include <devpkey.h>
#include <newdev.h>

#include "wperf-common\public_ver.h"
#include "wperf-common\gitver.h"

#define MAX_STRING_LENGTH 1024
#define MAX_HARDWARELIST_SIZE 2048

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

BOOL do_update_driver()
{
    BOOL exit = true, reboot = false;

    if (!UpdateDriverForPlugAndPlayDevices(NULL, hardwareId, FullInfPath->c_str(), INSTALLFLAG_FORCE, &reboot)) {
        exit = false;

        std::cerr << "Error updating driver";
        if (reboot)
        {
            std::cerr << " - Reboot please.";
        }
        std::cerr << std::endl;

        goto clean;
    }

clean:
    return exit;
}

BOOL do_search(HDEVINFO& deviceInfoSet, SP_DEVINFO_DATA& devInfoData)
{
    DWORD index = 0;
    SP_DEVINFO_DATA searchDevInfoData;

    searchDevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    while (SetupDiEnumDeviceInfo(deviceInfoSet, index, &searchDevInfoData))
    {
        index++;
        DEVPROPTYPE propType;
        WCHAR returnedHardwareID[MAX_HARDWARELIST_SIZE];
        DWORD returnedSize = 0;

        ZeroMemory(returnedHardwareID, sizeof(returnedHardwareID));

        if (!SetupDiGetDeviceProperty(deviceInfoSet, &searchDevInfoData, &DEVPKEY_Device_HardwareIds, &propType, (PBYTE)returnedHardwareID, sizeof(returnedHardwareID), &returnedSize, 0))
        {
            /* We might not be able to get information for all devices. */
        }

        WCHAR* ptr = returnedHardwareID;
        while (ptr[0] != L'\0')
        {
            std::wstring currHardwareId(ptr);
            if (std::wstring(hardwareId) == currHardwareId)
            {
                std::wcout << std::wstring(ptr) << std::endl;
                devInfoData = searchDevInfoData;
                return true;
            }
            ptr += currHardwareId.size() + 1; // Jump to next string
        }
    }
    return false;
}

BOOL do_remove_device()
{
    BOOL exit = true;
    BOOL found = false;
    GUID classGUID;
    WCHAR className[MAX_STRING_LENGTH];
    WCHAR hwIdList[MAX_STRING_LENGTH];
    HDEVINFO deviceInfoSet = INVALID_HANDLE_VALUE;
    SP_DEVINFO_DATA deviceInfoData{ 0 };

    ZeroMemory(hwIdList, sizeof(hwIdList));
    if (FAILED(StringCchCopy(hwIdList, LINE_LEN, hardwareId))) {
        exit = false;
        goto clean;
    }

    if (!SetupDiGetINFClass(FullInfPath->c_str(), &classGUID, className, sizeof(className) / sizeof(WCHAR), 0))
    {
        std::cerr << "Error getting INF Class information" << std::endl;
        exit = false;
        goto clean;
    }

    deviceInfoSet = SetupDiGetClassDevs(NULL, NULL, NULL, DIGCF_ALLCLASSES | DIGCF_PRESENT);
    if (deviceInfoSet == INVALID_HANDLE_VALUE)
    {
        std::cerr << "Error retrieving class deviceInfoSet " <<  std::endl;
        exit = false;
        goto clean;
    }

    found = do_search(deviceInfoSet, deviceInfoData);

    if(found)
    {
        std::cout << "Device found" << std::endl;
        if (!SetupDiCallClassInstaller(DIF_REMOVE, deviceInfoSet, &deviceInfoData))
        {
            std::cerr << "Error uninstalling device " << std::endl;
            exit = false;
            goto clean;
        }
    }
    else {
        std::cerr << "Error - WindowsPerf Device not found" << std::endl;
        exit = false;
        goto clean;
    }

clean:
    if (deviceInfoSet != INVALID_HANDLE_VALUE) {
        SetupDiDestroyDeviceInfoList(deviceInfoSet);
    }

    return exit;
}

BOOL do_create_device()
{
    BOOL exit = true;
    GUID classGUID;
    WCHAR className[MAX_STRING_LENGTH];
    WCHAR hwIdList[MAX_STRING_LENGTH];
    HDEVINFO deviceInfoSet = INVALID_HANDLE_VALUE;
    SP_DEVINFO_DATA deviceInfoData{ 0 };

    ZeroMemory(hwIdList, sizeof(hwIdList));
    if (FAILED(StringCchCopy(hwIdList, LINE_LEN, hardwareId))) {
        exit = false;
        goto clean;
    }

     if (!SetupDiGetINFClass(FullInfPath->c_str(), &classGUID, className, sizeof(className) / sizeof(WCHAR), 0))
    {
        std::cerr << "Error getting INF Class information" << std::endl;
        exit = false;
        goto clean;
    }

    // Search for the device first
    {
        HDEVINFO deviceInfoSet = SetupDiGetClassDevs(NULL, NULL, NULL, DIGCF_ALLCLASSES | DIGCF_PRESENT);
        if (deviceInfoSet == INVALID_HANDLE_VALUE)
        {
            std::cerr << "Error retrieving class deviceInfoSet " << std::endl;
            exit = false;
            goto clean;
        }

        if (do_search(deviceInfoSet, deviceInfoData))
        {
            std::cerr << "Device already installed" << std::endl;
            exit = false;
            goto clean;
        }
    }

    deviceInfoSet = SetupDiCreateDeviceInfoList(&classGUID, 0);
    if (deviceInfoSet == INVALID_HANDLE_VALUE)
    {
        std::cerr << "Error creating device info list" << std::endl;
        exit = false;
        goto clean;
    }

    deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    if (!SetupDiCreateDeviceInfo(deviceInfoSet,
        className,
        &classGUID,
        NULL,
        0,
        DICD_GENERATE_ID,
        &deviceInfoData))
    {
        std::cerr << "Error creating device info" << std::endl;
        exit = false;
        goto clean;
    }

    if (!SetupDiSetDeviceRegistryProperty(deviceInfoSet, &deviceInfoData, SPDRP_HARDWAREID, (LPBYTE)hwIdList, ((DWORD)wcslen(hwIdList) + 1 + 1) * sizeof(WCHAR)))
    {
        std::cerr << "Error setting device registry property" << std::endl;
        exit = false;
        goto clean;
    }

    if (!SetupDiCallClassInstaller(DIF_REGISTERDEVICE,
        deviceInfoSet,
        &deviceInfoData))
    {
        std::cerr << "Error calling class installer" << std::endl;
        exit = false;
    }
clean:
    if (deviceInfoSet != INVALID_HANDLE_VALUE) {
        SetupDiDestroyDeviceInfoList(deviceInfoSet);
    }

    return exit;
}

void print_help_header()
{
    std::wcout << L"WindowsPerf-DevGen"
        << L" ver. " << MAJOR << "." << MINOR << "." << PATCH
        << L" ("
        << WPERF_GIT_VER_STR
        << L"/"
#ifdef _DEBUG
        << L"Debug"
#else
        << L"Release"
#endif
        << L") WindowsPerf's Kernel Driver Installer."
        << std::endl;

    std::wcout << L"Report bugs to: https://gitlab.com/Linaro/WindowsPerf/windowsperf/-/issues"
        << std::endl;
}

void print_usage()
{
    const wchar_t* wsHelp = LR"(
NAME:
    wperf-devgen - Install WindowsPerf Kernel Driver

SYNOPSIS:
    wperf [--version] [--help] [OPTIONS]

OPTIONS:

    -h, --help
        Run wperf-devgen help command.

    --version
        Display version.

    install
        Install WindowsPerf Kernel Driver.

    uninstall
        Uninstall previously installed WindowsPerf Kernel Driver
)";

    std::wcout << wsHelp;
}

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        print_help_header();
        print_usage();
        return -1;
    }
    else
    {
        // Simple command line parser with only one valid CLI argument
        std::string cli_arg = argv[1];
        if (cli_arg == "--help" || cli_arg == "-h")
        {
            print_help_header();
            print_usage();
            return 0;
        }
        else if (cli_arg == "--version")
        {
            print_help_header();
            return 0;
        }
    }

    int errorCode = 0;

    GetFullInfPath();

    bool is_install;

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
    if (is_install)
    {
        if (!do_create_device())
        {
            errorCode = GetLastError();
            goto clean_exit;
        }

        if (!do_update_driver())
        {
            errorCode = GetLastError();
            goto clean_exit;
        }

        std::cout << "Device installed successfully" << std::endl;
    }
    else {
        if (!do_remove_device())
        {
            errorCode = GetLastError();
            goto clean_exit;
        }
        std::cout << "Device uninstalled sucessfully" << std::endl;
    }

clean_exit:
    if(errorCode) std::cout << "GetLastError " << errorCode << std::endl;
    delete FullInfPath;
    return errorCode;
}
