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

import os
import json
from common import run_command, is_json
from common import get_product_name, get_make_CPU_name, get_CPUs_supported_by_ustress

import pytest

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


@pytest.mark.parametrize("core,event,event_freq,benchmark,param,hottest,hottest_overhead",
[
    (7, "l1d_cache_refill", 10000, "l1d_cache_workload.exe", 5, "stress", 99),
]
)
def test_ustress_record_microbenchmark(core,event,event_freq,benchmark,param,hottest,hottest_overhead):
    r""" Execute 'telemetry-solution\tools\ustress\<NAME> <PARAM>' and run sampling.
         Process JSON output to determine sampling accuracy.

        <CORE>              - CPU number to count on (and spawn micro-benchmark process)
        <EVENT>             - name of event to sample
        <EVENT_FREQ>        - event sample frequency (in Hz)
        <BENCHMARK>         - micro-benchmark to execute
        <PARAM>             - micro-benchmark command line parameter, in ustress case a benchmark execution in seconds (approx.)
        <HOTTEST>           - we expect sampling to determine that this was "hottest" function
        <HOTTEST_OVERHEAD>  - we expect sampling overhead for <HOTTEST> to be at least this big
    """

    ## Execute benchmark
    benchmark_path = os.path.join(TS_USTRESS_DIR, benchmark)
    stdout, _ = run_command(f"wperf record -e {event}:{event_freq} -c {core} --timeout 4 --json {benchmark_path} {param}")

    assert is_json(stdout)
    json_output = json.loads(stdout)

    r"""
    {
        "sampling": {
            "pe_file": "telemetry-solution/tools/ustress/l1d_cache_workload.exe",
            "pdb_file": "telemetry-solution/tools/ustress/l1d_cache_workload.pdb",
            "sample_display_row": 50,
            "samples_generated": 129,
            "samples_dropped": 1,
            "base_address": 140700047330040,
            "runtime_delta": 140694678601728,
            "events": [
                {
                    "type": "l1d_cache_refill",
                    "samples": [
                        {
                            "overhead": 100,
                            "count": 128,
                            "symbol": "stress"
                        }
                    ],
                    "interval": 10000,
                    "printed_sample_num": 1,
                    "annotate": []
                }
            ]
        }
    }
    """

    assert json_output["sampling"]["pe_file"].endswith(benchmark)
    assert json_output["sampling"]["pdb_file"].endswith(benchmark.replace(".exe", ".pdb"))

    assert "events" in json_output["sampling"]
    assert len(json_output["sampling"]["events"]) > 0

    # Check if event we sample for is in "events"
    hotest_symbol = json_output["sampling"]["events"][0]
    assert hotest_symbol["type"] == event
    assert len(hotest_symbol["samples"]) > 0
    assert hotest_symbol["interval"] == event_freq

    # We expect in events.samples[0] hottest sample (which we want to check for)
    hotest_symbol = json_output["sampling"]["events"][0]["samples"][0]
    symbol_overhead = hotest_symbol["overhead"]
    symbol_count = hotest_symbol["count"]
    symbol_name = hotest_symbol["symbol"]

    if not symbol_name == hottest:
        pytest.skip(f"{benchmark} hottest function sampled: '{symbol_name}' count={symbol_count} overhead={symbol_overhead}, expected '{hottest}' -- sampling mismatch")

    assert symbol_name == hottest
    assert symbol_count > 0
    assert symbol_overhead >= hottest_overhead
