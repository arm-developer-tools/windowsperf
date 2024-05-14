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
import pytest
from common import run_command, is_json, check_if_file_exists
from common import wperf_metric_is_available


N_CORES = os.cpu_count()

### Test cases

@pytest.mark.parametrize("events,cores,metric,sleep",
[
    (b"inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec", "0", "", 1),
    (b"inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec", "0", "imix", 1),
    (b"inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec", "0,1", "", 1),
    (b"inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec", ','.join(str(cores) for cores in range(0, N_CORES)), "", 1),

    (b"inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec,br_immed_spec,crypto_spec", "0", "", 1),
    (b"inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec,br_immed_spec,crypto_spec", "0", "imix", 1),
    (b"inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec,br_immed_spec,crypto_spec", "0,1", "", 1),
    (b"inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec,br_immed_spec,crypto_spec", ','.join(str(cores) for cores in range(0, N_CORES)), "", 1),
]
)
def test_wperf_stat_json(events,cores,metric,sleep):
    """ Test `wperf stat --json` command line output.

        Use pytest.mark.parametrize to set up below command line switches:

            ( -e <events>, -c <cores>, -m <metric>, sleep <value> )
    """
    cmd = 'wperf stat'.split()
    if events:
        cmd += ['-e', events]
    if cores:
        cmd += ['-c', cores]
    if metric and wperf_metric_is_available(metric):
        cmd += ['-m', metric]
    if sleep:
        cmd += ['sleep', str(sleep)]

    cmd += ['--json']

    stdout, _ = run_command(cmd)
    assert is_json(stdout)

def test_wperf_stat_no_events():
    """ Test for required -e for `wperf stat` """
    cmd = "wperf stat -c 0 sleep 1"
    _, stderr = run_command(cmd)
    assert b'no event specified' in stderr

@pytest.mark.parametrize("events,cores,metric,sleep",
[
    (b"inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec", "0", "", 1),
    (b"inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec", "0", "dcache", 1),
    (b"inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec", "0,1", "", 1),
    (b"inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec", ','.join(str(cores) for cores in range(0, N_CORES)), "", 1),
    (b"inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec", ','.join(str(cores) for cores in range(0, N_CORES,N_CORES//2)), "", 1),
    (b"inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec", ','.join(str(cores) for cores in range(0, N_CORES,N_CORES//6)), "", 1),
    (b"inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec", ','.join(str(cores) for cores in range(0, N_CORES,N_CORES//8)), "", 1),

    (b"inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec,br_immed_spec,crypto_spec", "0", "", 1),
    (b"inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec,br_immed_spec,crypto_spec", "0", "icache", 1),
    (b"inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec,br_immed_spec,crypto_spec", "0,1", "", 1),
    (b"inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec,br_immed_spec,crypto_spec", ','.join(str(cores) for cores in range(0, N_CORES)), "", 1),
    (b"inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec,br_immed_spec,crypto_spec", ','.join(str(cores) for cores in range(0, N_CORES,N_CORES//2)), "", 1),
    (b"inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec,br_immed_spec,crypto_spec", ','.join(str(cores) for cores in range(0, N_CORES,N_CORES//6)), "", 1),
    (b"inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec,br_immed_spec,crypto_spec", ','.join(str(cores) for cores in range(0, N_CORES,N_CORES//8)), "", 1),

    (b"LD_SPEC", "0", "", 1),
    (b"inst_spec,vfp_spec,ASE_SPEC,dp_spec,ld_spec,st_spec", "0", "", 1),
    (b"inst_spec,vfp_spec,ASE_SPEC,dp_spec,ld_spec,ST_SPEC", "0", "", 1),
    (b"INST_SPEC,VFP_SPEC,ASE_SPEC,DP_SPEC,LD_SPEC,ST_SPEC", "0", "", 1),
]
)
def test_wperf_stat(events,cores,metric,sleep):
    """ Test `wperf stat` command line output.

        Use pytest.mark.parametrize to set up below command line switches:

            ( -e <events>, -c <cores>, -m <metric>, sleep <value> )
    """
    cmd = 'wperf stat'.split()
    if events:
        cmd += ['-e', events]
    if cores:
        cmd += ['-c', cores]
    if metric and wperf_metric_is_available(metric):
        cmd += ['-m', metric]
    if sleep:
        cmd += ['sleep', str(sleep)]

    stdout, _ = run_command(cmd)

    # Common CLI outputs
    assert re.search(b'\\d+\\.\\d+ seconds time elapsed', stdout)

    # Core number message
    if cores:
        for core in cores.split(','):
            assert b'Performance counter stats for core %d' % int(core) in stdout

    # Columns we see when we print `wperf stat`
    COLS = [b'counter value' , b'event name', b'event idx', b'event note']
    MCOLS = [b'multiplexed' , b'scaled value']  # These columns are added when multiplexing

    # Find all column names in one row
    rstr = b'[\\s]+'    # Regext for all column names on one row
    for col in COLS:
        rstr += col + b'[\\s]+'

    if b', multiplexed' in stdout:  # Pretty table basic columns (multiplexing)
        for col in MCOLS:
            rstr += col + b'[\\s]+'

    assert re.search(rstr, stdout)  # Check for all columns in one line

    # Event names in pretty table
    if events:
        for event in events.split(b','):
            assert re.search(b'[\\d]+[\\s]+%s[\\s]+0x[0-9a-f]+' % event.lower(), stdout)
        assert re.search(b'[\\s]+cycle[\\s]+fixed', stdout)

    # Overall summary header when more than one CPU count
    # Note: if -c is not specified we count on all cores
    if not cores or len(cores.split(',')) > 1:
        assert b'System-wide Overall:' in stdout
    elif len(cores.split(',')) == 1:
        assert b'System-wide Overall:' not in stdout

@pytest.mark.parametrize("events,cores,metric,sleep",
[
    (b"inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec", "0", "", 1),
    (b"inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec", "0", "dcache", 1),
    (b"inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec", "0,1", "", 1),
    (b"inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec", ','.join(str(cores) for cores in range(0, N_CORES)), "", 1),

    (b"inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec,br_immed_spec,crypto_spec", "0", "", 1),
    (b"inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec,br_immed_spec,crypto_spec", "0", "dcache", 1),
    (b"inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec,br_immed_spec,crypto_spec", "0,1", "", 1),
    (b"inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec,br_immed_spec,crypto_spec", ','.join(str(cores) for cores in range(0, N_CORES)), "", 1),
]
)
def test_wperf_stat_json_file_output_exists(events, cores, metric, sleep, tmp_path):
    """ Test `wperf stat` JSON output to file """
    file_path = tmp_path / 'test.json'
    cmd = 'wperf stat'.split()
    if events:
        cmd += ['-e', events]
    if cores:
        cmd += ['-c', cores]
    if metric and wperf_metric_is_available(metric):
        cmd += ['-m', metric]
    if sleep:
        cmd += ['sleep', str(sleep)]

    cmd += ['--output', str(file_path)]

    print(' '.join(str(c) for c in cmd))
    stdout, _ = run_command(cmd)
    print(stdout)
    assert check_if_file_exists(str(file_path))

@pytest.mark.parametrize("events,cores,metric,sleep",
[
    (b"inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec", "0", "", 1),
    (b"inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec", "0", "icache", 1),
    (b"inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec", "0,1", "", 1),
    (b"inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec", ','.join(str(cores) for cores in range(0, N_CORES)), "", 1),

    (b"inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec,br_immed_spec,crypto_spec", "0", "", 1),
    (b"inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec,br_immed_spec,crypto_spec", "0", "icache", 1),
    (b"inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec,br_immed_spec,crypto_spec", "0,1", "", 1),
    (b"inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec,br_immed_spec,crypto_spec", ','.join(str(cores) for cores in range(0, N_CORES)), "", 1),
]
)
def test_wperf_stat_json_file_output_valid(events, cores, metric, sleep, tmp_path):
    """ Test `wperf stat` JSON output to file validity """
    file_path = tmp_path / 'test.json'
    cmd = 'wperf stat'.split()

    if events:
        cmd += ['-e', events]
    if cores:
        cmd += ['-c', cores]
    if metric:
        cmd += ['-m', metric]
    if sleep:
        cmd += ['sleep', str(sleep)]

    cmd += ['--output', str(file_path)]
    _, _ = run_command(cmd)
    try:
        with open(file_path) as f:
            json_obj = f.read()
            assert is_json(json_obj)
    except:
        assert 0

@pytest.mark.parametrize("flag,core",
[
    ("-k", 0),
    ("-k", 1),
    ("", 0),
    ("", 1),
]
)
def test_wperf_stat_K_flag(flag,core):
    cmd = f'wperf stat -m imix -c {core} {flag} --timeout 1'
    stdout, _ = run_command(cmd.split())

    ## "included," vs "excluded,"
    if (flag):
        kernel_mode_str = b"included"
    else:
        kernel_mode_str = b"excluded"

    k_str = b'Performance counter stats for core %d, no multiplexing, kernel mode %b,' % (core, kernel_mode_str)
    assert k_str in stdout

@pytest.mark.parametrize("flag,core",
[
    ("-k", 0),
    ("-k", 1),
    ("", 0),
    ("", 1),
]
)
def test_wperf_stat_K_flag_json(flag,core):
    """Check output if user defines `-k` (kernel mode)."""
    cmd = f'wperf stat -m imix -c {core} {flag} --timeout 1 --json'
    stdout, _ = run_command(cmd.split())
    json_output = json.loads(stdout)

    assert is_json(stdout)

    assert "core" in json_output
    assert "Kernel_mode" in json_output["core"]     # Check if kernel mode flag is present
    assert "Multiplexing" in json_output["core"]    # sanity check (no multiplexing this time
    assert json_output["core"]["cores"][0]["core_number"] == core            # sanity check

    assert json_output["core"]["Kernel_mode"] == bool(flag) #   bool("") ==> False, bool("-k") ==> True
    assert json_output["core"]["Multiplexing"] is False

def test_wperf_stat_process_spawn_args():
    """ Test CLI option -- that separates wperf CLI options with process name and command line arguments.
        Command line option -v enables extra printouts with PE file and arguments values.
    """
    if not check_if_file_exists("wperf.exe"):
        pytest.skip("place wperf.exe in the same directory as tests to run the test")

    cmd = 'wperf stat -m imix -c 1 --timeout 1 -v -- wperf.exe --help'
    stdout, _ = run_command(cmd.split())

    # Example output:
    # deduced image name 'wperf.exe'
    # deduced image PDB file 'wperf.pdb'
    # pe_file 'wperf.exe', args '--help'
    # 
    # events to be counted:
    #      6              core events: 0x001b 0x0073 0x0075 0x0074 0x0070 0x0071
    # wperf.exe pid is 7124
    # counting ... done
    # 
    # Performance counter stats for core 1, no multiplexing, kernel mode excluded, on Arm Limited core implementation:
    # note: 'e' - normal event, 'gN' - grouped event with group number N, metric name will be appended if 'e' or 'g' comes from it
    # 
    #         counter value  event name  event idx  event note
    #         =============  ==========  =========  ==========
    #                     0  cycle       fixed      e
    #                     0  inst_spec   0x1b       g0,imix
    #                     0  dp_spec     0x73       g0,imix
    #                     0  vfp_spec    0x75       g0,imix
    #                     0  ase_spec    0x74       g0,imix
    #                     0  ld_spec     0x70       g0,imix
    #                     0  st_spec     0x71       g0,imix

    assert b"deduced image name 'wperf.exe'" in stdout
    assert b"deduced image PDB file 'wperf.pdb'" in stdout
    assert b"pe_file 'wperf.exe', args 'wperf.exe --help'" in stdout
    for col in [b"counter value", b"event name", b"event idx", b"event note"]:
        assert col in stdout

@pytest.mark.parametrize("duration,expected_result",
[
    ("700ms", 0.7),
    ("0.1s", 0.1),
    ("1s", 1),
]
)
def test_wperf_stat_timeout_flag(duration, expected_result):
    cmd = f'wperf stat -m imix -c 0 -t -i 1 --timeout {duration} -n 1 --json' #'sleep' assumed to work if --timeout tests pass
    stdout, _ = run_command(cmd.split())
    json_output = json.loads(stdout)

    assert is_json(stdout)

    assert "count_duration" in json_output
    assert json_output["count_duration"] == expected_result


@pytest.mark.parametrize("duration",
[
    ("0.001ms"),
    ("qwerty"),
    ("2d30m"),
]
)
def test_wperf_stat_timeout_flag_throws(duration):
    cmd = f'wperf stat -m imix -c 0 --timeout {duration} --json'
    _,stderr = run_command(cmd.split())

    expected_error = f"input: \"{duration}\" to argument '--timeout' is invalid".encode()

    assert expected_error in stderr

@pytest.mark.parametrize("duration,expected_result",
[
    ("100ms", 0.1),
    ("0.01m", 0.6),
    ("0.50s", 0.5),
]
)
def test_wperf_stat_i_flag(duration, expected_result):
    cmd = f'wperf stat -m imix -c 0 -t -i {duration} --timeout 1 -n 1 --json'
    stdout, _ = run_command(cmd.split())
    json_output = json.loads(stdout)

    assert is_json(stdout)

    assert "count_interval" in json_output
    assert json_output["count_interval"] == expected_result

@pytest.mark.parametrize("duration",
[
    (b"."),
    (b"1!s"),
    (b".2m"),
]
)
def test_wperf_stat_i_flag_throws(duration):
    cmd = f'wperf stat -m imix -c 0 -t -i {duration} --timeout 1 -n 1 --json'
    _,stderr = run_command(cmd.split())

    expected_error = f"input: \"{duration}\" to argument '-i' is invalid".encode()

    assert expected_error in stderr


@pytest.mark.parametrize("cores,expected_cores",
[
    ("1,2,3", {1,2,3}),
    ("1-3", {1,2,3}),
    ("1,2,3,4-7", {1,2,3,4,5,6,7}),
    ("0-0,3-1", {0,1,2,3}),
    ("2-5,0-1,4", {0,1,2,3,4,5}),
]
)
def test_wperf_stat_cores_exist(cores, expected_cores):
    cmd = f'wperf stat -m imix -c {cores} --timeout 1 --json'
    print(cmd)
    stdout, _ = run_command(cmd.split())
    json_output = json.loads(stdout)

    assert is_json(stdout)
    
    assert "core" in json_output
    assert "cores" in json_output["core"]

    for result_core in json_output["core"]["cores"]:
        assert result_core["core_number"] in expected_cores
