# WindowsPerf Regression Testing

This set of Python scripts is using [pytest](https://docs.pytest.org/) library to drive functional testing of [wperf](../../README.md). Here you can also find `stress.ps1` a PowerShell script to stress test the driver.

This set of Python scripts utilizes the A[pytest](https://docs.pytest.org/) library to conduct functional testing of An external link was removed to protect your privacy.. Additionally, it includes `stress.ps1`, a PowerShell script designed to stress test the driver.

## WindowsPerf Testing Prerequisites

WindowsPerf must be tested natively on WOA hardware with installed `wperf-driver` and `wperf`. Both must be compatible (have the same version). See `wperf --version` to see if your system facilitates above.

- You must have Python 3.11.1 (or newer) installed on your system and available on system environment PATH.
  - `pytest` module is also required, if missing please install it via `pip install pytest` command, see [pytest](https://pypi.org/project/pytest/). We've added `pytest.ini` file to ignore `telemetry-solution` directory (with `--ignore=telemetry-solution`) and setup common `pytest` command line options for consistency.
  - All required Python modules by WindowsPerf functional testing are specified in [requirements.txt](../requirements.txt). Use the `pip install -r requirements.txt` command to install all of the Python modules and packages listed in `requirements.txt` file.
- Tests must be executed on ARM64 Windows On Arm machine.
- [wperf-driver](../../wperf-driver/README.md) must be installed on the system.
- [wperf](../../wperf/README.md) application must be present in the same directory as tests or be on system environment PATH.
- To test if `wperf` JSON output is valid test directory must contain `schemas/` sub-directory.
  - Note: if `schemas` directory is missing JSON output tests will fail (and not skipped).
- Make sure that `windowsperf\wperf-scripts\tests\telemetry-solution\` submodule is pulled with git.
  - How to pull submodule: On init run the following command: `git submodule update --init --recursive` from within the git repo directory, this will pull all latest including submodules. Or you can clone submodules directly using `git clone --recurse-submodules`.

### Test Execution With Pytest

> The pytest framework makes it easy to write small tests, yet scales to support complex functional testing for applications and libraries.

Use `pytest` command in `windowsperf\wperf-scripts\tests\` directory to run all regression tests, see example below:

```
> cd windowsperf\wperf-scripts\tests\
> pytest
================================ test session starts =====================================
platform win32 -- Python 3.11.1, pytest-7.2.0, pluggy-1.0.0
rootdir: C:\Users\$USER\Desktop\wperf\merge-request\3.2.2, configfile: pytest.ini
collected 201 items

wperf_cli_common_test.py ....                                                       [  1%]
wperf_cli_config_test.py .....                                                      [  4%]
...

======================== 201 passed in 607.65s (0:10:07) =================================
```

Other `pytest` command invocation include:
- `pytest -x` - Stop after first failure.
- `pytest -v` - Print each test name in the report.
- `pytest wperf_cli_list_test.py` - Run tests in a module `wperf_cli_list_test.py`.
- `pytest test_mod.py::test_func` - Run a specific test within a module.

See `pytest` official documentation [How to invoke pytest](https://docs.pytest.org/en/7.2.x/how-to/usage.html) for more details.

## Arm Telemetry-Solution ustress Framework Test Bench

We've added [ustress](https://gitlab.arm.com/telemetry-solution/telemetry-solution/-/tree/main/tools/ustress) test bench. It's located in [wperf-scripts](https://github.com/arm-developer-tools/windowsperf/tree/main/wperf-scripts/tests). It allows users to build, execute `ustress` micro-benchmarks and track with `wperf` timeline specified metrics.

This test bench is composed of:
- `wperf-scripts/tests/telemetry-solution` - [telemetry-solution](https://gitlab.arm.com/telemetry-solution/telemetry-solution) git submodule pointing to specific SHA we use for testing. Use `git submodule update --init --recursive` command to "pull" this submodule.
- `pytest` test cases prefixed with `wperf_cli_ustress_`. For example:
  - `wperf_cli_ustress_bench_test.py` - Will attempt to build `ustress` from sources using available `clang` compiler environment.
  - `wperf_cli_ustress_dep_record_test.py` - `wperf record` tests with `ustress` micro-benchmarks.
  - `wperf_cli_ustress_dep_wperf_lib_timeline_test.py` - Test timeline (`wperf stat -t`) with `ustress` micro-benchmarks.
  - `wperf_cli_ustress_dep_wperf_test.py` - Test `wperf stat` using `ustress` micro-benchmarks and telemetry-solution defined metrics.

Note:
- `pytest` calls tests in alphabetical order so first "build test" has a _bench_ in name and dependent test files are prefixed with _dep_.
- `ustress` supports various `neoverse` CPU platforms only!

### ustress Build Dependencies

WindowsPerf `ustress` test bench will build `ustress` from sources. It will produce `ustress` micro-benchmarks executables using below dependencies.
To build `ustress` from sources you will need `MSVC cross/native arm64 build environment` with clang toolchain, and  `make` plus `tr` to drive `ustress` Makefile.

Note: when you run WindowsPerf test bench with `pytest` command, script [wperf_cli_ustress_bench_test.py](https://github.com/arm-developer-tools/windowsperf/blob/main/wperf-scripts/tests/wperf_cli_ustress_bench_test.py) will be responsible for building `ustress`. Tests in this script will be skipped if your AMR64 host platform is not supported. Currently `ustress` supports various `neoverse` platforms.

Note: If your ARM64 host platform is not supported you will see list of `ustress` skipped tests with assert warning.
```
=========================== short test summary info ===========================
SKIPPED [1] wperf_cli_ustress_bench_test.py:106: unsupported configuration: ustress do not support CPU=ARMV8_A
SKIPPED [1] wperf_cli_ustress_dep_record_test.py:104: unsupported configuration: ustress do not support CPU=ARMV8_A
SKIPPED [1] wperf_cli_ustress_dep_wperf_lib_timeline_test.py:107: unsupported configuration: ustress do not support CPU=ARMV8_A
SKIPPED [1] wperf_cli_ustress_dep_wperf_test.py:110: unsupported configuration: ustress do not support CPU=ARMV8_A
```

### List Of All Build Dependencies For `ustress` Build

- MSVC cross/native arm64 build environment, see `vcvarsall.bat`. See configuration needed to build `ustress` tests:

```
> %comspec% /k "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" arm64
**********************************************************************
** Visual Studio 2022 Developer Command Prompt v17.7.1
** Copyright (c) 2022 Microsoft Corporation
**********************************************************************
[vcvarsall.bat] Environment initialized for: 'arm64
```

- GNU Make 3.81 - ustress Makefile requires it.
  - Download "Complete package, except sources" with installer here: https://gnuwin32.sourceforge.net/packages/make.htm or
  - install it from the command line with `winget install GnuWin32.Make` command.
- GNU tr - ustress Makefile internals requires it.
  - Download "Complete package, except sources" with installer here: https://gnuwin32.sourceforge.net/packages/coreutils.htm
- clang targeting `aarch64-pc-windows-msvc`.
  - Go to MSVC installer and install: Modify -> Individual Components -> search "clang".
  - install: "C++ Clang Compiler..." and "MSBuild support for LLVM..."

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
> make --version
GNU Make 3.81
Copyright (C) 2006  Free Software Foundation, Inc.
This is free software; see the source for copying conditions.
There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.

This program built for i386-pc-mingw32
```

```
> clang --version
clang version 16.0.5
Target: aarch64-pc-windows-msvc
Thread model: posix
InstalledDir: C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\Llvm\ARM64\bin
```

### Example: How to build ustress from the command line for neoverse-n1 CPU

**Note**: Below steps require clang targeting `aarch64-pc-windows-msvc`. See `Build dependencies` chapter for more details.

```
> cd telemetry-solution/tools/ustress
> make clean
> make CPU=NEOVERSE-N1
```

### See Merge Request Documentation

- [ustress test environment](https://gitlab.com/Linaro/WindowsPerf/windowsperf/-/merge_requests/327#ustress-test-environment) section for more details.
- [327](https://gitlab.com/Linaro/WindowsPerf/windowsperf/-/merge_requests/327), [330](https://gitlab.com/Linaro/WindowsPerf/windowsperf/-/merge_requests/330), and [339](https://gitlab.com/Linaro/WindowsPerf/windowsperf/-/merge_requests/339).

## Testing Sampling Feature With Native CPython

We've added `cpython` test bench. It's located in `wperf-scripts/tests/`. It allows us to build CPython (in debug mode) from sources, run and test `wperf` sampling of CPython binary `python_d.exe`. This regression tests improve our overall testing strategy.

This test bench is composed of:
- `wperf-scripts/tests/cpython` - [CPython](https://github.com/python/cpython) git submodule pointing to specific SHA we use for testing. Use `git submodule update --init --recursive` command to "pull" this submodule.
- `pytest` test cases prefixed with `wperf_cli_cpython_`. For example:
    - `wperf_cli_cpython_bench_test.py` - will build CPython from sources (first build may take few minutes, second is under 30 sec).
    - `wperf_cli_cpython_dep_record_test.py` - will run sampling with `wperf record` on CPython `python_d.exe` executable.

Note: `pytest` calls tests in alphabetical order so first "build test" has _bench_ in name and dependent test files are prefixed with _dep_.

## Testing Directory Structure

In below test directory we've added locally `wperf.exe`.

### Go To Regression Test Directory

```
> cd windowsperf\wperf-scripts\tests
```

### Example Test Execution, neoverse-n1 CPU

Below you can see example regression test pass report ran on `neoverse-n1` WOA hardware with WindowsPerf 3.4.0 installed:

```
> pytest
================================ test session starts ================================
platform win32 -- Python 3.11.1, pytest-7.2.0, pluggy-1.0.0
rootdir: C:\Users\$USER\Desktop\testing\wperf\3.4.0, configfile: pytest.ini
collected 225 items

wperf_cli_common_test.py ....                                                  [  1%]
wperf_cli_config_test.py .....                                                 [  4%]
wperf_cli_extra_events_test.py ....                                            [  5%]
wperf_cli_hammer_core_test.py .................                                [ 13%]
wperf_cli_info_str_test.py .                                                   [ 13%]
wperf_cli_json_validator_test.py ..........                                    [ 18%]
wperf_cli_list_test.py .....                                                   [ 20%]
wperf_cli_lock_test.py ..                                                      [ 21%]
wperf_cli_metrics_test.py ................                                     [ 28%]
wperf_cli_padding_test.py ..............                                       [ 34%]
wperf_cli_prettytable_test.py .....                                            [ 36%]
wperf_cli_record_test.py ..............                                        [ 43%]
wperf_cli_sample_test.py ..                                                    [ 44%]
wperf_cli_stat_test.py ....................................................    [ 67%]
wperf_cli_test_test.py ........                                                [ 70%]
wperf_cli_timeline_test.py ................................................... [ 93%]
wperf_cli_ustress_bench_test.py ......                                         [ 96%]
wperf_cli_ustress_dep_record_test.py .                                         [ 96%]
wperf_cli_ustress_dep_wperf_lib_timeline_test.py .                             [ 97%]
wperf_cli_ustress_dep_wperf_test.py .....                                      [ 99%]
wperf_lib_app_test.py .                                                        [100%]
========================== WindowsPerf Test Configuration ===========================
OS: Windows-10-10.0.25217-SP0, ARM64
CPU: 80 x ARMv8 (64-bit) Family 8 Model D0C Revision 301, Ampere(R)
Python: 3.11.1 (tags/v3.11.1:a7a450f, Dec  6 2022, 19:44:02) [MSC v.1934 64 bit (ARM64)]
Time: 26/01/2024, 08:07:54
wperf: 3.4.0.2fb0f1b9
wperf-driver: 3.4.0.2fb0f1b9

========================== 225 passed in 745.45s (0:12:25) ==========================
```

### Example test execution (Lenovo x13s Snapdragon)

Below you can see example regression test pass report ran on Snapdragon WOA hardware with WindowsPerf 3.4.0 installed:

```
pytest
================================================= test session starts =================================================
platform win32 -- Python 3.11.0rc2, pytest-7.2.0, pluggy-1.0.0
rootdir: C:\Users\$USER\Workspace\driver-binary\3.4.0, configfile: pytest.ini
collected 215 items / 4 skipped

wperf_cli_common_test.py ....                                                                                    [  1%]
wperf_cli_config_test.py .....                                                                                   [  4%]
wperf_cli_cpython_bench_test.py ..                                                                               [  5%]
wperf_cli_cpython_dep_record_test.py .                                                                           [  5%]
wperf_cli_extra_events_test.py ....                                                                              [  7%]
wperf_cli_hammer_core_test.py .................                                                                  [ 15%]
wperf_cli_info_str_test.py .                                                                                     [ 15%]
wperf_cli_json_validator_test.py ..........                                                                      [ 20%]
wperf_cli_list_test.py .....                                                                                     [ 22%]
wperf_cli_lock_test.py ..                                                                                        [ 23%]
wperf_cli_metrics_test.py ..s....sssssssss                                                                       [ 31%]
wperf_cli_padding_test.py ..............                                                                         [ 37%]
wperf_cli_prettytable_test.py .....                                                                              [ 40%]
wperf_cli_record_test.py ..............                                                                          [ 46%]
wperf_cli_sample_test.py ..                                                                                      [ 47%]
wperf_cli_stat_test.py ....................................................                                      [ 71%]
wperf_cli_test_test.py ........                                                                                  [ 75%]
wperf_cli_timeline_test.py ...........................ssssssssssssssssss.......                                  [ 99%]
wperf_lib_app_test.py .                                                                                          [100%]
=========================================== WindowsPerf Test Configuration ============================================
OS: Windows-10-10.0.22621-SP0, ARM64
CPU: 8 x ARMv8 (64-bit) Family 8 Model D4B Revision   0, Qualcomm Technologies Inc
Python: 3.11.0rc2 (main, Sep 11 2022, 20:10:00) [MSC v.1933 64 bit (ARM64)]
Time: 12/02/2024, 13:20:41
wperf: 3.4.0.83e23b6e
wperf-driver: 3.4.0.83e23b6e

=============================================== short test summary info ===============================================
SKIPPED [1] wperf_cli_ustress_bench_test.py:106: unsupported configuration: ustress do not support CPU=ARMV8_A
SKIPPED [1] wperf_cli_ustress_dep_record_test.py:104: unsupported configuration: ustress do not support CPU=ARMV8_A
SKIPPED [1] wperf_cli_ustress_dep_wperf_lib_timeline_test.py:107: unsupported configuration: ustress do not support CPU=ARMV8_A
SKIPPED [1] wperf_cli_ustress_dep_wperf_test.py:110: unsupported configuration: ustress do not support CPU=ARMV8_A
SKIPPED [1] wperf_cli_metrics_test.py:71: unsupported metric: ddr_bw
SKIPPED [1] wperf_cli_metrics_test.py:71: unsupported metric: l3_cache
SKIPPED [1] wperf_cli_metrics_test.py:99: unsupported metric: l1d_cache_miss_ratio
SKIPPED [1] wperf_cli_metrics_test.py:99: unsupported metric: load_percentage
SKIPPED [1] wperf_cli_metrics_test.py:99: unsupported metric: store_percentage
SKIPPED [1] wperf_cli_metrics_test.py:99: unsupported metric: backend_stalled_cycles
SKIPPED [1] wperf_cli_metrics_test.py:129: unsupported metric: l1d_cache_miss_ratio
SKIPPED [1] wperf_cli_metrics_test.py:129: unsupported metric: load_percentage
SKIPPED [1] wperf_cli_metrics_test.py:129: unsupported metric: store_percentage
SKIPPED [1] wperf_cli_metrics_test.py:129: unsupported metric: backend_stalled_cycles
SKIPPED [4] wperf_cli_timeline_test.py:248: unsupported metric: l1d_cache_miss_ratio
SKIPPED [2] wperf_cli_timeline_test.py:248: unsupported metric: l1d_tlb_mpki
SKIPPED [8] wperf_cli_timeline_test.py:310: unsupported metric: l1d_cache_miss_ratio
SKIPPED [4] wperf_cli_timeline_test.py:310: unsupported metric: l1d_tlb_mpki
===================================== 187 passed, 32 skipped in 449.23s (0:07:29) =====================================
```

Note: some tests are skipped due to different hardware configuration. For example some tests are using Telemetry-Solution
ustress micro-benchmarks which are tuned for Neoverse platform(s) only.

## WindowsPerf Stress Testing Script

Having properly setup the Python unit tests using the instructions above all you need to do is to run:

```
> stress.ps1
```

The default number of cycles is 1. You can change that using the first command line argument, so if you want to run it 5 times just type:

```
> stress.ps1 5
```

A bunch of commands will be executed in succession trying to stress the driver, at the end of each cycle the Python tests are also executed so make sure they are properly configured.
