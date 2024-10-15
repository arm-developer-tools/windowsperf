# Introduction

The `wperf-devgen` project is a utility designed to facilitate the installation of the WindowsPerf Kernel Driver through the Software Device API.

# Requirements

Before running the executable make sure `wperf-driver.sys`, `wperf-driver.inf` and `wperf-driver.cat` are
all at the same directory as the `wperf-devgen` executable. Make sure those file are properly signed by Linaro and Microsoft.

> :warning: `wperf-devgen` requires latest Microsoft Visual C++ Redistributable version. If you encounter a `Runtime Error` (A Windows Error 0xC000007B) or a `System Error` (VCRUNTIME140.dll and MSVCP140.dll missing) during tool execution, please download and install the [latest Microsoft Visual C++ Redistributable Version](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist?view=msvc-170#latest-microsoft-visual-c-redistributable-version).

# Usage

You can type `wperf-devgen install` to install the software device along with the driver. If some error occurs and
the device and driver were already installed make sure to type `wperf-devgen uninstall` to remove the device first.

## Driver Installation

```
> wperf-devgen.exe install
Executing command: install.
Install requested.
Waiting for device creation...
Device installed successfully.
Trying to install driver...
Success installing driver.
```

## Driver Uninstallation

```
> wperf-devgen uninstall
Executing command: uninstall.
Uninstall requested.
Waiting for device creation...
Device uninstalled successfully.
Trying to remove driver C:\Users\%USER%r\path\to\wperf-driver\wperf-driver.inf.
Driver removed successfully.
```
