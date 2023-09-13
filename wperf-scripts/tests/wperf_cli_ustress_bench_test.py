#!/usr/bin/env python3

# BSD 3-Clause License
#
# Copyright (c) 2022, Arm Limited
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

This is ustress test bench build script. It contains all functions needed to:
* Check for basic dependencies and build dependencies.
* Cleanup telemetry-solution/tools/ustress build directory with `make clean`
* Build `ustress` with `make CPU=` based on host product_name.
  * Note: This tests will not build on machines which do not support `product_name`
    detection. E.g. generic armv8-a machines.

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

>tr (GNU coreutils) 8.32
Copyright (C) 2020 Free Software Foundation, Inc.
License GPLv3+: GNU GPL version 3 or later <https://gnu.org/licenses/gpl.html>.
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.

Written by Jim Meyering.

>clang --version
clang version 16.0.5
Target: aarch64-pc-windows-msvc
Thread model: posix
InstalledDir: C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\Llvm\ARM64\bin


"""

import csv
import os
import re
from statistics import median
from common import run_command
from common import wperf_test_no_params, get_result_from_test_results
from common import get_product_name, get_make_CPU_name, get_CPUs_supported_by_ustress

import pytest

N_CORES = os.cpu_count()

# Where we keep Telemetry Solution code
TS_USTRESS_DIR = "telemetry-solution/tools/ustress"
# Header with supported by `ustress` CPUs:
TS_USTRESS_HEADER = os.path.join(TS_USTRESS_DIR, "cpuinfo.h")

### Test cases

# Skip whole module if ustress is not supported by this CPU
_cpus = get_CPUs_supported_by_ustress(TS_USTRESS_HEADER)
_product_name = get_product_name()
_product_name_cpus = get_make_CPU_name(_product_name)

if _product_name_cpus not in _cpus:
    pytest.skip(f'skipping as ustress do not support CPU={_product_name_cpus}' % (), allow_module_level=True)


def test_ustress_bench_compatibility_tests():
    """ Check if this bench can run on HW we are currently on. """

    product_name = str()
    cpus = []

    ### Check if ustress is present
    assert os.path.isdir(TS_USTRESS_DIR)

    ### Check type of CPU is supported and get CPU list from header file
    cpus = get_CPUs_supported_by_ustress(TS_USTRESS_HEADER)
    assert len(cpus) > 0

    ### Check product name of HW we are now on, it must be supported by ustress
    ### as we build ustress with certain cpuinfo.h configs.
    product_name = get_product_name()    # e.g. "armv8-a" or "neoverse-n1"
    assert len(product_name) > 0

    ### Check if HW we are currently on is supported by ustress

    # Promote PRODUCT_NAME to CPUS[n] format
    # Note: Makefile accepts both NEOVERSE_N1 and NEOVERSE-N1 format (it replaces '-' with '_')
    product_name_cpus = get_make_CPU_name(product_name)
    assert product_name_cpus in cpus

@pytest.mark.parametrize("cmd,expected,msg",
[
    ('make',    b'GNU Make 3',              'GNU `make` not installed, or not in the PATH'),   # E.g. GNU Make 3.81
    ('tr',      b'tr (GNU coreutils)',      'GNU `tr` not installed, or not in the PATH'),    # E.g. tr (GNU coreutils) 8.32
    ('clang',   b'aarch64-pc-windows-msvc', '`clang` targeting `aarch64-pc-windows-msvc` not installed, or not in the PATH'),
]
)
def test_ustress_bench_build_ustress__deps(cmd,expected,msg):
    """ Check ustress build dependencies. """

    ### Check if GNU Make is installed
    stdout, _ = run_command(f"{cmd} --version")
    assert expected in stdout, msg

def test_ustress_bench_build_ustress__make_clean():
    """ ustress build cleanup. """

    #### Run make clean in ustress build directory
    stdout, _ = run_command("make clean", TS_USTRESS_DIR)
    assert b'rm -f' in stdout   # Sanity check


def test_ustress_bench_build_ustress__make_cpu():
    """ Build ustress with clang. """

    ### Build ustress for this platform
    product_name = get_product_name()
    CPU = get_make_CPU_name(product_name)
    stdout, _ = run_command(f"make CPU={CPU}", TS_USTRESS_DIR)   # Build all tests

    # Build sanity checks, e.g.:
    # clang -std=c11 -O2 -g -Wall -pedantic -DCPU_NEOVERSE_N1  --target=arm64-pc-windows-msvc -o branch_direct_workload.exe branch_direct_workload.c
    targets = stdout.count(b'target=arm64-pc-windows-msvc')
    dcpus = stdout.count(b'DCPU_%b' % (str.encode(CPU)))
    assert targets > 0
    assert dcpus > 0
    assert targets == dcpus


################################################################################################
#      Below are micro-benchmarks which we will execute and check timeline (metric output)
################################################################################################

def get_metric_values(cvs_file_path, metric):
    """ Return list of values from timeline CVS file (for one core only). """
    result = []
    metric_column = "M@" + metric   # This is how we encode metric column in CVS timeline file

    with open(cvs_file_path, newline='') as csvfile:
        spamreader = csv.reader(csvfile, delimiter=',', quotechar='|')

        """ Example 'row'(s):
            ['core', '1,core', '1,core', '1,core', '1,']
            ['cycle,l1d_cache,l1d_cache_refill,M@l1d_cache_miss_ratio,']
            ['163757124,46340967,2007747,0.043,']
            ['77863232,31287908,780320,0.025,']
            ['34097420,8487530,356686,0.042,']
            ['41752921,9459244,411182,0.043,']
            ['54506416,17393294,576923,0.033,']
        """
        metric_col_index = -1       # Nothing found yet ( < 0 )
        for row in spamreader:
            if metric_col_index >= 0 and len(row) > metric_col_index:
                val = float(row[metric_col_index])
                result.append(val)

            # Find row with event names and metric(s) names
            # Below you will find rows with event count and metric values
            if metric_column in row:
                metric_col_index = row.index(metric_column)

    return result

@pytest.mark.parametrize("core,N,I,metric,benchmark,param,threshold ",
[
    (7, 5, 1, "l1d_cache_miss_ratio", "l1d_cache_workload.exe", 10, 0.91),
]
)
def test_ustress_execute_micro_benchmark(core,N,I,metric,benchmark,param,threshold):
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

    ## Execute benchmark
    benchmark_path = os.path.join(TS_USTRESS_DIR, benchmark)
    stdout, _ = run_command(f"wperf stat -v -c {core} -m {metric} -t -n {N} -i {I} --timeout 1 {benchmark_path} {param}")

    # Get timeline CVS filename from stdout (we get this with `-v`)
    cvs_files = re.findall(rb'wperf_core_%s_[0-9_]+\.core\.csv' % (str.encode(str(core))), stdout)   # e.g. ['wperf_core_1_2023_06_29_09_09_05.core.csv']
    assert len(cvs_files) == 1

    metric_values = get_metric_values(cvs_files[0], metric)
    med = median(metric_values)

    assert len(metric_values) == N      # We should get <N> rows in CSV file with metric values

    if not med >= threshold:
        pytest.skip(f"{benchmark} metric '{metric}' median {med} < {threshold} -- threshold not reached")

    assert med >= threshold             # Check if median is above threshold
