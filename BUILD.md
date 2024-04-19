# Building WindowsPerf solution

Currently WindowsPerf is targeted for Windows on Arm devices. Both, user space `wperf` application and Kernel-mode driver `wperf-driver` are `ARM64` binaries.
Both projects `wperf` and `wperf-driver` in WindowsPerf solution are configured for cross compilation.

> :warning: You can build WindowsPerf natively on `ARM64` machines but please note that native compilation may be still wobbly due to constant improvements to WDK Kit.

## wperf project

WindowsPerf's project `wperf` and its application require [DIA SDK](https://learn.microsoft.com/en-us/visualstudio/debugger/debug-interface-access/getting-started-debug-interface-access-sdk?view=vs-2022) support.

You may need to register DIA SDK using [regsvr32](https://support.microsoft.com/en-us/topic/how-to-use-the-regsvr32-tool-and-troubleshoot-regsvr32-error-messages-a98d960a-7392-e6fe-d90a-3f4e0cb543e5).
If the `DIA SDK` directory is missing from your system go to your VS installer, launch it and in `Workloads` tab please make sure you’ve installed `Desktop development with C++`. This installation should add `C:\Program Files\Microsoft Visual Studio\2022\Community\DIA SDK`. This directory should contain `DIA SDK` root file system with DIA SDK DLL.

> :warning: You may see `wperf` error message `CoCreateInstance failed for DIA` in case DIA SDK is not installed or registered as a COM service.

As an Administrator run from command line:

```
> cd "C:\Program Files\Microsoft Visual Studio\2022\Community\DIA SDK\bin\arm64"
> regsvr32 msdia140.dll
```

## Solution dependencies

### Toolchain and software kits

* [Visual Studio 2022 Community Edition](https://visualstudio.microsoft.com/vs/).
 * Windows Software Development Kit (SDK).
* [Windows Driver Kit (WDK)](https://learn.microsoft.com/en-us/windows-hardware/drivers/download-the-wdk).

Please note that SDK and WDK versions installed on your system must be compatible! First install Windows SDK using Visual Studio installer and after it’s installed proceed and install WDK which must match the DSK version so that the first three numbers of the version are the same. For example, the SDK version `10.0.22621.1` and WDK `10.0.22621.382` are a match.

### Code base

WindowsPerf solution is implemented in `C/C++17`.

## Build solution from command line

```
> cd WindowsPerf
> devenv windowsperf.sln /Build "Debug|ARM64"
```

For more information regarding `devenv` and its command line options visit [Devenv command-line switches](https://learn.microsoft.com/en-us/visualstudio/ide/reference/devenv-command-line-switches?view=vs-2022).

## Build specific project in the solution from command line

```
> cd WindowsPerf
> devenv windowsperf.sln /Rebuild "Debug|ARM64" /Project wperf\wperf.vcxproj
```

```
> cd WindowsPerf
> devenv windowsperf.sln /Rebuild "Debug|ARM64" /Project wperf-driver\wperf-driver.vcxproj
```

## Makefile

You can issue project build commands, project cleanup and documentation generation using GNU `make`. `Makefile` with popular commands is located in the solution directory.
Please note that for documentation generation you would need `Doxygen` installed on your system.

### Below few useful commands

| Action | Command |
| ------ | ------- |
| Rebuild whole solution | `> make all` |
| Rebuild `wperf` project | `> make wperf` |
| Rebuild `wperf-driver` project | `> make wperf-driver`  |
| Rebuild `wperf-test` project | `> make wperf-test`  |
| Rebuild `wperf`, `wperf-test` projects and run unit tests  | `> make test`  |
| Solution clean up | `> make clean`  |
| Create binary release packages | `> make release`  |
| Remove all build directories (deep clean)  | `> make purge`  |
