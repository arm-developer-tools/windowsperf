# WindowsPerf

## Introduction

WindowsPerf is (`Linux perf` inspired) Windows on Arm performance profiling tool. Profiling is based on ARM64 PMU and its hardware counters. WindowsPerf supports the counting model for obtaining aggregate counts of occurrences of special events, and sampling model for determining the frequencies of event occurrences produced by program locations at the function, basic block, and/or instruction levels.

WindowsPerf can instrument Arm CPU performance counters. As of now, it can collect:
- Core PMU counters for all or specified CPU core.
- unCore PMU counters:
  - ARM DynamIQ Shared Unit (DSU) PMU and
  - DMC-520 Dynamic Memory Controller are supported.
- Arm Statistical Profiling Extension (SPE).

Currently we support:
- **counting model**: WindowsPerf can utilize the Performance Monitoring Unit (PMU) counters from the CPU, DSU, and DMC to capture detailed counting profiles of workloads. By leveraging these counters, WindowsPerf can monitor various performance metrics and events, providing insights into the behavior and efficiency of the system. This comprehensive profiling helps in identifying bottlenecks, optimizing performance, and ensuring that workloads are running efficiently across different components of the system. You can find examples [here](https://github.com/arm-developer-tools/windowsperf/tree/main/wperf#counting-model).
- **sampling model**: WindowsPerf can sample CPU Performance Monitoring Unit (PMU) events using two methods: software sampling and hardware sampling. In software sampling, the process is triggered by a PMU counter overflow interrupt request (IRQ), allowing the system to collect data at specific intervals. On the other hand, hardware sampling with the Arm Statistical Profiling Extension (SPE) provides precise sampling directly in hardware. This method captures detailed performance data without the overhead associated with software-based sampling, resulting in more accurate and reliable measurements. You can find examples [here](https://github.com/arm-developer-tools/windowsperf/tree/main/wperf#sampling-model).

### Arm Telemetry Solution Integration

The integration of WindowsPerf and [Arm Telemetry Solution](https://developer.arm.com/documentation/109542/0100/About-Arm-CPU-Telemetry-Solution) is a significant advancement in performance analysis on Windows On Arm. This integration is primarily based on PMU (Performance Monitoring Unit) events, which provide a detailed insight into the system’s performance. One of the standout features of the WindowsPerf Tool is the implementation of the [Arm Topdown Methodology](https://developer.arm.com/documentation/109542/0100/Arm-Topdown-methodology) for μarch (microarchitecture) performance analysis. This methodology is tailored for each Arm CPU μarch. It involves the use of PMU events, metrics, and groups of metrics to provide a comprehensive analysis of the system’s performance. Furthermore, the WindowsPerf Tool is capable of [platform μarchitecture detection](https://gitlab.arm.com/telemetry-solution/telemetry-solution/-/tree/main), including Neoverse-N1, V1, and N2 CPUs.

The Arm Telemetry Solution also includes a [topdown-tool](https://gitlab.arm.com/telemetry-solution/telemetry-solution/-/tree/main/tools/topdown_tool) that leverages the WindowsPerf as a backend for Windows On Arm. This tool applies the top-down methodology to break down CPU performance into different hierarchical levels, providing a detailed and systematic approach to performance analysis.

The `topdown-tool` uses the WindowsPerf to access the PMU events and metrics on Windows On Arm, enabling it to gather and analyze performance data directly from the hardware. This integration allows the `topdown-tool` to provide a comprehensive view of the system’s performance, from high-level metrics to low-level, detailed μarch events.

## WindowsPerf Installation

You can find the latest WindowsPerf installation instructions in [INSTALL.md](https://github.com/arm-developer-tools/windowsperf/blob/main/INSTALL.md).

## WindowsPerf Releases

You can find all binary releases of WindowsPerf (`wperf-driver` and `wperf` application) [here](https://github.com/arm-developer-tools/windowsperf/releases).

## Building WindowsPerf

You can find the latest WindowsPerf build instructions in [BUILD.md](https://github.com/arm-developer-tools/windowsperf/blob/main/BUILD.md).

## Contributing

When contributing to this repository, please first read [CONTRIBUTING.md](https://github.com/arm-developer-tools/windowsperf/blob/main/CONTRIBUTING.md) file for more details regarding how to contribute to this project.

## WindowsPerf Modules

WindowsPerf solution contains few projects:

- [wperf](https://github.com/arm-developer-tools/windowsperf/tree/main/wperf) is a perf-like user space command line interface tool.
- [wperf-test](https://github.com/arm-developer-tools/windowsperf/tree/main/wperf-test) contains unit tests for the `wperf` project.
- [wperf-driver](https://github.com/arm-developer-tools/windowsperf/tree/main/wperf-driver) is a Kernel-Mode Driver Framework (KMDF) driver.
  - See [Using WDF to Develop a Driver](https://learn.microsoft.com/en-us/windows-hardware/drivers/wdf/using-the-framework-to-develop-a-driver) article for more details on KMDF.
- [wperf-devgen](https://github.com/arm-developer-tools/windowsperf/tree/main/wperf-devgen) is our own simple implementation of tool which can install or remove [wperf-driver](https://github.com/arm-developer-tools/windowsperf/tree/main/wperf-driver).
  - See [INSTALL.md](https://github.com/arm-developer-tools/windowsperf/blob/main/INSTALL.md) for more details and usage.
- [wperf-installer](https://github.com/arm-developer-tools/windowsperf/tree/main/wperf-installer) is our Windows Installer project. The project uses [WiX Toolset](https://wixtoolset.org/) to build a MSI package to install WindowsPerf. This project requires WiX v5.
  - See [wperf-installer/README.md](https://github.com/arm-developer-tools/windowsperf/blob/main/wperf-installer/README.md) for more details.
- [wperf-lib](https://github.com/arm-developer-tools/windowsperf/tree/main/wperf-lib) is our WindowsPerf C library, please note that is doesn't not support all the latest features of WindowsPerf.
  - [wperf-lib-app](https://github.com/arm-developer-tools/windowsperf/tree/main/wperf-lib-app) is an example application linked with `wperf-lib`.
    - [wperf-lib-c-compat](https://github.com/arm-developer-tools/windowsperf/tree/main/wperf-lib-app/wperf-lib-c-compat) is smoke test application for `wperf-lib`.
    - [wperf-lib-timeline](https://github.com/arm-developer-tools/windowsperf/tree/main/wperf-lib-app/wperf-lib-timeline) is smoke test application for `wperf-lib`.

Other directories contain:
- [wperf-common](https://github.com/arm-developer-tools/windowsperf/tree/main/wperf-common) contains common code between `wperf` and `wperf-driver` project. Mostly data structures describing IOCTRL binary protocol.
  - Note: [wperf](https://github.com/arm-developer-tools/windowsperf/tree/main/wperf) application communicates with [wperf-driver](https://github.com/arm-developer-tools/windowsperf/tree/main/wperf-driver) via IOCTRL buffer. Proprietary binary protocol is used to exchange data, commands and status between two.
- [wperf-scripts](https://github.com/arm-developer-tools/windowsperf/tree/main/wperf-scripts) contains various scripts including testing scripts.

## Project resources

For more information regarding the project visit [WindowsPerf Wiki](https://linaro.atlassian.net/wiki/spaces/WPERF/overview).

# References

## Blogs And Announcements

- [WindowsPerf Release 3.7.2](https://www.linaro.org/blog/expanding-profiling-capabilities-with-windowsperf-372-release) blog post.
- [WindowsPerf Release 3.3.0](https://www.linaro.org/blog/windowsperf-release-3-3-0/) blog post.
- [WindowsPerf Release 3.0.0](https://www.linaro.org/blog/windowsperf-release-3-0-0/) blog post.
- [WindowsPerf Release 2.5.1](https://www.linaro.org/blog/windowsperf-release-2-5-1/) blog post.
- [WindowsPerf release 2.4.0 introduces the first stable version of sampling model support](https://www.linaro.org/blog/windowsperf-release-2-4-0-introduces-the-first-stable-version-of-sampling-model-support/) blog post.
- [Announcing WindowsPerf: Open-source performance analysis tool for Windows on Arm](https://community.arm.com/arm-community-blogs/b/infrastructure-solutions-blog/posts/announcing-windowsperf) blog post.

### Linaro Connect (Madrid) 2024:
- [Enhancements in WindowsPerf](https://resources.linaro.org/zh/resource/fMJbSCfrn29fvxTbXopnGA).
- [Boost your workload and platform performance on Windows on Arm with WindowsPerf](https://resources.linaro.org/zh/resource/kiYEWhaFDFpEGVQD7XuvXa).

## Arm Learning Path

- [Perf for Windows on Arm (WindowsPerf)](https://learn.arm.com/install-guides/wperf/).
- [Get started with WindowsPerf](https://learn.arm.com/learning-paths/laptops-and-desktops/windowsperf/).
- [Sampling CPython with WindowsPerf](https://learn.arm.com/learning-paths/laptops-and-desktops/windowsperf_sampling_cpython/).

## Arm CPU Telemetry Solution

### Arm Neoverse PMU Guides

- [Arm Neoverse N1 PMU Guide](https://developer.arm.com/documentation/109956/latest/)
- [Arm Neoverse V1 PMU Guide](https://developer.arm.com/documentation/109708/latest/)
- [Arm Neoverse N2 PMU Guide](https://developer.arm.com/documentation/109710/latest/)
- [Arm Neoverse V2 PMU Guide](https://developer.arm.com/documentation/109709/latest/)

### Arm Telemetry Specifications

- [Arm CPU Telemetry Solution Topdown Methodology Specification](https://developer.arm.com/documentation/109542/0100/Introduction/Useful-resources).
- [Arm Telemetry Solution Tools](https://gitlab.arm.com/telemetry-solution/telemetry-solution).
- [Arm Neoverse N1 Core Telemetry Specification](https://developer.arm.com/documentation/108070/0100/?lang=en).
- [Arm Neoverse V1 Core Telemetry Specification](https://developer.arm.com/documentation/109216/0100/?lang=en).
- [Arm Neoverse N2 Core Telemetry Specification](https://developer.arm.com/documentation/109215/0200/?lang=en).
- [Arm Neoverse V2 Core Telemetry Specification](https://developer.arm.com/documentation/109528/0200)
- [Arm Statistical Profiling Extension: Performance Analysis Methodology White Paper](https://developer.arm.com/documentation/109429/latest/) documentation.
- [Arm Neoverse V1 – Top-down Methodology for Performance Analysis & Telemetry Specification](https://community.arm.com/arm-community-blogs/b/infrastructure-solutions-blog/posts/arm-neoverse-v1-top-down-methodology) blog (with white paper).

## Other

- [ARM64 Intrinsics](https://learn.microsoft.com/en-us/cpp/intrinsics/arm64-intrinsics?view=msvc-170) documentation.
- [Building and Loading a WDF Driver](https://learn.microsoft.com/en-us/windows-hardware/drivers/wdf/building-and-loading-a-kmdf-driver) documentation.
- [Write a Universal Windows driver (KMDF) based on a template](https://learn.microsoft.com/en-us/windows-hardware/drivers/gettingstarted/writing-a-kmdf-driver-based-on-a-template) documentation.
