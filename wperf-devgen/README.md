# wperf-devgen

[[_TOC_]]

# Introduction

Project `wperf-devgen` is a help utility to install the Windows Perf Driver using the Software Device API.

# Requirements

Before running the executable make sure `wperf-driver.sys`, `wperf-driver.inf` and `wperf-driver.cat` are
all at the same directory as the `wperf-devgen` executable. Make sure those file are properly signed by Linaro and Microsoft.

# Usage

You can type `wperf-devgen install` to install the software device along with the driver. If some error occurs and
the device and driver were already installed make sure to type `wperf-devgen uninstall` to remove the device first.
