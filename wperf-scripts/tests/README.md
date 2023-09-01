# wperf functional testing

[[_TOC_]]

# Introduction

This set of Python scripts is using [pytest](https://docs.pytest.org/) library to drive functional testing of [wperf](../../README.md).

## Testing prerequisites

* You must have Python 3.11.1 (or newer) installed on your system and available on system environment PATH.
  * `pytest` module is also required, if missing please install it via `pip install pytest` command, see [pytest](https://pypi.org/project/pytest/).
  * All required Python modules by WindowsPerf functional testing are specified in [requirements.txt](../requirements.txt). Use the `pip install -r requirements.txt` command to install all of the Python modules and packages listed in `requirements.txt` file.
* Tests must be executed on ARM64 Windows On Arm machine.
* [wperf-driver](../../wperf-driver/README.md) must be installed on the system.
* [wperf](../../wperf/README.md) application must be present in the same directory as tests or be on system environment PATH.
* To test if `wperf` JSON output is valid test directory must contain `schemes/` sub-directory.
  * Note: if `schemas` directory is missing JSON output tests will fail (and not skipped).
* Make sure that `windowsperf\wperf-scripts\tests\telemetry-solution\` sub-module is pulled with git.

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
collected 140 items

wperf_cli_common_test.py ....                                                [  2%]
wperf_cli_config_test.py .....                                               [  6%]
wperf_cli_extra_events_test.py ....                                          [  9%]
wperf_cli_info_str_test.py .                                                 [ 10%]
wperf_cli_json_validator_test.py ....                                        [ 12%]
wperf_cli_list_test.py .....                                                 [ 16%]
wperf_cli_metrics_test.py ................                                   [ 27%]
wperf_cli_padding_test.py ...........                                        [ 35%]
wperf_cli_record_test.py ..............                                      [ 45%]
wperf_cli_stat_test.py ...................................................   [ 82%]
wperf_cli_test_test.py .....                                                 [ 85%]
wperf_cli_timeline_test.py ..............                                    [ 95%]
wperf_cli_ustress_bench_test.py .....                                        [ 99%]
wperf_lib_app_test.py s                                                      [100%]

============================= short test summary info =============================
SKIPPED [1] wperf_lib_app_test.py:44: Can not run wperf-lib-app.exe
================= 139 passed, 1 skipped in 236.35s (0:03:56) ======================
```
