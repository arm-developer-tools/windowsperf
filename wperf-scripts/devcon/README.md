# wperf-devcon helper scripts

[[_TOC_]]

:warning: This script is a helper script for those who prefer to use `devcon` tool from Windows Driver Kit (WDK), see this [article](https://learn.microsoft.com/en-us/windows-hardware/drivers/devtest/devcon#where-can-i-download-devcon).

:grey_exclamation: Please use `wperf-devgen.exe` distributed with latest WindowsPerf releases to install `wperf-driver`.

## Description

PowerShell helper script `wperf-devcon.ps1`. Manipulate `wperf-driver` installation, removal and check driver installation status.

### Usage

```
wperf-devcon.ps1  <status> -infpath <path\to\driver\dir>

    <status>                - install, remove or (default) status
    <path\to\driver\dir>    - path to directory with driver (`inf`, `sys` and `cat` files).
```

### Examples

```
PS> wperf-devcon.ps1 install -infpath <path\to\driver\dir>
PS> wperf-devcon.ps1 remove  -infpath <path\to\driver\dir>
PS> wperf-devcon.ps1 status  -infpath <path\to\driver\dir>
```

## Scripts

| Script operation           | Description                 | More info |
| -------------------------- | --------------------------- | --------- |
| `wperf-devcon.ps1 install` | Install `wperf-driver`      | [DevCon Install](https://gitlab.com/Linaro/WindowsPerf/windowsperf/-/tree/main/wperf-driver#devcon-install) |
| `wperf-devcon.ps1 remove`  | Remove `wperf-driver`       | [DevCon Remove](https://gitlab.com/Linaro/WindowsPerf/windowsperf/-/tree/main/wperf-driver#devcon-remove) |
| `wperf-devcon.ps1 status`  | Check `wperf-driver` status | [DevCon Status](https://gitlab.com/Linaro/WindowsPerf/windowsperf/-/tree/main/wperf-driver#devcon-status) |

## Examples

### Prepare directory with driver
```
PS>

dir C:\path\to\driver


    Directory: C:\path\to\driver


Mode                 LastWriteTime         Length Name
----                 -------------         ------ ----
-a----         1/12/2023   1:41 AM           2459 wperf-driver.cat
-a----         1/12/2023   1:40 AM           3869 wperf-driver.inf
-a----         1/12/2023   1:41 AM          33712 wperf-driver.sys
```

### Driver installation
```
PS C:\path\to\windowsperf\wperf-scripts\devcon> .\wperf-devcon.ps1 install -infpath C:\path\to\driver
Device node created. Install is complete when drivers are installed...
Updating drivers for Root\WPERFDRIVER from C:\path\to\driver\wperf-driver.inf.
Drivers installed successfully.
```

### Driver status check
```
PS C:\path\to\windowsperf\wperf-scripts\devcon> .\wperf-devcon.ps1 status -infpath C:\path\to\driver
ROOT\SYSTEM\0001
    Name: WPERFDRIVER Driver
    Driver is running.
1 matching device(s) found.
```

### Driver removal
```
PS C:\path\to\windowsperf\wperf-scripts\devcon> .\wperf-devcon.ps1 remove -infpath C:\path\to\driver
ROOT\SYSTEM\0001                                            : Removed
1 device(s) were removed.
```
