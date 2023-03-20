# WindowsPerf

[[_TOC_]]

## Introduction

Take the Linux perf tool and port it to Windows on Arm with the same command line interface so that you can do deep performance analysis.

Currently we support:
* **counting model**, for obtaining aggregate counts of occurrences of special events, and
* **sampling model**, for determining the frequencies of event occurrences produced by program locations at the function, basic block, and/or instruction levels. Sampling model features include:
  * sampling mode initial merge, see  https://gitlab.com/Linaro/WindowsPerf/windowsperf/-/merge_requests/111
  * support for DLL symbol resolution, see  https://gitlab.com/Linaro/WindowsPerf/windowsperf/-/merge_requests/132
  * deduce from command line image name and PDB file name, see  https://gitlab.com/Linaro/WindowsPerf/windowsperf/-/merge_requests/134
  * stop sampling when sampled process ends, see  https://gitlab.com/Linaro/WindowsPerf/windowsperf/-/merge_requests/135

## Modules

WindowsPerf solution consists of two projects:
* `wperf` is a perf-like user space command line interface tool.
* `wperf-driver` is a Kernel-Mode Driver Framework (KMDF) driver.
  * See [Using WDF to Develop a Driver](https://learn.microsoft.com/en-us/windows-hardware/drivers/wdf/using-the-framework-to-develop-a-driver) article for more details on KMDF.

`wperf` application communicates with `wperf-driver` via I/O buffer. Proprietary binary protocol is used to exchange data, commands and status between two.

Currently `wperf-driver` can communicate with one instance of `wperf`.

## Contributing

When contributing to this repository, please first read `CONTRIBUTING.md` file for more details regarding how to contribute to this project.

## Project resources

For more information regarding project visit [WindowsPerf Wiki](https://linaro.atlassian.net/wiki/spaces/WPERF/overview).

# Building WindowsPerf project

Currently WindowsPerf is targeted for Windows on Arm devices. Both, user space `wperf` application and Kernel-mode driver `wperf-driver` are `ARM64EC` and `ARM64` binaries respectively.

Currently both projects `wperf` and `wperf-driver` in WindowsPerf solution are configured for cross compilation. You can build WindowsPerf natively on `ARM64` machines but please note that native compilation may be still wobbly due to constant improvements to WDK Kit.

Please build `wperf` application with `ARM64EC` configuration as it's requiring DIA SDK support which is not available in `ARM64` mode.

## Project requirements

### Toolchain and software kits

* [Visual Studio 2022 Community Edition](https://visualstudio.microsoft.com/vs/).
 * Windows Software Development Kit (SDK).
* [Windows Driver Kit (WDK)](https://learn.microsoft.com/en-us/windows-hardware/drivers/download-the-wdk).

Please note that SDK and WDK versions installed on your system must be compatible! First install Windows SDK using Visual Studio installer and after it’s installed proceed and install WDK which must match DSK version so that first three numbers of the version are the same. For example, SDK version `10.0.22621.1` and WDK `10.0.22621.382` is a match.

### Codebase

WindowsPerf solution is implemented in `C/C++17`.

### Debugging Kernel-Mode driver

You can see `wperf-driver` debug printouts with [DebugView](https://learn.microsoft.com/en-us/sysinternals/downloads/debugview). Kernel-Mode debug prints are produced with macros [DbgPrint](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-dbgprint) and [DbgPrintEx](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-dbgprintex).

After adding sampling model we've moved to more robust tracing. For kernel driver traces please use [TraceView](https://learn.microsoft.com/en-us/windows-hardware/drivers/devtest/traceview) application. You will need to present TraceView with `wperf-driver.pdb` file and you are ready to go!

Debugging Tools for Windows supports kernel debugging over a USB cable using EEM on an Arm device. Please refer to [Setting Up Kernel-Mode Debugging over USB EEM on an Arm device using KDNET](https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/setting-up-kernel-mode-debugging-over-usb-eem-arm-kdnet) article for more details.

### Creating Reliable Kernel-Mode Drivers

To create a reliable kernel-mode driver, follow these [guidelines](https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/creating-reliable-kernel-mode-drivers).

## Build solution from command line

```
> cd WindowsPerf
> devenv windowsperf.sln /Build "Debug|ARM64EC"
```

For more information regarding `devenv` and its command line options visit [Devenv command-line switches](https://learn.microsoft.com/en-us/visualstudio/ide/reference/devenv-command-line-switches?view=vs-2022).


## Build specific project in the solution

```
> cd WindowsPerf
> devenv windowsperf.sln /Rebuild "Debug|ARM64EC" /Project wperf\wperf.vcxproj
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

# Reference

* [Announcing WindowsPerf: Open-source performance analysis tool for Windows on Arm](https://community.arm.com/arm-community-blogs/b/infrastructure-solutions-blog/posts/announcing-windowsperf)
* [ARM64 Intrinsics](https://learn.microsoft.com/en-us/cpp/intrinsics/arm64-intrinsics?view=msvc-170)
* [Building and Loading a WDF Driver](https://learn.microsoft.com/en-us/windows-hardware/drivers/wdf/building-and-loading-a-kmdf-driver)
* [Write a Universal Windows driver (KMDF) based on a template](https://learn.microsoft.com/en-us/windows-hardware/drivers/gettingstarted/writing-a-kmdf-driver-based-on-a-template)
