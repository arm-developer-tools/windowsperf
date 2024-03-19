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

""" Module testing basic output from `wperf stat -m <metric> ...
    Tests include "Telemetry Solution" metrics for WindowsPerf that should be
    available for `neoverse` CPUs.

    Telemetry Solution metrics which are available with `neoverse-` products. Please note we will:
    * SKIP whole `metric_ts` test module (file `wperf_cli_metrics_ts_test.py`) if host CPU is not `neoverse` family.
    * If it is `neoverse` CPU but metric(s) from test is not supported we will SKIP that test.
    * If it is `neoverse` CPU but metric's require more free GPSs than available, we will XFAIL.
"""

import json
import re
import pytest
from common import run_command
from common import wperf_metric_events, wperf_metric_events_list
from common import wperf_metric_is_available
from common import get_product_name, get_make_CPU_name
from fixtures import fixture_is_enough_GPCs

### Test cases

_product_name = get_product_name()
_product_name_cpus = get_make_CPU_name(_product_name)

# Skip whole module if Telemetry Solution metrics (only for `neoverse-` platforms) are not supported by this CPU
if not _product_name.startswith("neoverse-"):
    pytest.skip(f'unsupported configuration: CPU={_product_name_cpus} must support Telemetry Solution metrics' % (), allow_module_level=True)

@pytest.mark.parametrize("metric",
[
    ("backend_stalled_cycles"),
    ("branch_misprediction_ratio"),
    ("branch_mpki"),
    ("branch_percentage"),
    ("crypto_percentage"),
    ("dtlb_mpki"),
    ("dtlb_walk_ratio"),
    ("frontend_stalled_cycles"),
    ("integer_dp_percentage"),
    ("ipc"),
    ("itlb_mpki"),
    ("itlb_walk_ratio"),
    ("l1d_cache_miss_ratio"),
    ("l1d_cache_mpki"),
    ("l1d_tlb_miss_ratio"),
    ("l1d_tlb_mpki"),
    ("l1i_cache_miss_ratio"),
    ("l1i_cache_mpki"),
    ("l1i_tlb_miss_ratio"),
    ("l1i_tlb_mpki"),
    ("l2_cache_miss_ratio"),
    ("l2_cache_mpki"),
    ("l2_tlb_miss_ratio"),
    ("l2_tlb_mpki"),
    ("ll_cache_read_hit_ratio"),
    ("ll_cache_read_miss_ratio"),
    ("ll_cache_read_mpki"),
    ("load_percentage"),
    ("scalar_fp_percentage"),
    ("simd_percentage"),
    ("store_percentage"),

]
)
def test_metrics_telemetry_solution_metrics(metric):
    """ Run known TS metrics and check if defined events are present """
    if not wperf_metric_is_available(metric):
        pytest.skip(f"unsupported metric: {metric}")

    #
    # Assert that metric has events
    #
    events = wperf_metric_events(metric)
    assert events is not None, f"metric={metric} has no events!"

    #
    # Assert that metric has enough free GPC to run
    #
    events_cnt = len(wperf_metric_events_list(metric))
    if not fixture_is_enough_GPCs(events_cnt):
        pytest.xfail(f"this test requires at least {events_cnt} GPCs, metric={metric}")

    cmd = 'wperf stat -m ' + metric + ' -c 1 sleep 1'
    stdout, _ = run_command(cmd.split())

    assert b'Telemetry Solution Metrics:' in stdout

    # Event names in pretty table
    if events:
        for event in events.split(','):
            event = event.strip("{}/")   # Some events may be part of groups
            if "/" in event:
                event = event.split('/')[1]    # e.g. "/dsu/<event_name>" -> "<event_name>"
            assert re.search(b'[\\d]+[\\s]+%s[\\s]+0x[0-9a-f]+' % str.encode(event), stdout)

@pytest.mark.parametrize("metric",
[
    ("backend_stalled_cycles"),
    ("branch_misprediction_ratio"),
    ("branch_mpki"),
    ("branch_percentage"),
    ("crypto_percentage"),
    ("dtlb_mpki"),
    ("dtlb_walk_ratio"),
    ("frontend_stalled_cycles"),
    ("integer_dp_percentage"),
    ("ipc"),
    ("itlb_mpki"),
    ("itlb_walk_ratio"),
    ("l1d_cache_miss_ratio"),
    ("l1d_cache_mpki"),
    ("l1d_tlb_miss_ratio"),
    ("l1d_tlb_mpki"),
    ("l1i_cache_miss_ratio"),
    ("l1i_cache_mpki"),
    ("l1i_tlb_miss_ratio"),
    ("l1i_tlb_mpki"),
    ("l2_cache_miss_ratio"),
    ("l2_cache_mpki"),
    ("l2_tlb_miss_ratio"),
    ("l2_tlb_mpki"),
    ("ll_cache_read_hit_ratio"),
    ("ll_cache_read_miss_ratio"),
    ("ll_cache_read_mpki"),
    ("load_percentage"),
    ("scalar_fp_percentage"),
    ("simd_percentage"),
    ("store_percentage"),
]
)
def test_metrics_telemetry_solution_metrics_json(metric):
    """ Test if output JSON has TS data  """
    if not wperf_metric_is_available(metric):
        pytest.skip(f"unsupported metric: {metric}")
        return

    #
    # Assert that metric has events
    #
    events = wperf_metric_events(metric)
    assert events is not None, f"metric={metric} has no events!"

    #
    # Assert that metric has enough free GPC to run
    #
    events_cnt = len(wperf_metric_events_list(metric))
    if not fixture_is_enough_GPCs(events_cnt):
        pytest.xfail(f"this test requires at least {events_cnt} GPCs, metric={metric}")

    cmd = 'wperf stat -m ' + metric + ' --json -c 1 sleep 1'
    stdout, _ = run_command(cmd.split())
    json_output = json.loads(stdout)

    """
        "ts_metric": {
            "Telemetry_Solution_Metrics": [
                {
                    "core": "1",
                    "product_name": "neoverse-n1",
                    "metric_name": "l1d_cache_miss_ratio",
                    "value": "0.24",
                    "unit": "per cache access"
                }
            ]
        }
    """
    assert 'core' in json_output
    assert 'ts_metric' in json_output['core']
    assert 'Telemetry_Solution_Metrics' in json_output['core']['ts_metric']
    assert len(json_output['core']['ts_metric']['Telemetry_Solution_Metrics']) == 1
    assert 'metric_name' in json_output['core']['ts_metric']['Telemetry_Solution_Metrics'][0]
    assert metric in json_output['core']['ts_metric']['Telemetry_Solution_Metrics'][0]['metric_name']
