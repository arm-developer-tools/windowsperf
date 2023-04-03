# wperf functional testing

[[_TOC_]]

# Introduction

This set of Python scripts is using [pyest](https://docs.pytest.org/) library to drive functional testing of [wperf]{../../README.md}.

## Testing prerequisites

* You must have Python 3.11.1 (or newer) installed on your system and available on system environment PATH.
  * `pytest` module is also required, if missing please install it via `pip install pytest` command, see [pytest](https://pypi.org/project/pytest/).
  * All required Python modules by WindowsPerf functional testing are specified in [requirements.txt](../requirements.txt). Use the `pip install -r requirements.txt` command to install all of the Python modules and packages listed in `requirements.txt` file.
* Tests must be executed on ARM64 Windows On Arm machine.
* [wperf-driver](../../wperf-driver/README.md) must be installed on the system.
* [wperf](../../wperf/README.md) application must be present in the same directory as tests or be on system environment PATH.
* To test if `wperf` JSON output is valid test directory must contain `schemes/` sub-directory.
  * Note: if `schemas` directory is missing JSON output tests will fail (and not skipped).

## Testing directory structure

In below test directory we've added locally `wperf.exe`.

```
C:\example\of\functional\testing> tree /f
│   common.py
│   wperf_cli_common_test.py
│   wperf_cli_json_validator_test.py
│   wperf_cli_list_test.py
│   wperf_cli_metrics_test.py
│   wperf_cli_padding_test.py
│   wperf_cli_stat_test.py
│   wperf_cli_test_test.py
│   wperf.exe
│
└───schemas
        wperf.list.schema
        wperf.sample.schema
        wperf.stat.schema
        wperf.test.schema
        wperf.version.schema
```

### Test execution

In directory `C:\example\of\functional\testing` from above example type `pytest`:

```
>pytest
================================================= test session starts =================================================
platform win32 -- Python 3.11.1, pytest-7.2.0, pluggy-1.0.0
rootdir: C:\example\of\functional\testing
collected 71 items

wperf_cli_common_test.py ....                                                                                    [  5%]
wperf_cli_json_validator_test.py ....                                                                            [ 11%]
wperf_cli_list_test.py ...                                                                                       [ 15%]
wperf_cli_metrics_test.py ........                                                                               [ 26%]
wperf_cli_padding_test.py .........                                                                              [ 39%]
wperf_cli_stat_test.py .......................................                                                   [ 94%]
wperf_cli_test_test.py ....                                                                                      [100%]

=========================================== 71 passed in 162.97s (0:02:42) ============================================
```
