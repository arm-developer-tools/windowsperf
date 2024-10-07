# wperf-driver

[[_TOC_]]

# Build wperf-driver

You can build `wperf-driver` project from the command line:

```
> devenv windowsperf.sln /Rebuild "Debug|ARM64" /Project wperf-driver\wperf-driver.vcxproj
```

# Debugging Kernel-Mode driver

You can see `wperf-driver` debug printouts with [DebugView](https://learn.microsoft.com/en-us/sysinternals/downloads/debugview). Kernel-Mode debug prints are produced with macros [KdPrintEx](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-kdprintex).

After adding a sampling model we've moved to more robust tracing. For kernel driver traces please use [TraceView](https://learn.microsoft.com/en-us/windows-hardware/drivers/devtest/traceview) application. You will need to present the TraceView with the `wperf-driver.pdb` file and you are ready to go!

Debugging Tools for Windows supports kernel debugging over a USB cable using EEM on an Arm device. Please refer to [Setting Up Kernel-Mode Debugging over USB EEM on an Arm device using KDNET](https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/setting-up-kernel-mode-debugging-over-usb-eem-arm-kdnet) article for more details.

## Tracing wperf-driver

Enable Kernel Driver tracing with project specific `ENABLE_TRACING` macro. In order to use it just set it at the VS `wperf-driver` project `Properties` -> `C/C++` -> `Preprocessor` -> `Preprocessor Definitions`.

## Inject ETW trace from wperf-driver

New macro, `ENABLE_ETW_TRACING` is used to control ETW output inside the `wperf-driver`.

# Kernel Driver Installation

Kernel drivers can be installed and removed on ARM64 machines with DevCon command.

Please note that we now release `WindowsPerf` with the signed Kernel driver `wperf-driver`. This driver can be installed on all ARM64 machines without tempering with SecureBoot or other critical security settings. Please refer to [releases](https://github.com/arm-developer-tools/windowsperf/releases) to get the latest stable driver and user space application.

First change directory to directory where your `wperf-driver` files (`wperf-driver.cat`, `wperf-driver.inf`, and `wperf-driver.sys`) are:

```
> cd D:\Workspace\wperf-driver
> dir
28/10/2022  10:07             2,459 wperf-driver.cat
28/10/2022  10:06             7,738 wperf-driver.inf
28/10/2022  10:06            33,712 wperf-driver.sys
```

Use `DevCon Install` command to install the driver and `DevCon Status` to check the driver's status. You can also remove the driver using the `DevCon Remove` command. See examples below.

Note: We've prepared wrapper scripts for installation, removal and status check of the driver. Please see [wperf-scripts/devcon](https://github.com/arm-developer-tools/windowsperf/tree/main/wperf-scripts/devcon).

## DevCon Install

Creates a new, root-enumerated devnode for a non-Plug and Play device and installs its supporting software. Valid only on the local computer. See [DevCon Install](https://learn.microsoft.com/en-us/windows-hardware/drivers/devtest/devcon-install) article for more details.

```
> devcon install wperf-driver.inf Root\WPERFDRIVER
Device node created. Install is complete when drivers are installed...
Updating drivers for Root\WPERFDRIVER from D:\Workspace\wperf-driver\wperf-driver.inf.
Drivers installed successfully.
```

Other uses of `devcon` command are:

## DevCon Remove

Removes the device from the device tree and deletes the device stack for the device. As a result of these actions, child devices are removed from the device tree and the drivers that support the device are unloaded. See [DevCon Remove](https://learn.microsoft.com/en-us/windows-hardware/drivers/devtest/devcon-remove) article for more details.

```
> devcon remove wperf-driver.inf Root\WPERFDRIVER
ROOT\SYSTEM\0001                                            : Removed
1 device(s) were removed.
```

## DevCon Status

Displays the status (running, stopped, or disabled) of the driver for devices on the computer. See [DevCon Status](https://learn.microsoft.com/en-us/windows-hardware/drivers/devtest/devcon-status) article for more details.

```
> devcon status wperf-driver.inf Root\WPERFDRIVER
ROOT\SYSTEM\0001
    Name: WPERFDRIVER Driver
    Driver is running.
1 matching device(s) found.
```

# DevCon Device Console (devcon.exe)

For more information about `devcon` command please visit [Device Console Commands](https://learn.microsoft.com/en-us/windows-hardware/drivers/devtest/devcon-general-commands).

Please note that `devcon.exe` should be located in `%WindowsSdkDir%\tools\arm64\devcon.exe` location. For example in  `c:\Program Files (x86)\Windows Kits\10\Tools\ARM64\` on your ARM64 machine. Please note that the Visual Studio environment variable, `%WindowsSdkDir%`, represents the path to the Windows Kits directory where the kits are installed.

If you face issues installing the driver you might look at the last sections of `c:\windows\inf\setupapi.app.log` and `c:\windows\inf\setupapi.dev.log`. They might provide some hints.

## Where can I download DevCon?

DevCon (`Devcon.exe`) is included when you install the [WDK](https://learn.microsoft.com/en-us/windows-hardware/drivers/download-the-wdk), Visual Studio, and the Windows SDK for desktop apps. For information about downloading the kits, see [Windows Hardware Downloads](https://learn.microsoft.com/en-us/windows-hardware/drivers/download-the-wdk).

You can also refer to [Where can I download DevCon?](https://learn.microsoft.com/en-us/windows-hardware/drivers/devtest/devcon#where-can-i-download-devcon) article for more details.

## Extra step (installing non-signed driver)

Users who develop their own `wperf-driver` (e.g., build from sources), and do not sign it themselves, will be forced to do a few extra steps to enable non-signed driver installation. Please note that early WindowsPerf binary releases (before release [2.4.0](https://github.com/arm-developer-tools/windowsperf/releases/2.4.0)) we distributed WITHOUT signed driver.

Enabling installation of an unsigned driver on WOA ARM64 machines requires a few extra steps:

**Note**: Use below steps at your own risk!

### You must first enable test-signed code

Use the following BCDEdit command line (Administrator rights required):

```
> bcdedit /set testsigning on
```

See [Enable Loading of Test Signed Drivers](https://learn.microsoft.com/en-us/windows-hardware/drivers/install/the-testsigning-boot-configuration-option#enable-or-disable-use-of-test-signed-code) article for more details.

Note: *This change takes effect after reboot*.

After reboot you can try to install the driver with the `DevCon Install` command.

This might further require you to:

### Second, Disable BitLocker on Windows 11

If `BitLocker` is enabled on your machine and before you attempt to disable it make sure you have `BitLocker Recovery Key` for your machine.
You can obtain it in advance with procedure described [here](https://support.microsoft.com/en-us/windows/finding-your-bitlocker-recovery-key-in-windows-6b71ad27-0b89-ea08-f143-056f5ab347d6).

### Third, Disable Secure Boot on your machine

[Disable secure boot](https://learn.microsoft.com/en-us/windows-hardware/manufacture/desktop/disabling-secure-boot?view=windows-11) from BIOS if it has been enabled. See also [Secure boot feature signing requirements for kernel-mode drivers](https://learn.microsoft.com/en-us/windows/win32/w8cookbook/secured-boot-signing-requirements-for-kernel-mode-drivers) article for more details.

# Kernel Driver user space configuration

Users can now specify counting timer period (see !300+ for more details). User can adjust count timer period from `10ms` to `100ms`. This value has to be set for each count separately. Driver will not "remember" adjusted counter timer period. Users must specify it with `--config count.period=VALUE` command line option (see !301+ for more details), where `VALUE` is between `10` and `100` ms. See example:

## Example setting of counting timer value to 10ms

```
> wperf stat .... --config count.period=10 ...
```

## How to check current `config.count.period*` settings

Users can check current minimal, maximal and default counting timer period with `wperf test` command. See example below:

```
> wperf test
...
        config.count.period                                 100
        config.count.period_max                             100
        config.count.period_min                             10
...
```

Note: Please note that this period is set in `PMU_CTL_START` IOCTRL message.

## Dry test --config command line setting

You can dry test this setting and:

```
> wperf test --config count.period=13
...
        config.count.period                                 13
...
```
