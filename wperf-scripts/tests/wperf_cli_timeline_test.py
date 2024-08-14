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
This script runs simple wperf CLI tests.

Requires:
    pytest -    install with `pip install -U pytest`

Usage:
    >py.test wperf_cli_test.py

"""

import json
import os
import re
import tempfile
import pytest
from common import run_command
from common import get_result_from_test_results
from common import wperf_test_no_params
from common import is_json, check_if_file_exists

### Test cases

def test_wperf_timeline_core_n3():
    """ Test timeline (one core) output.  """
    cmd = 'wperf stat -m imix -c 1 -t -i 1 -n 3 sleep 1'
    stdout, _ = run_command(cmd.split())

    assert stdout.count(b"counting ...") == 3
    assert stdout.count(b"sleeping ...") == 3

@pytest.mark.parametrize("N",
[
    (1), (2), (5)
])
def test_wperf_timeline_system_n(N):
    """ Test timeline (system == all cores) output.  """
    cmd = f"wperf stat -m imix -t -i 1 -n {N} sleep 1"
    stdout, _ = run_command(cmd.split())

    assert stdout.count(b"counting ...") == N
    assert stdout.count(b"sleeping ...") == N

@pytest.mark.parametrize("C,I,N,SLEEP",
[
    (0,0,3,1),
    (0,1,5,2),

    (1,0,3,1),
    (1,1,5,2),
]
)
def test_wperf_timeline_core_n_file_output(C, I, N, SLEEP):
    """ Test timeline (core X) file format output.  """
    cmd = f'wperf stat -m imix -c {C} -t -i {I} -n {N} -v sleep {SLEEP}'
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
        assert cvs.count(f"Count interval,{I}.00") == 1
        assert cvs.count("Event class,core") == 1
        assert cvs.count("cycle,inst_spec,dp_spec,vfp_spec,") == 1  # This should be checked dynamically
        assert cvs.count(f"core {C},") == gpc_num + 1  # +1 for cycle fixed counter

        # Find lines with counter values, e.g.. 80182394,86203106,38111732,89739,61892,20932002,
        pattern = r'([0-9]+,){%s}\n' % (gpc_num + 1)
        assert len(re.findall(pattern, cvs, re.DOTALL)) == N

@pytest.mark.parametrize("C,I,N,SLEEP",
[
    (0,0,3,1),
    (0,1,5,2),
]
)
@pytest.mark.parametrize("CLI_OUTPUT",
[
    ("output"),
    ("output-csv"),
]
)
def test_wperf_timeline_core_n_file_cwd_output_and_csv_output(C, I, N, SLEEP, CLI_OUTPUT):
    """ Test timeline (core X) file format output.  """
    csv_file = f"timeline_csv_cwd_{os.getpid()}--{CLI_OUTPUT}.csv"
    tmp_dir = tempfile.TemporaryDirectory()
    file_path = os.path.join(tmp_dir.name, csv_file)

    assert len(file_path) > len(csv_file), f"'{file_path}' vs '{csv_file}'"

    cmd = f'wperf stat -m imix -c {C} -t -i {I} -n {N} -v --{CLI_OUTPUT} {csv_file} --timeout {SLEEP}'
    stdout, stderr = run_command(cmd.split() + ['--cwd', tmp_dir.name])

    assert b"unexpected arg" not in stderr

    # Test for timeline file name in verbose mode
    assert str.encode(file_path) in stdout, f"in {cmd} --cwd {tmp_dir.name}"
    assert b"timeline file: '%s'" % (str.encode(file_path)) in stdout, f"in {cmd} --cwd {tmp_dir.name}"

    # Test for timeline file content
    with open(file_path, 'r') as file:
        cvs = file.read()
        assert cvs.count("Multiplexing,FALSE") == 1
        assert cvs.count("Kernel mode,FALSE") == 1
        assert cvs.count(f"Count interval,{I}.00") == 1
        assert cvs.count("Event class,core") == 1
        assert cvs.count("cycle,inst_spec,dp_spec,vfp_spec,") == 1  # This should be checked dynamically

    tmp_dir.cleanup()

@pytest.mark.parametrize("I,N,SLEEP",
[
    (0,2,1),
    (0,4,2),

    (1,2,1),
    (1,4,2),
]
)
def test_wperf_timeline_system_n_file_output(I, N, SLEEP):
    """ Test timeline (system - all cores) file format output.  """
    cmd = f'wperf stat -m imix -t -i {I} -n {N} -v sleep {SLEEP}'
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
        assert cvs.count(f"Count interval,{I}.00") == 1
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

@pytest.mark.parametrize("C, CSV_FILENAME, EXPECTED",
[
    ("0",       "timeline_{core}_{class}.csv", b"timeline_0_core.csv"),
    ("1",       "{core}-{class}.csv", b"1-core.csv"),
    ("2",       "{class}-{core}.csv", b"core-2.csv"),
    ("3",       "{class}{core}.csv", b"core3.csv"),
    ("4,5,6",   "timeline_{core}_{class}.csv", b"timeline_4_core.csv"),
]
)
@pytest.mark.parametrize("CLI_OUTPUT",
[
    ("output"),
    ("output-csv"),
]
)
def test_wperf_timeline_core_n_cli_file_output_command(C, CSV_FILENAME, EXPECTED, CLI_OUTPUT):
    """ Test timeline --output <FILENAME> custom format.  """
    cmd = f'wperf stat -m imix -c {C} -t -i 1 -n 2 -v --timeout 1 --{CLI_OUTPUT} {CSV_FILENAME}'
    stdout, _ = run_command(cmd.split())

    # Test for timeline file content
    assert b"timeline file: '" in stdout    # Smoke test
    assert b"timeline file: '" + EXPECTED in stdout
    assert check_if_file_exists(EXPECTED)
    os.remove(EXPECTED)

@pytest.mark.parametrize("I,N,SLEEP,KERNEL_MODE,EVENTS",
[
    (0, 6, 1, True, 'l1i_cache,l1i_cache_refill,l2i_cache,l2i_cache_refill,inst_retired,inst_spec,dp_spec,vfp_spec'),
    (0, 5, 1, False, 'l1i_cache,l1i_cache_refill,l2i_cache,l2i_cache_refill,inst_retired,inst_spec,dp_spec,vfp_spec,ase_spec,ld_spec,st_spec'),
    (0, 4, 1, True, 'l1i_cache,l1i_cache_refill,l2i_cache,l2i_cache_refill,inst_retired,inst_spec,dp_spec,vfp_spec,ase_spec,ld_spec,st_spec,l1i_tlb'),
    (0, 3, 1, False, 'l1i_cache,l1i_cache_refill,l2i_cache,l2i_cache_refill,inst_retired,inst_spec,dp_spec,vfp_spec,ase_spec,ld_spec,st_spec,l1i_tlb,l1i_tlb_refill,l2i_tlb,l2i_tlb_refill'),
    (0, 2, 1, True, 'l1i_cache,l1i_cache_refill,l2i_cache,l2i_cache_refill,inst_retired,inst_spec,dp_spec,vfp_spec,ase_spec,ld_spec,st_spec,l1i_tlb,l1i_tlb_refill,l2i_tlb,l2i_tlb_refill,l1d_tlb,l1d_tlb_refill,l2d_tlb,l2d_tlb_refill'),

    (1, 6, 1, True, 'l1i_cache,l1i_cache_refill,l2i_cache,l2i_cache_refill,inst_retired,inst_spec,dp_spec,vfp_spec'),
    (2, 5, 1, False, 'l1i_cache,l1i_cache_refill,l2i_cache,l2i_cache_refill,inst_retired,inst_spec,dp_spec,vfp_spec,ase_spec,ld_spec,st_spec'),
    (3, 4, 1, True, 'l1i_cache,l1i_cache_refill,l2i_cache,l2i_cache_refill,inst_retired,inst_spec,dp_spec,vfp_spec,ase_spec,ld_spec,st_spec,l1i_tlb'),
    (2, 3, 1, False, 'l1i_cache,l1i_cache_refill,l2i_cache,l2i_cache_refill,inst_retired,inst_spec,dp_spec,vfp_spec,ase_spec,ld_spec,st_spec,l1i_tlb,l1i_tlb_refill,l2i_tlb,l2i_tlb_refill'),
    (1, 2, 1, True, 'l1i_cache,l1i_cache_refill,l2i_cache,l2i_cache_refill,inst_retired,inst_spec,dp_spec,vfp_spec,ase_spec,ld_spec,st_spec,l1i_tlb,l1i_tlb_refill,l2i_tlb,l2i_tlb_refill,l1d_tlb,l1d_tlb_refill,l2d_tlb,l2d_tlb_refill'),
]
)
def test_wperf_timeline_core_file_output_multiplexing(I, N, SLEEP, KERNEL_MODE, EVENTS):
    """ Test timeline (system - all cores) with multiplexing.  """
    cmd = f'wperf stat -e {EVENTS} -t -i {I} -n {N} sleep {SLEEP} -v'
    if KERNEL_MODE:
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

@pytest.mark.parametrize("C,I,N,SLEEP",
[
    (0,0,1,1),
    (1,1,2,1),
    (2,0,3,1),
    (3,1,4,1),
]
)
def test_wperf_timeline_json(C, I, N, SLEEP):
    """ Test timeline (core X) file format output.  """
    cmd = f'wperf stat -m imix -c {C} --json -t -i {I} -n {N} --timeout {SLEEP}'
    stdout, _ = run_command(cmd.split())
    json_output = json.loads(stdout)

    assert is_json(stdout), f"in {cmd}"
    assert "timeline" in json_output
    assert type(json_output["timeline"]) is list
    assert len(json_output["timeline"]) == N

@pytest.mark.parametrize("C,I,N,SLEEP",
[
    (0,1,1,1),
    (3,1,4,1),
    (2,1.3,7,2.2),
]
)
def test_wperf_timeline_json_output(C, I, N, SLEEP):
    """ Test timeline (core X) file format output.  """
    file_path = "timeline_json_%d.json" % os.getpid()
    cmd = ['wperf', 'stat', '-m', 'imix', '-c', str(C), '--json', '--output', str(file_path), '-t', '-i', str(I), '-n', str(N), '--timeout', str(SLEEP)]
    _, _ = run_command(cmd)

    try:
        with open(file_path) as f:
            json_output_f = f.read()
    except:
        assert 0, f"in {cmd}"

    assert is_json(json_output_f), f"in {cmd}"

    # load to object JSON from file for more checks
    json_output = json.loads(json_output_f)

    assert "count_duration" in json_output
    assert "count_interval" in json_output
    assert "count_timeline" in json_output
    assert "timeline" in json_output

    assert type(json_output["timeline"]) is list
    assert len(json_output["timeline"]) == N

    assert json_output["count_duration"] == SLEEP
    assert json_output["count_interval"] == I
    assert json_output["count_timeline"] == N

@pytest.mark.parametrize("C,I,N,SLEEP",
[
    (0, 1, 1, 2),
    (1, 1, 2, 2),
    (2, 1, 3, 2),
]
)
def test_wperf_timeline_json_cwd_output(C, I, N, SLEEP):
    """ Test timeline (core X) file format output.  """
    json_file = "timeline_json_cwd_%d.json" % os.getpid()
    tmp_dir = tempfile.TemporaryDirectory()
    file_path = os.path.join(tmp_dir.name, json_file)

    cmd = ['wperf', 'stat',
           '-m', 'imix',
           '-c', str(C),
           '--json',
           '--cwd', tmp_dir.name,
           '--output', str(json_file),
           '-t', '-i', str(I), '-n', str(N), '--timeout', str(SLEEP)]
    _, stderr = run_command(cmd)

    assert b"unexpected arg" not in stderr

    try:
        with open(file_path) as f:
            json_output_f = f.read()
    except:
        assert 0, f"in {cmd}"

    assert is_json(json_output_f), f"in {cmd}"

    # load to object JSON from file for more checks
    json_output = json.loads(json_output_f)

    assert "count_duration" in json_output
    assert "count_interval" in json_output
    assert "count_timeline" in json_output
    assert "timeline" in json_output

    assert type(json_output["timeline"]) is list
    assert len(json_output["timeline"]) == N

    assert json_output["count_duration"] == SLEEP
    assert json_output["count_interval"] == I
    assert json_output["count_timeline"] == N

    tmp_dir.cleanup()

@pytest.mark.parametrize("C,I,N,SLEEP",
[
    (0, 1, 2, 3),
]
)
def test_wperf_timeline_json_cwd_output_and_csv_output(C, I, N, SLEEP):
    """ Test timeline with --output and --output-csv  """
    json_file = "timeline_cwd_json_2_%d.json" % os.getpid()
    csv_file = "timeline_cwd_csv_2_%d.csv" % os.getpid()
    tmp_dir = tempfile.TemporaryDirectory()
    file_json_path = os.path.join(tmp_dir.name, json_file)
    file_csv_path = os.path.join(tmp_dir.name, csv_file)

    cmd = ['wperf', 'stat',
           '-m', 'imix',
           '-c', str(C),
           '--json',
           '--cwd', tmp_dir.name,
           '--output', str(json_file),
           '--output-csv', str(csv_file),
           '-t', '-i', str(I), '-n', str(N), '--timeout', str(SLEEP)]
    _, stderr = run_command(cmd)

    assert b"unexpected arg" not in stderr

    assert len(file_json_path) > len(json_file)
    assert len(file_csv_path) > len(csv_file)

    #
    # Check if --output has JSON file
    #
    try:
        with open(file_json_path) as f:
            json_output_f = f.read()
            assert is_json(json_output_f), f"in {cmd}"
    except:
        assert 0, f"in {cmd}"

    #
    # Check if --output-csv has CSV file
    #
    with open(file_csv_path, 'r') as file:
        cvs = file.read()
        assert cvs.count("Multiplexing,FALSE") == 1, f"in {cmd}"
        assert cvs.count("Kernel mode,FALSE") == 1, f"in {cmd}"
        assert cvs.count(f"Count interval,{I}.00") == 1, f"in {cmd}"
        assert cvs.count("Event class,core") == 1, f"in {cmd}"
        assert cvs.count("cycle,inst_spec,dp_spec,vfp_spec,") == 1, f"in {cmd}"  # This should be checked dynamically

    tmp_dir.cleanup()
