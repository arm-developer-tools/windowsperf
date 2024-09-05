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
Tests checking if `wperf stat` for `ustress` micro-benchmarks is returning correct values.
"""

import os
import json
from statistics import median
from time import sleep
import pytest
from common import run_command, is_json
from common import get_product_name, get_make_CPU_name, get_CPUs_supported_by_ustress
from common_ustress import TS_USTRESS_DIR, TS_USTRESS_HEADER


### Test cases

# Skip whole module if ustress is not supported by this CPU
_cpus = get_CPUs_supported_by_ustress(TS_USTRESS_HEADER)
_product_name = get_product_name()
_product_name_cpus = get_make_CPU_name(_product_name)

if _product_name_cpus not in _cpus:
    pytest.skip(f'unsupported configuration: ustress do not support CPU={_product_name_cpus}' % (), allow_module_level=True)


@pytest.mark.parametrize("benchmark,metrics",
[
    ("branch_direct_workload.exe",      dict({"integer_dp_percentage" : 90.0})),
    ("branch_indirect_workload.exe",    dict({"integer_dp_percentage" : 33.0, "load_percentage" : 33.0})),
    ("mul64_workload.exe",              dict({"integer_dp_percentage" : 70.0, "branch_percentage" : 20.0})),
    ("mul64_workload.exe",              dict({"integer_dp_percentage" : 70.0, "branch_percentage" : 20.0})),
    ("fpmul_workload.exe",              dict({"integer_dp_percentage" : 20.0, "scalar_fp_percentage" : 50.0})),
    ("int2double_workload.exe",         dict({"integer_dp_percentage" : 30.0, "scalar_fp_percentage" : 40.0})),
    ("store_buffer_full_workload.exe",  dict({"integer_dp_percentage" : 33.0, "branch_percentage" : 20.0, "store_percentage" : 20.0})),
    ("l1d_cache_workload.exe",          dict({"integer_dp_percentage" : 30.0, "branch_percentage" : 30.0, "load_percentage" : 30.0})),
]
)
def test_ustress_bench_stat_operation_mix_profile(benchmark, metrics):
    #
    # Execute benchmark
    #
    benchmark_path = os.path.join(TS_USTRESS_DIR, benchmark)

    cmd = f"wperf stat -m Operation_Mix -c 7 --json --timeout 5 -- {benchmark_path} 1000"
    stdout, _ = run_command(cmd)

    assert is_json(stdout), f"in {cmd}"
    json_output = json.loads(stdout)

    assert 'core' in json_output
    assert 'ts_metric' in json_output['core']
    assert 'Telemetry_Solution_Metrics' in json_output['core']['ts_metric']

    def metric_in_set(metric_name):
        for metric in json_output['core']['ts_metric']['Telemetry_Solution_Metrics']:
            if metric["metric_name"] == metric_name:
                return True
        return False

    for k, v in metrics.items():
        # Check if metric we want to compare is in
        assert metric_in_set(k)
        # Check if metric value is at least `v`
        for metric in json_output['core']['ts_metric']['Telemetry_Solution_Metrics']:
            if metric["metric_name"] == k:
                value = float(metric["value"])
                assert value >= v, f"{benchmark} -> {k}, {value} >= {v} failed"
