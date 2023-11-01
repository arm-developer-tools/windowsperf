# wperf functional testing

[[_TOC_]]

# Introduction

This set of Python scripts is using [pytest](https://docs.pytest.org/) library to drive functional testing of [wperf](../../README.md). Here you can also find `stress.ps1` a PowerShell script to stress test the driver.

## Testing prerequisites

* You must have Python 3.11.1 (or newer) installed on your system and available on system environment PATH.
  * `pytest` module is also required, if missing please install it via `pip install pytest` command, see [pytest](https://pypi.org/project/pytest/). We've added `pytest.ini` file to ignore `telemetry-solution` directory (with `--ignore=telemetry-solution`) and setup common `pytest` command line options for consistency.
  * All required Python modules by WindowsPerf functional testing are specified in [requirements.txt](../requirements.txt). Use the `pip install -r requirements.txt` command to install all of the Python modules and packages listed in `requirements.txt` file.
* Tests must be executed on ARM64 Windows On Arm machine.
* [wperf-driver](../../wperf-driver/README.md) must be installed on the system.
* [wperf](../../wperf/README.md) application must be present in the same directory as tests or be on system environment PATH.
* To test if `wperf` JSON output is valid test directory must contain `schemes/` sub-directory.
  * Note: if `schemas` directory is missing JSON output tests will fail (and not skipped).
* Make sure that `windowsperf\wperf-scripts\tests\telemetry-solution\` sub-module is pulled with git.
  * How to pull sub-module: On init run the following command: `git submodule update --init --recursive` from within the git repo directory, this will pull all latest including submodules. Or you can close this repository manually.

### ustress framework test bench

We've added [ustress](https://gitlab.arm.com/telemetry-solution/telemetry-solution/-/tree/main/tools/ustress) test bench. It's located in [wperf-scripts](https://gitlab.com/Linaro/WindowsPerf/windowsperf/-/tree/main/wperf-scripts/tests?ref_type=heads). It allows users to build, execute `ustress` micro-benchmarks and track with `wperf` timeline specified metrics.

#### Build dependencies

* MSVC cross/native arm64 build environment, see `vcvarsall.bat`. See configuration needed to build `ustress` tests:
```
> %comspec% /k "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" arm64
**********************************************************************
** Visual Studio 2022 Developer Command Prompt v17.7.1
** Copyright (c) 2022 Microsoft Corporation
**********************************************************************
[vcvarsall.bat] Environment initialized for: 'arm64
```

* GNU Make 3.x - ustress Makefile requires it. Download "complete package" here: https://gnuwin32.sourceforge.net/packages/make.htm
* GNU tr - ustress Makefile internals requires it. Download "complete package" here: https://gnuwin32.sourceforge.net/packages/coreutils.htm
* clang targeting `aarch64-pc-windows-msvc`.
  * Go to MSVC installer and install: Modify -> Individual Components -> search "clang".
  * install: "C++ Clang Compiler..." and "MSBuild support for LLVM..."

You should be able to verify if your dependencies are correct by few simple checks:

```
> %comspec% /k "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" arm64
**********************************************************************
** Visual Studio 2022 Developer Command Prompt v17.7.1
** Copyright (c) 2022 Microsoft Corporation
**********************************************************************
[vcvarsall.bat] Environment initialized for: 'arm64'
```

```
>make --version
GNU Make 3.81
Copyright (C) 2006  Free Software Foundation, Inc.
This is free software; see the source for copying conditions.
There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.

This program built for i386-pc-mingw32
```

```
>clang --version
clang version 16.0.5
Target: aarch64-pc-windows-msvc
Thread model: posix
InstalledDir: C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\Llvm\ARM64\bin
```

#### Example: How to build ustress from command line for neoverse-n1 CPU

**Note**: Below steps require clang targeting `aarch64-pc-windows-msvc`. See `Build dependencies` chapter for more details.

```
> cd telemetry-solution/tools/ustress
> make clean
> make CPU=NEOVERSE-N1
```

#### See merge request documentation
* [ustress test environment](https://gitlab.com/Linaro/WindowsPerf/windowsperf/-/merge_requests/327#ustress-test-environment) section for more details.
* !327+, !330+ and !339+.

## Testing directory structure

In below test directory we've added locally `wperf.exe`.

### Go to regression test directory

```
> cd windowsperf\wperf-scripts\tests
```

### Test execution

```
>pytest
=============================== test session starts ===============================
platform win32 -- Python 3.11.1, pytest-7.2.0, pluggy-1.0.0
rootdir: C:\windowsperf\wperf-scripts\tests, configfile: pytest.ini
collected 143 items

wperf_cli_common_test.py ....                                                [  2%]
wperf_cli_config_test.py .....                                               [  6%]
wperf_cli_extra_events_test.py ....                                          [  9%]
wperf_cli_info_str_test.py .                                                 [  9%]
wperf_cli_json_validator_test.py ....                                        [ 12%]
wperf_cli_list_test.py .....                                                 [ 16%]
wperf_cli_metrics_test.py ................                                   [ 27%]
wperf_cli_padding_test.py ...........                                        [ 34%]
wperf_cli_record_test.py ..............                                      [ 44%]
wperf_cli_stat_test.py ...................................................   [ 80%]
wperf_cli_test_test.py .....                                                 [ 83%]
wperf_cli_timeline_test.py ..............                                    [ 93%]
wperf_cli_ustress_bench_test.py .......                                      [ 98%]
wperf_cli_ustress_dep_record_test.py .                                       [ 99%]
wperf_lib_app_test.py s                                                      [100%]

============================= short test summary info =============================
SKIPPED [1] wperf_lib_app_test.py:44: Can not run wperf-lib-app.exe
===================== 142 passed, 1 skipped in 237.56s (0:03:57) ==================
```

## Stress testing

Having properly setup the Python unit tests using the instructions above all you need to do is to run 

```
>stress.ps1 
```

The default number of cycles is 1. You can change that using the first command line argument, so if you want to run it 5 times just type

```
>stress.ps1 5
```

A bunch of commands will be executed in succession trying to stress the driver, at the end of each cycle the Python tests are also executed so make sure they are properly configured.