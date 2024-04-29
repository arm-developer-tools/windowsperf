#usr/bin/env python3

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

"""
This script runs Timeline feature tests with Ustress metrics.

Requires:
    pytest -    install with `pip install -U pytest`

Usage:
    >py.test wperf_cli_test.py

"""

import re
import pytest
from common import run_command
from common import wperf_metric_events, wperf_metric_is_available
from common import get_product_name, get_make_CPU_name, get_CPUs_supported_by_ustress
from common_ustress import TS_USTRESS_HEADER

# Skip whole module if ustress is not supported by this CPU
_cpus = get_CPUs_supported_by_ustress(TS_USTRESS_HEADER)
_product_name = get_product_name()
_product_name_cpus = get_make_CPU_name(_product_name)

if _product_name_cpus not in _cpus:
    pytest.skip(f'skipping as ustress do not support CPU={_product_name_cpus}' % (), allow_module_level=True)

#
# Test cases
#

@pytest.mark.parametrize("I,C,N,METRICS",
[
    (0, 0, 3, "l1d_cache_miss_ratio"),
    (0, 1, 2, "l1d_tlb_mpki"),
    (0, 2, 1, "l1d_cache_miss_ratio,l1d_tlb_mpki"),

    (1, 1, 2, "l1d_tlb_mpki"),
    (2, 2, 1, "l1d_cache_miss_ratio,l1d_tlb_mpki"),
    (3, 0, 3, "l1d_cache_miss_ratio"),
]
)
def test_wperf_timeline_ts_metrics(I, C, N, METRICS):
    """ Test timeline with TS metrics. """
    for metric in METRICS.split(","):
        if not wperf_metric_is_available(metric):
            pytest.skip("unsupported metric: " + metric)
            return

    cmd = f'wperf stat -m {METRICS} -t -i {I} -n {N} -c {C} sleep 1 -v'
    stdout, _ = run_command(cmd.split())

    assert b"Telemetry Solution Metrics" not in stdout

    COLS = int()    # How many columns are in this timeline (events + metrics)

    EVENTS = []
    for metric in METRICS.split(","):
        metric_events = wperf_metric_events(metric).strip("{}").split(",")
        EVENTS += metric_events
        COLS += len(metric_events)

    COLS += len(METRICS.split(","))

    assert b"timeline file: 'wperf_core_%s" % (str.encode(str(C))) in stdout    # e.g. timeline file: 'wperf_core_1_2023_06_29_09_09_05.core.csv'
    cvs_files = re.findall(rb'wperf_core_%s_[0-9_]+\.core\.csv' % (str.encode(str(C))), stdout)   # e.g. ['wperf_core_1_2023_06_29_09_09_05.core.csv']
    assert len(cvs_files) == 1

    expected_events_header = "cycle,"
    COLS += 1

    for event in EVENTS:
        expected_events_header += f"{event},"

    for metric in METRICS.split(","):
        expected_events_header += f"M@{metric},"   # Metrics start with "M@<metric_name>"

    expected_cores = (f"core {C},") * COLS

    with open(cvs_files[0], 'r') as file:
        cvs = file.read()

        assert expected_cores + '\n' in cvs            # E.g.  core 2,core 2,core 2,core 2,core 2,core 2,core 2,
        assert expected_events_header + '\n' in cvs    # E.g.  cycle,l1d_cache,l1d_cache_refill,inst_retired,l1d_tlb_refill,M@l1d_cache_miss_ratio,M@l1d_tlb_mpki,

        # Find lines with counter values, e.g.. 38111732,89739,61892,20932002,0.003,4.222
        pattern = r'^((\d*\.*\d+),){%s}$' % (COLS)
        assert len(re.findall(pattern, cvs, re.MULTILINE)) == N

@pytest.mark.parametrize("I,C,N,METRICS",
[
    (0, '1,2',     2, "l1d_cache_miss_ratio"),
    (0, '1,2',     3, "l1d_cache_miss_ratio,load_percentage"),
    (0, '1,2,3',   2, "l1d_tlb_mpki"),
    (0, '1,2,3',   3, "l1d_tlb_mpki,load_percentage"),
    (0, '1,3,5,7', 2, "l1d_cache_miss_ratio,l1d_tlb_mpki"),
    (0, '1,3,5,7', 3, "l1d_cache_miss_ratio,l1d_tlb_mpki,load_percentage"),

    (1, '1,2',     2, "l1d_cache_miss_ratio"),
    (2, '1,2',     3, "l1d_cache_miss_ratio,load_percentage"),
    (3, '1,2,3',   2, "l1d_tlb_mpki"),
    (1, '1,2,3',   3, "l1d_tlb_mpki,load_percentage"),
    (2, '1,3,5,7', 2, "l1d_cache_miss_ratio,l1d_tlb_mpki"),
    (3, '1,3,5,7', 3, "l1d_cache_miss_ratio,l1d_tlb_mpki,load_percentage"),
]
)
def test_wperf_timeline_ts_metrics_many_cores(I, C, N, METRICS):
    """ Test timeline with TS metrics. Multiple cores variant. """
    for metric in METRICS.split(","):
        if not wperf_metric_is_available(metric):
            pytest.skip("unsupported metric: " + metric)
            return

    cmd = f'wperf stat -m {METRICS} -t -i {I} -n {N} -c {C} --timeout 1 -v'
    stdout, _ = run_command(cmd.split())

    cores = C.split(',')

    assert len(cores) > 1   # Sanity check as we test for list of cores > 1

    assert b"timeline file: 'wperf_system_side_" in stdout    # e.g. timeline file: 'wperf_system_side_2023_09_04_04_00_42.core.csv'
    cvs_files = re.findall(rb'wperf_system_side_[0-9_]+\.core\.csv', stdout)   # e.g. ['wperf_system_side_2023_09_04_04_00_42.core.csv']
    assert len(cvs_files) == 1

    ### We will check here ONLY for columns at the end holding metric names and cores for them

    """ Example:
    >wperf stat .... -t .... -c 1,2 .... -m l1d_cache_miss_ratio,load_percentage ....

    <CSV header>

    .......  ,core 1                     ,core 1              ,core 2                  ,core 2,                  <expected_metric_cores>
    .......  ,M@l1d_cache_miss_ratio     ,M@load_percentage   ,M@l1d_cache_miss_ratio  ,M@load_percentage,       <expected_metric_header>
    .......  ,0.019                      ,21.609              ,0.013                   ,22.499,
    .......  ,0.007                      ,22.262              ,0.042                   ,21.241,
    .......  ,0.008                      ,21.708              ,0.012                   ,22.943,
    """

    # Sanity checks for metric names at the end of the rows
    expected_metric_cores = str()

    for c in cores:
        for _ in METRICS.split(","):
            expected_metric_cores += f"core {c},"          #   core 1,core 1,core 2,core 2,

    expected_metric_header = str()

    for _ in cores:
        for metric in METRICS.split(","):
            expected_metric_header += f"M@{metric},"   # M@l1d_cache_miss_ratio,M@load_percentage,M@l1d_cache_miss_ratio,M@load_percentage,

    ### Check for complete list of events including metrics
    EVENTS = []
    for _ in cores:
        EVENTS.append("cycle")  # Each core includes cycles (first fixed counter)
        for metric in METRICS.split(","):
            metric_events = wperf_metric_events(metric).strip("{}").split(",")
            EVENTS += metric_events

    expected_events_header = str()
    for event in EVENTS:
        expected_events_header += f"{event},"
    expected_events_header += expected_metric_header

    with open(cvs_files[0], 'r') as file:
        csv = file.read()

        # Check for line endings
        assert expected_metric_cores + '\n' in csv
        assert expected_metric_header + '\n' in csv

        # Check for whole events header including metric events and metric names
        assert expected_events_header + '\n' in csv
