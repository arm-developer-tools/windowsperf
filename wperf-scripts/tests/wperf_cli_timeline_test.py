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

"""
This script runs simple wperf CLI tests.

Requires:
    pytest -    install with `pip install -U pytest`

Usage:
    >py.test wperf_cli_test.py

"""

import os
import re
from common import run_command
from common import get_result_from_test_results
from common import wperf_test_no_params
from common import wperf_metric_events, wperf_metric_is_available


import pytest

### Test cases

def test_wperf_timeline_core_n3():
    """ Test timeline (one core) output.  """
    cmd = 'wperf stat -m imix -c 1 -t -i 1 -n 3 sleep 1'
    stdout, _ = run_command(cmd.split())

    assert stdout.count(b"counting ...") == 3
    assert stdout.count(b"sleeping ...") == 3

def test_wperf_timeline_system_n3():
    """ Test timeline (system == all cores) output.  """
    cmd = 'wperf stat -m imix -t -i 1 -n 3 sleep 1'
    stdout, _ = run_command(cmd.split())

    assert stdout.count(b"counting ...") == 3
    assert stdout.count(b"sleeping ...") == 3

@pytest.mark.parametrize("C, N, SLEEP",
[
    (0,3,1),
    (1,5,2),
]
)
def test_wperf_timeline_core_n_file_output(C, N, SLEEP):
    """ Test timeline (core X) file format output.  """
    cmd = f'wperf stat -m imix -c {C} -t -i 1 -n {N} -v sleep {SLEEP}'
    stdout, _ = run_command(cmd.split())

    json_output = wperf_test_no_params()      # get output from `wperf test`
    gpc_num = get_result_from_test_results(json_output, "PMU_CTL_QUERY_HW_CFG [gpc_num]")
    gpc_num = int(gpc_num, 16)  # it's a hex string e,g,. 0x0005

    assert stdout.count(b"counting ...") == N
    assert stdout.count(b"sleeping ...") == N

    # Test for timeline file content
    assert b"timeline file: 'wperf_core_%s" % (str.encode(str(C))) in stdout    # e.g. timeline file: 'wperf_core_1_2023_06_29_09_09_05.core.csv'

    cvs_files = re.findall(rb'wperf_core_%s_[0-9_]+\.core\.csv' % (str.encode(str(C))), stdout)   # e.g. ['wperf_core_1_2023_06_29_09_09_05.core.csv']
    assert len(cvs_files) == 1

    with open(cvs_files[0], 'r') as file:
        cvs = file.read()

        assert cvs.count("Multiplexing,FALSE") == 1
        assert cvs.count("Kernel mode,FALSE") == 1
        assert cvs.count("Count interval,1.00") == 1
        assert cvs.count("Event class,core") == 1
        assert cvs.count("cycle,inst_spec,dp_spec,vfp_spec,") == 1  # This should be checked dynamically
        assert cvs.count(f"core {C},") == gpc_num + 1  # +1 for cycle fixed counter

        # Find lines with counter values, e.g.. 80182394,86203106,38111732,89739,61892,20932002,
        pattern = r'([0-9]+,){%s}\n' % (gpc_num + 1)
        assert len(re.findall(pattern, cvs, re.DOTALL)) == N

@pytest.mark.parametrize("N, SLEEP",
[
    (2,1),
    (4,2),
]
)
def test_wperf_timeline_system_n_file_output(N, SLEEP):
    """ Test timeline (system - all cores) file format output.  """
    cmd = f'wperf stat -m imix -t -i 1 -n {N} -v sleep {SLEEP}'
    stdout, _ = run_command(cmd.split())

    json_output = wperf_test_no_params()      # get output from `wperf test`
    gpc_num = get_result_from_test_results(json_output, "PMU_CTL_QUERY_HW_CFG [gpc_num]")
    gpc_num = int(gpc_num, 16)  # it's a hex string e,g,. 0x0005

    N_CORES = os.cpu_count()

    assert stdout.count(b"counting ...") == N
    assert stdout.count(b"sleeping ...") == N

    # Test for timeline file content
    assert b"timeline file: 'wperf_system_side_" in stdout    # e.g. timeline file: 'wperf_system_side_2023_06_29_10_05_27.core.csv'

    cvs_files = re.findall(rb'wperf_system_side_[0-9_]+\.core\.csv', stdout)   # e.g. ['wperf_system_side_2023_06_29_10_05_27.core.csv']
    assert len(cvs_files) == 1

    with open(cvs_files[0], 'r') as file:
        cvs = file.read()

        assert cvs.count("Multiplexing,FALSE") == 1
        assert cvs.count("Kernel mode,FALSE") == 1
        assert cvs.count("Count interval,1.00") == 1
        assert cvs.count("Event class,core") == 1
        assert cvs.count("cycle,inst_spec,dp_spec,vfp_spec,") == N_CORES  # This should be checked dynamically

        cores_str = str()
        for C in range(0, N_CORES):
            for _ in range(0, gpc_num + 1):
                cores_str += f"core {C},"

        assert cvs.count(cores_str) == 1

        # Find lines with counter values, e.g.. 80182394,86203106,38111732,89739,61892,20932002,
        pattern = r'([0-9]+,){%s}\n' % (gpc_num + 1)
        assert len(re.findall(pattern, cvs, re.DOTALL)) == N

@pytest.mark.parametrize("N,SLEEP,KERNEL_MODE,EVENTS",
[
    (6, 1, True, 'l1i_cache,l1i_cache_refill,l2i_cache,l2i_cache_refill,inst_retired,inst_spec,dp_spec,vfp_spec'),
    (5, 1, False, 'l1i_cache,l1i_cache_refill,l2i_cache,l2i_cache_refill,inst_retired,inst_spec,dp_spec,vfp_spec,ase_spec,ld_spec,st_spec'),
    (4, 1, True, 'l1i_cache,l1i_cache_refill,l2i_cache,l2i_cache_refill,inst_retired,inst_spec,dp_spec,vfp_spec,ase_spec,ld_spec,st_spec,l1i_tlb'),
    (3, 1, False, 'l1i_cache,l1i_cache_refill,l2i_cache,l2i_cache_refill,inst_retired,inst_spec,dp_spec,vfp_spec,ase_spec,ld_spec,st_spec,l1i_tlb,l1i_tlb_refill,l2i_tlb,l2i_tlb_refill'),
    (2, 1, True, 'l1i_cache,l1i_cache_refill,l2i_cache,l2i_cache_refill,inst_retired,inst_spec,dp_spec,vfp_spec,ase_spec,ld_spec,st_spec,l1i_tlb,l1i_tlb_refill,l2i_tlb,l2i_tlb_refill,l1d_tlb,l1d_tlb_refill,l2d_tlb,l2d_tlb_refill'),
]
)
def test_wperf_timeline_core_file_output_multiplexing(N, SLEEP,KERNEL_MODE,EVENTS):
    """ Test timeline (system - all cores) with multiplexing.  """
    cmd = f'wperf stat -e {EVENTS} -t -i 1 -n {N} sleep {SLEEP} -v'
    if (KERNEL_MODE):
        cmd += ' -k'

    stdout, _ = run_command(cmd.split())

    # Test for timeline file content
    assert b"timeline file: 'wperf_system_side_" in stdout    # e.g. timeline file: 'wperf_system_side_2023_06_29_10_05_27.core.csv'

    cvs_files = re.findall(rb'wperf_system_side_[0-9_]+\.core\.csv', stdout)   # e.g. ['wperf_system_side_2023_06_29_10_05_27.core.csv']
    assert len(cvs_files) == 1

    expected_events = 'cycle,sched_times,'  # Multiplexing start with this

    # We will weave in 'sched_times' between every event to match multiplexing column format
    for event in EVENTS.split(","):
        expected_events += event + ",sched_times,"

    with open(cvs_files[0], 'r') as file:
        cvs = file.read()

        str_kernel_mode = "Kernel mode," + str(KERNEL_MODE).upper()

        assert cvs.count("Multiplexing,TRUE") == 1
        assert cvs.count("Event class,core") == 1
        assert cvs.count(str_kernel_mode) == 1
        assert expected_events in cvs

        # Find lines with counter values, e.g.. 80182394,86203106,38111732,89739,61892,20932002,
        pattern = r'([0-9]+,){%s}\n' % (len(expected_events.split(',')))
        assert len(re.findall(pattern, cvs, re.DOTALL)) == N

@pytest.mark.parametrize("C,N,METRICS",
[
    (0, 3, "l1d_cache_miss_ratio"),
    (1, 2, "l1d_tlb_mpki"),
    (2, 1, "l1d_cache_miss_ratio,l1d_tlb_mpki"),
]
)
def test_wperf_timeline_ts_metrics(C, N, METRICS):
    """ Test timeline with TS metrics. """
    for metric in METRICS.split(","):
        if not wperf_metric_is_available(metric):
            pytest.skip("unsupported metric: " + metric)
            return

    cmd = f'wperf stat -m {METRICS} -t -i 1 -n {N} -c {C} sleep 1 -v'
    stdout, _ = run_command(cmd.split())

    COLS = int()    # How many columns are in this timeline (events + metrics)

    EVENTS = list()
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

@pytest.mark.parametrize("C,N,METRICS",
[
    ('1,2',     2, "l1d_cache_miss_ratio"),
    ('1,2',     3, "l1d_cache_miss_ratio,load_percentage"),
    ('1,2,3',   2, "l1d_tlb_mpki"),
    ('1,2,3',   3, "l1d_tlb_mpki,load_percentage"),
    ('1,3,5,7', 2, "l1d_cache_miss_ratio,l1d_tlb_mpki"),
    ('1,3,5,7', 3, "l1d_cache_miss_ratio,l1d_tlb_mpki,load_percentage"),
]
)
def test_wperf_timeline_ts_metrics_many_cores(C, N, METRICS):
    """ Test timeline with TS metrics. Multiple cores variant. """
    for metric in METRICS.split(","):
        if not wperf_metric_is_available(metric):
            pytest.skip("unsupported metric: " + metric)
            return

    cmd = f'wperf stat -m {METRICS} -t -i 1 -n {N} -c {C} --timeout 1 -v'
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
    EVENTS = list()
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
