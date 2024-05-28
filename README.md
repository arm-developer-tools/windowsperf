# WindowsPerf

[[_TOC_]]

## Introduction

WindowsPerf is (`Linux perf` inspired) Windows on Arm performance profiling tool. Profiling is based on ARM64 PMU and its hardware counters. WindowsPerf supports the counting model for obtaining aggregate counts of occurrences of special events, and sampling model for determining the frequencies of event occurrences produced by program locations at the function, basic block, and/or instruction levels.

WindowsPerf can instrument Arm CPU performance counters. As of now, it can collect:
* Core PMU counters for all or specified CPU core.
* unCore PMU counters,
* system cache (DSU-520) and
* DRAM (DMC-620) are supported.

Currently we support:
* **counting model**, for obtaining aggregate counts of occurrences of special events, and
* **sampling model**, for determining the frequencies of event occurrences produced by program locations at the function, basic block, and/or instruction levels. Sampling model features include:
  * sampling mode initial merge, see  https://gitlab.com/Linaro/WindowsPerf/windowsperf/-/merge_requests/111
  * support for DLL symbol resolution, see  https://gitlab.com/Linaro/WindowsPerf/windowsperf/-/merge_requests/132
  * deduce from command line image name and PDB file name, see  https://gitlab.com/Linaro/WindowsPerf/windowsperf/-/merge_requests/134
  * stop sampling when sampled process ends, see  https://gitlab.com/Linaro/WindowsPerf/windowsperf/-/merge_requests/135

You can find example usage of [counting model](https://gitlab.com/Linaro/WindowsPerf/windowsperf/-/tree/main/wperf#counting-model) and [sampling model](https://gitlab.com/Linaro/WindowsPerf/windowsperf/-/tree/main/wperf#sampling-model) in `wperf` [README.md](https://gitlab.com/Linaro/WindowsPerf/windowsperf/-/blob/main/wperf/README.md).

## WindowsPerf Installation

You can find latest WindowsPerf installation instructions in [INSTALL.md](https://gitlab.com/Linaro/WindowsPerf/windowsperf/-/blob/main/INSTALL.md?ref_type=heads).

## WindowsPerf Releases

You can find all binary releases of WindowsPerf (`wperf-driver` and `wperf` application) [here](https://gitlab.com/Linaro/WindowsPerf/windowsperf/-/releases).

## Building WindowsPerf

You can find latest WindowsPerf build instructions in [BUILD.md](https://gitlab.com/Linaro/WindowsPerf/windowsperf/-/blob/main/BUILD.md?ref_type=heads).

## Contributing

When contributing to this repository, please first read [CONTRIBUTING.md](https://gitlab.com/Linaro/WindowsPerf/windowsperf/-/blob/main/CONTRIBUTING.md) file for more details regarding how to contribute to this project.

## WindowsPerf Modules

WindowsPerf solution contains few projects:

* [wperf](https://gitlab.com/Linaro/WindowsPerf/windowsperf/-/tree/main/wperf) is a perf-like user space command line interface tool.
* [wperf-test](https://gitlab.com/Linaro/WindowsPerf/windowsperf/-/tree/main/wperf-test) contains unit tests for the `wperf` project.
* [wperf-driver](https://gitlab.com/Linaro/WindowsPerf/windowsperf/-/tree/main/wperf-driver) is a Kernel-Mode Driver Framework (KMDF) driver.
  * See [Using WDF to Develop a Driver](https://learn.microsoft.com/en-us/windows-hardware/drivers/wdf/using-the-framework-to-develop-a-driver) article for more details on KMDF.
* [wperf-devgen](https://gitlab.com/Linaro/WindowsPerf/windowsperf/-/tree/main/wperf-devgen) is our own simple implementation of tool which can install or remove [wperf-driver](https://gitlab.com/Linaro/WindowsPerf/windowsperf/-/tree/main/wperf-driver).
  * See [INSTALL.md](https://gitlab.com/Linaro/WindowsPerf/windowsperf/-/blob/main/INSTALL.md?ref_type=heads) for more details and usage.

Other directories contain:
* [wperf-common](https://gitlab.com/Linaro/WindowsPerf/windowsperf/-/tree/main/wperf-common) contains common code between `wperf` and `wperf-driver` project. Mostly data structures describing IOCTRL binary protocol.
  * Note: [wperf](https://gitlab.com/Linaro/WindowsPerf/windowsperf/-/tree/main/wperf) application communicates with [wperf-driver](https://gitlab.com/Linaro/WindowsPerf/windowsperf/-/tree/main/wperf-driver) via IOCTRL buffer. Proprietary binary protocol is used to exchange data, commands and status between two.
* [wperf-scripts](https://gitlab.com/Linaro/WindowsPerf/windowsperf/-/tree/main/wperf-scripts) contains various scripts including testing scripts.

## Project resources

For more information regarding the project visit [WindowsPerf Wiki](https://linaro.atlassian.net/wiki/spaces/WPERF/overview).

# References

## Blogs and announcements

- [WindowsPerf Release 3.3.0](https://www.linaro.org/blog/windowsperf-release-3-3-0/) blog post.
- [WindowsPerf Release 3.0.0](https://www.linaro.org/blog/windowsperf-release-3-0-0/) blog post.
- [WindowsPerf Release 2.5.1](https://www.linaro.org/blog/windowsperf-release-2-5-1/) blog post.
- [WindowsPerf release 2.4.0 introduces the first stable version of sampling model support](https://www.linaro.org/blog/windowsperf-release-2-4-0-introduces-the-first-stable-version-of-sampling-model-support/) blog post.
- [Announcing WindowsPerf: Open-source performance analysis tool for Windows on Arm](https://community.arm.com/arm-community-blogs/b/infrastructure-solutions-blog/posts/announcing-windowsperf) blog post.

## Arm Learning Path

- [Perf for Windows on Arm (WindowsPerf)](https://learn.arm.com/install-guides/wperf/).
- [Get started with WindowsPerf](https://learn.arm.com/learning-paths/laptops-and-desktops/windowsperf/).
- [Sampling CPython with WindowsPerf](https://learn.arm.com/learning-paths/laptops-and-desktops/windowsperf_sampling_cpython/).

## Arm CPU Telemetry Solution

- [Arm CPU Telemetry Solution Topdown Methodology Specification](https://developer.arm.com/documentation/109542/0100/Introduction/Useful-resources).
- [Arm Telemetry Solution Tools](https://gitlab.arm.com/telemetry-solution/telemetry-solution).
- [Arm Neoverse N1 Core Telemetry Specification](https://developer.arm.com/documentation/108070/0100/?lang=en).
- [Arm Neoverse V1 Core Telemetry Specification](https://developer.arm.com/documentation/109216/0100/?lang=en).
- [Arm Neoverse N2 Core Telemetry Specification](https://developer.arm.com/documentation/109215/0200/?lang=en).
- [Arm Statistical Profiling Extension: Performance Analysis Methodology White Paper](https://developer.arm.com/documentation/109429/latest/) documentation.
- [Arm Neoverse V1 – Top-down Methodology for Performance Analysis & Telemetry Specification](https://community.arm.com/arm-community-blogs/b/infrastructure-solutions-blog/posts/arm-neoverse-v1-top-down-methodology) blog (with white paper).

## Other

- [ARM64 Intrinsics](https://learn.microsoft.com/en-us/cpp/intrinsics/arm64-intrinsics?view=msvc-170) documentation.
- [Building and Loading a WDF Driver](https://learn.microsoft.com/en-us/windows-hardware/drivers/wdf/building-and-loading-a-kmdf-driver) documentation.
- [Write a Universal Windows driver (KMDF) based on a template](https://learn.microsoft.com/en-us/windows-hardware/drivers/gettingstarted/writing-a-kmdf-driver-based-on-a-template) documentation.
