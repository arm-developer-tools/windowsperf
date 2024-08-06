#!/usr/bin/env python3

# BSD 3-Clause License
#
# Copyright (c) 2024, Arm Limited
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this
#    list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its
#    contributors may be used to endorse or promote products derived from
#    this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

r"""
# Description

This is wperf sampling regression test script.
This script depends on actions in `wperf_cli_ustress_bench_test.py`:
* Checks if `ustress` is available.
* Builds `ustress` that will be used by this script.

Disclaimer: if you want to use this script to run tests build `ustress` manually or
            execute `wperf_cli_ustress_bench_test.py` first.
            All tests in this module (file) will be skipped if `ustress` binaries are
            not available.

Note: `wperf_cli_ustress_bench_test.py` should be executed first to build `ustress`.

# Build dependencies

* MSVC cross/native arm64 build environment, see `vcvarsall.bat`.
* GNU Make 3.x - ustress Makefile requires it. Download "complete package"
  here: https://gnuwin32.sourceforge.net/packages/make.htm
* GNU tr - ustress Makefile internals requires it. Download "complete package"
  here: https://gnuwin32.sourceforge.net/packages/coreutils.htm
* clang targeting `aarch64-pc-windows-msvc`.
  * Go to MSVC installer and install: Modify -> Individual Components -> search "clang".
  * install: "C++ Clang Compiler..." and "MSBuild support for LLVM..."

---

See configuration needed to build those tests:
> %comspec% /k "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" arm64
**********************************************************************
** Visual Studio 2022 Developer Command Prompt v17.7.1
** Copyright (c) 2022 Microsoft Corporation
**********************************************************************
[vcvarsall.bat] Environment initialized for: 'arm64'

>make --version
GNU Make 3.81
Copyright (C) 2006  Free Software Foundation, Inc.
This is free software; see the source for copying conditions.
There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.

This program built for i386-pc-mingw32

>clang --version
clang version 16.0.5
Target: aarch64-pc-windows-msvc
Thread model: posix
InstalledDir: C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\Llvm\ARM64\bin


"""

import json
import os
import pathlib as pl
import re
from statistics import median
from time import sleep

import pytest

from common import run_command
from common import get_product_name, get_make_CPU_name, get_CPUs_supported_by_ustress
from common_ustress import TS_USTRESS_DIR, TS_USTRESS_HEADER
from common_ustress import get_metric_values


### Test cases

# Skip whole module if ustress is not supported by this CPU
_cpus = get_CPUs_supported_by_ustress(TS_USTRESS_HEADER)
_product_name = get_product_name()
_product_name_cpus = get_make_CPU_name(_product_name)

if _product_name_cpus not in _cpus:
    pytest.skip(f'unsupported configuration: ustress do not support CPU={_product_name_cpus}' % (), allow_module_level=True)

if not pl.Path("wperf.exe").is_file():
    pytest.skip("Can not find wperf.exe", allow_module_level=True)


################################################################################################
#      Below are micro-benchmarks which we will execute and check timeline (metric output)
################################################################################################


@pytest.mark.parametrize("core,N,I,metric,benchmark,param,threshold ",
[
    (4, 5, 1, "scalar_fp_percentage", "fpdiv_workload.exe",     10, 0.91),
    (5, 5, 1, "scalar_fp_percentage", "fpmac_workload.exe",     10, 0.91),
    (6, 5, 1, "scalar_fp_percentage", "fpmul_workload.exe",     10, 0.91),
    (5, 5, 1, "scalar_fp_percentage", "fpsqrt_workload.exe",    10, 0.91),

    (4, 5, 1, "l1d_cache_miss_ratio", "l1d_cache_workload.exe", 10, 0.91),
]
)
def test_ustress_bench_execute_micro_benchmark(core,N,I,metric,benchmark,param,threshold):
    r""" Execute 'telemetry-solution\tools\ustress\<NAME> <PARAM>' and measure timeline's <METRIC>.
        Note: This function only works for CSV timeline files with ONE metric calculated (on one core)

        <CORE>      - CPU number to count on (and spawn micro-benchmark process)
        <N>         - how many times count in timeline
        <I>         - interval between counts (in seconds)
        <METRIC>    - name of metric to check (and read from CSV file)
        <BENCHMARK> - micro-benchmark to execute
        <PARAM>     - micro-benchmark command line parameter, in ustress case a benchmark execution in seconds (approx.)
        <THRESHOLD> - median of all metric measurements must be above this value or test fails
    """

    ## Do not rely on other tests, sleep before we run record to make sure core(s) counters are not saturated
    sleep(2)

    ## Execute benchmark
    benchmark_path = os.path.join(TS_USTRESS_DIR, benchmark)
    cmd = f"wperf stat -v -c {core} -m {metric} -t -n {N} -i {I} --timeout 1 -- {benchmark_path} {param}"
    stdout, _ = run_command(cmd)

    # Get timeline CSV filename from stdout (we get this with `-v`)
    cvs_files = re.findall(rb'wperf_core_%s_[0-9_]+\.core\.csv' % (str.encode(str(core))), stdout)   # e.g. ['wperf_core_1_2023_06_29_09_09_05.core.csv']
    assert len(cvs_files) == 1, f"in {cmd}"

    metric_values = get_metric_values(cvs_files[0], metric)
    assert len(metric_values) > 0, f"in {cmd}"

    med = median(metric_values)
    assert len(metric_values) == N, f"in {cmd}"      # We should get <N> rows in CSV file with metric values

    if not med >= threshold:
        pytest.skip(f"{benchmark} metric '{metric}' median {med} < {threshold} -- threshold not reached")

    assert med >= threshold, f"in {cmd}"             # Check if median is above threshold

def test_ustress_bench_2_orthogonal_metrics_l1d_cache_workload():
    """ Run `l1d_cache_workload.exe` for two orthogonal metrics, expect:
        - `l1d_cache_miss_ratio` to be ~1.0, and
        - `scalar_fp_percentage` to be ~0.0.

        We want to make sure that in scenario with one micro-benchmark we
        can clearly see two unrelated metrics are consistently "different".
    """

    ## Do not rely on other tests, sleep before we run record to make sure core(s) counters are not saturated
    sleep(2)

    metrics = "scalar_fp_percentage,l1d_cache_miss_ratio"
    benchmark = "l1d_cache_workload.exe"
    benchmark_path = os.path.join(TS_USTRESS_DIR, benchmark)

    scalar_fp_percentage_vals = []
    l1d_cache_miss_ratio_vals = []

    for _ in range(3):
        cmd = f"wperf stat -m {metrics}  -c 7 --timeout 5 --json -- {benchmark_path} 100"
        stdout, _ = run_command(cmd)
        json_output = json.loads(stdout)

        """Expected:
            ...
            Telemetry Solution Metrics:
            core  product_name  metric_name           value  unit
            ====  ============  ===========           =====  ====
               7  neoverse-n1   l1d_cache_miss_ratio  1.000  per cache access
               7  neoverse-n1   scalar_fp_percentage  0.000  percent of operations
        """

        assert 'core' in json_output
        assert 'ts_metric' in json_output['core']
        assert 'Telemetry_Solution_Metrics' in json_output['core']['ts_metric']
        assert len(json_output['core']['ts_metric']['Telemetry_Solution_Metrics']) == len(metrics.split(","))

        l1d_cache_miss_ratio = None
        scalar_fp_percentage = None

        for metric in json_output['core']['ts_metric']['Telemetry_Solution_Metrics']:
            if metric["metric_name"] == 'l1d_cache_miss_ratio':
                l1d_cache_miss_ratio = float(metric["value"])
            if metric["metric_name"] == 'scalar_fp_percentage':
                scalar_fp_percentage = float(metric["value"])

        assert l1d_cache_miss_ratio is not None
        assert scalar_fp_percentage is not None
        l1d_cache_miss_ratio_vals.append(l1d_cache_miss_ratio)
        scalar_fp_percentage_vals.append(scalar_fp_percentage)

    assert median(l1d_cache_miss_ratio_vals) >= 0.90, f"expected `l1d_cache_miss_ratio` == ~1.000, for workload {benchmark}"
    assert median(scalar_fp_percentage_vals) <= 0.10, f"expected ` scalar_fp_percentage` ==  ~0.000, for workload {benchmark}"

def test_ustress_bench_2_orthogonal_metrics_fpmul_workload():
    """ Run `fpmul_workload.exe` for two orthogonal metrics, expect:
        - `l1d_cache_miss_ratio` to be ~0.0, and
        - `scalar_fp_percentage` to be > 50.0 %.

        We want to make sure that in scenario with one micro-benchmark we
        can clearly see two unrelated metrics are consistently "different".
    """

    ## Do not rely on other tests, sleep before we run record to make sure core(s) counters are not saturated
    sleep(2)

    metrics = "scalar_fp_percentage,l1d_cache_miss_ratio"
    benchmark = "fpmul_workload.exe"
    benchmark_path = os.path.join(TS_USTRESS_DIR, benchmark)

    scalar_fp_percentage_vals = []
    l1d_cache_miss_ratio_vals = []

    for _ in range(3):
        cmd = f"wperf stat -m {metrics}  -c 7 --timeout 5 --json -- {benchmark_path} 100"
        stdout, _ = run_command(cmd)
        json_output = json.loads(stdout)

        """Expected on `neoverse-n1`:
            ...
            Telemetry Solution Metrics:
            core  product_name  metric_name           value  unit
            ====  ============  ===========           =====  ====
               7  neoverse-n1   l1d_cache_miss_ratio   0.000  per cache access
               7  neoverse-n1   scalar_fp_percentage  57.143  percent of operations
        """

        assert 'core' in json_output
        assert 'ts_metric' in json_output['core']
        assert 'Telemetry_Solution_Metrics' in json_output['core']['ts_metric']
        assert len(json_output['core']['ts_metric']['Telemetry_Solution_Metrics']) == len(metrics.split(","))

        l1d_cache_miss_ratio = None
        scalar_fp_percentage = None

        for metric in json_output['core']['ts_metric']['Telemetry_Solution_Metrics']:
            if metric["metric_name"] == 'l1d_cache_miss_ratio':
                l1d_cache_miss_ratio = float(metric["value"])
            if metric["metric_name"] == 'scalar_fp_percentage':
                scalar_fp_percentage = float(metric["value"])

        assert l1d_cache_miss_ratio is not None
        assert scalar_fp_percentage is not None
        l1d_cache_miss_ratio_vals.append(l1d_cache_miss_ratio)
        scalar_fp_percentage_vals.append(scalar_fp_percentage)

    assert median(l1d_cache_miss_ratio_vals) <= 0.10, f"expected `l1d_cache_miss_ratio` == ~0.000, for workload {benchmark}"
    assert median(scalar_fp_percentage_vals) >= 50.0, f"expected ` scalar_fp_percentage`> ~50.000, for workload {benchmark}"
