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
#include "debug.h"
#include "driver.tmh"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (INIT, WindowsPerfPrintDriverVersion)
#pragma alloc_text (PAGE, WindowsPerfEvtDeviceAdd)
#pragma alloc_text (PAGE, WindowsPerfEvtWdfDriverUnload)
#endif

VOID
WindowsPerfEvtWdfDriverUnload(
    _In_ WDFDRIVER Driver
)
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER(Driver);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry");   

    WindowsPerfDeviceUnload();

    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "unloaded\n"));

    //
    // Stop WPP Tracing
    //
    WPP_CLEANUP(WdfDriverWdmGetDriverObject(Driver));
}

/// <summary>
/// DriverEntry initializes the driver and is the first routine called by the
/// system after the driver is loaded. DriverEntry specifies the other entry
/// points in the function driver, such as EvtDevice and DriverUnload.
/// </summary>
/// <param name="DriverObject">represents the instance of the function driver
/// that is loaded into memory. DriverEntry must initialize members of
/// DriverObject before it returns to the caller. DriverObject is allocated
/// by the system before the driver is loaded, and it is released by the system
/// after the system unloads the function driver from memory.</param>
/// <param name="RegistryPath">represents the driver specific path in the
/// Registry. The function driver can use the path to store driver related
/// data between reboots. The path does not store hardware instance specific
/// data.</param>
/// <returns>STATUS_SUCCESS if successful, STATUS_UNSUCCESSFUL otherwise.
/// </returns>
NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
{
    WDF_DRIVER_CONFIG config;
    NTSTATUS status;

    WPP_INIT_TRACING(DriverObject, RegistryPath);
    
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry");

    // Initialize the driver config structure
    WDF_DRIVER_CONFIG_INIT(&config, WindowsPerfEvtDeviceAdd);

    // Specify the callout driver's Unload function
    config.EvtDriverUnload = WindowsPerfEvtWdfDriverUnload;

    // Create a WDFDRIVER object
    status = WdfDriverCreate(DriverObject,
                            RegistryPath,
                            WDF_NO_OBJECT_ATTRIBUTES,
                            &config,
                            WDF_NO_HANDLE);
    if (!NT_SUCCESS(status)) {
        KdPrint(("Error: WdfDriverCreate failed 0x%x\n", status));
        return status;
    }

#if DBG
    WindowsPerfPrintDriverVersion();
#endif
    
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Exit");

    return status;
}

/// <summary>
/// EvtDeviceAdd is called by the framework in response to AddDevice call from
/// the PnP manager. We create and initialize a device object to represent a
/// new instance of the device. 
/// </summary>
/// <param name="Driver">Handle to a framework driver object created in DriverEntry</param>
/// <param name="DeviceInit">Pointer to a framework-allocated WDFDEVICE_INIT structure.</param>
/// <returns>NTSTATUS</returns>
NTSTATUS
WindowsPerfEvtDeviceAdd(
    IN WDFDRIVER       Driver,
    IN PWDFDEVICE_INIT DeviceInit
    )
{
    NTSTATUS status;

    UNREFERENCED_PARAMETER(Driver);

    PAGED_CODE();

    KdPrint(("Enter EvtDeviceAdd\n"));

    status = WindowsPerfDeviceCreate(DeviceInit);

    return status;
}


/// <summary>
/// This routine shows how to retrieve framework version string and
/// also how to find out to which version of framework library the
/// client driver is bound to. 
/// </summary>
/// <returns>NTSTATUS</returns>
NTSTATUS
WindowsPerfPrintDriverVersion(
    )
{
    NTSTATUS status;
    WDFSTRING string;
    UNICODE_STRING us;
    WDF_DRIVER_VERSION_AVAILABLE_PARAMS ver;

    //
    // 1) Retreive version string and print that in the debugger.
    //
    status = WdfStringCreate(NULL, WDF_NO_OBJECT_ATTRIBUTES, &string);
    if (!NT_SUCCESS(status)) {
        KdPrint(("Error: WdfStringCreate failed 0x%x\n", status));
        return status;
    }

    status = WdfDriverRetrieveVersionString(WdfGetDriver(), string);
    if (!NT_SUCCESS(status)) {
        //
        // No need to worry about delete the string object because
        // by default it's parented to the driver and it will be
        // deleted when the driverobject is deleted when the DriverEntry
        // returns a failure status.
        //
        KdPrint(("Error: WdfDriverRetrieveVersionString failed 0x%x\n", status));
        return status;
    }

    WdfStringGetUnicodeString(string, &us);
    KdPrint(("WindowsPerf %wZ\n", &us));

    WdfObjectDelete(string);
    string = NULL; // To avoid referencing a deleted object.

    //
    // 2) Find out to which version of framework this driver is bound to.
    //
    WDF_DRIVER_VERSION_AVAILABLE_PARAMS_INIT(&ver, 1, 0);
    if (WdfDriverIsVersionAvailable(WdfGetDriver(), &ver) == TRUE) {
        KdPrint(("Yes, framework version is 1.0\n"));
    }else {
        KdPrint(("No, framework verison is not 1.0\n"));
    }

    return STATUS_SUCCESS;
}

