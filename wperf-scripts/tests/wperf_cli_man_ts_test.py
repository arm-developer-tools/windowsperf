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


"""Module is testing `wperf man` features."""
import pytest
import json
from common import run_command, is_json, check_if_file_exists

### Test cases

@pytest.mark.parametrize("cpu",
[
    ("neoverse-v1"),
    ("neoverse-v2"),
    ("neoverse-n1"),
    ("neoverse-n2"),
    ("neoverse-n2-r0p3"),
]
)
def test_wperf_man_ts_json(cpu):
    """ Test `wperf man` JSON output  """
    cmd = f'wperf man {cpu}/ld_spec --json'
    stdout, _ = run_command(cmd.split())
    assert is_json(stdout)

@pytest.mark.parametrize("cpu",
[
    ("neoverse-v1"),
    ("neoverse-v2"),
    ("neoverse-n1"),
    ("neoverse-n2"),
    ("neoverse-n2-r0p3"),
]
)
def test_wperf_man_ts_json_file_output_exists(cpu,tmp_path):
    """ Test `wperf  man` JSON output to file """
    file_path = tmp_path / 'test.json'
    cmd = ['wperf', 'man', f'{cpu}/ld_spec', '--output', str(file_path)]
    _, _ = run_command(cmd)
    assert check_if_file_exists(str(file_path))

@pytest.mark.parametrize("cpu",
[
    ("neoverse-v1"),
    ("neoverse-v2"),
    ("neoverse-n1"),
    ("neoverse-n2"),
    ("neoverse-n2-r0p3"),
]
)
def test_wperf_man_ts_json_file_output_valid(cpu,tmp_path):
    """ Test `wperf man` JSON output to file validity"""
    file_path = tmp_path / 'test.json'
    cmd = ['wperf', 'man', f'{cpu}/sw_incr', '--output', str(file_path)]
    _, _ = run_command(cmd)
    try:
        with open(file_path) as f:
            json_obj = f.read()
            assert is_json(json_obj)
    except:
        assert 0

def test_wperf_man_ts_event_json():
    """ Test `wperf man` JSON output  """
    cmd = f'wperf man neoverse-n1/ld_spec --json'
    stdout, _ = run_command(cmd.split())
    json_output = json.loads(stdout)

    assert is_json(stdout)
    assert b'CPU' in stdout
    assert b'NAME' in stdout
    assert b'DESCRIPTION' in stdout

def test_wperf_man_ts_metric_json():
    """ Test `wperf man` JSON output  """
    cmd = f'wperf man neoverse-n1/branch_percentage --json'
    stdout, _ = run_command(cmd.split())
    json_output = json.loads(stdout)

    assert is_json(stdout)
    assert b'CPU' in stdout
    assert b'NAME' in stdout
    assert b'DESCRIPTION' in stdout
    assert b'EVENTS' in stdout
    assert b'FORMULA' in stdout
    assert b'UNIT' in stdout

def test_wperf_man_ts_group_metrics_json():
    """ Test `wperf man` JSON output  """
    cmd = f'wperf man neoverse-n1/Miss_Ratio --json'
    stdout, _ = run_command(cmd.split())
    json_output = json.loads(stdout)

    assert is_json(stdout)
    assert b'CPU' in stdout
    assert b'NAME' in stdout
    assert b'DESCRIPTION' in stdout
    assert b'METRICS' in stdout


@pytest.mark.parametrize("cpu",
[
    ("neoverse-v1"),
    ("neoverse-v2"),
    ("neoverse-n1"),
    ("neoverse-n2"),
    ("neoverse-n2-r0p3"),
]
)
@pytest.mark.parametrize("argument",
[
    (""),
    ("ip_c"),
    ("tomorrow_land"),
    ("Miss_Rati0"),
    ("neoverse-n1/Miss_Ratio"),
    ("SVE-INST_--/ld_specSPEC"),
    ("..."),
]
)
def test_wperf_man_ts_invalid_arg_throws(cpu, argument):
    """Test `wperf man` when prompted with invalid CPUs throws the necessary error"""
    cmd = f'wperf man {cpu}/{argument}'
    _,stderr = run_command(cmd.split())

    assert b"unexpected arg" not in stderr

    arg_space = argument.find(" ")

    if arg_space == -1 :
        expected_error_arg = f"warning: \"{argument}\" not found! Ensure it is compatible with the specified CPU".encode()
    else:
        expected_error_arg = f"warning: \"{ argument[0:arg_space] }\" not found! Ensure it is compatible with the specified CPU".encode()

    assert expected_error_arg in stderr

@pytest.mark.parametrize("alias",
[
    ("neoverse-n2-r0p0"),
    ("neoverse-n2-r0p1"),
]
)
@pytest.mark.parametrize("cpu, event",
[
    ("neoverse-n2", "ld_spec"),
    ("neoverse-n2", "sve_inst_spec"),
    ("neoverse-n2", "ase_sve_int64_spec"),
]
)
def test_wperf_man_ts_cpu_alias_compare(alias, cpu, event):
    cmd = f'wperf man {alias}/{event}'
    stdout_alias, _ = run_command(cmd.split())

    cmd = f'wperf man {cpu}/{event}'
    stdout_cpu, _ = run_command(cmd.split())

    assert b"NAME" in stdout_alias
    assert b"CPU" in stdout_alias
    assert b"NAME" in stdout_cpu
    assert b"CPU" in stdout_cpu
    assert stdout_alias == stdout_cpu

@pytest.mark.parametrize("cpu",
[
    ("neoverse-v1"),
    ("neoverse-v2"),
    ("neoverse-v3"),
    ("neoverse-n1"),
    ("neoverse-n2"),
    ("neoverse-n3"),
    ("neoverse-n2-r0p0"),
    ("neoverse-n2-r0p1"),
    ("neoverse-n2-r0p3"),
]
)
@pytest.mark.parametrize("argument, title, description",
[
    ("ld_spec", b'Operation speculatively executed', b'Counts speculatively executed load operations'),
    ("sw_incr", b'Instruction architecturally executed', b'Counts software writes to the PMSWINC_EL0'),
]
)
def test_wperf_man_ts_events(cpu, argument, title, description):
    """ Test `wperf man` when prompt is an event """
    cmd = ['wperf', 'man', f'{cpu}/{argument}']
    stdout,_ = run_command(cmd)

    assert b'CPU' in stdout
    assert b'NAME' in stdout
    assert b'DESCRIPTION' in stdout
    assert argument.encode() in stdout
    assert title in stdout
    assert description in stdout


@pytest.mark.parametrize("cpu",
[
    ("neoverse-v1"),
    ("neoverse-v2"),
    ("neoverse-v3"),
    ("neoverse-n1"),
    ("neoverse-n2"),
    ("neoverse-n2-r0p0"),
    ("neoverse-n2-r0p1"),
    ("neoverse-n2-r0p3"),
]
)
@pytest.mark.parametrize("argument, event, unit",
[
    ("branch_percentage", b'br_immed_spec', b'percentage of operations'),
    ("crypto_percentage", b'crypto_spec', b'percent of operations'),
    ("ipc", b'cpu_cycles', b'per cycle'),
]
)
def test_wperf_man_ts_metrics(cpu, argument, event, unit):
    """ Test `wperf man` when prompt is an metric """
    cmd = ['wperf', 'man', f'{cpu}/{argument}']
    stdout,_ = run_command(cmd)

    assert b'CPU' in stdout
    assert b'NAME' in stdout
    assert b'DESCRIPTION' in stdout
    assert b'EVENTS' in stdout
    assert b'FORMULA' in stdout
    assert b'UNIT' in stdout

    assert argument.encode() in stdout
    assert event in stdout
    assert unit in stdout


@pytest.mark.parametrize("cpu",
[
    ("neoverse-v1"),
    ("neoverse-v2"),
    ("neoverse-v3"),
    ("neoverse-n1"),
    ("neoverse-n2"),
    ("neoverse-n3"),
    ("neoverse-n2-r0p0"),
    ("neoverse-n2-r0p1"),
    ("neoverse-n2-r0p3"),
]
)
@pytest.mark.parametrize("argument, title, metric",
[
    ("MPKI", b'Misses Per Kilo Instructions', b'branch_mpki'),
    ("Miss_Ratio", b'Miss Ratio', b'branch_misprediction_ratio'),
    ("Operation_Mix", b'Speculative Operation Mix', b'load_percentage')
]
)
def test_wperf_man_ts_group_metrics(cpu, argument, title, metric):
    """ Test `wperf man` when prompt is a group metric """
    cmd = ['wperf', 'man', f'{cpu}/{argument}']
    stdout,_ = run_command(cmd)

    assert b'CPU' in stdout
    assert b'NAME' in stdout
    assert b'DESCRIPTION' in stdout
    assert b'METRICS' in stdout

    assert argument.encode() in stdout
    assert title in stdout
    assert metric in stdout

@pytest.mark.parametrize("cpu",
[
    ("neoverse-v1"),
    ("neoverse-v2"),
    ("neoverse-v3"),
    ("neoverse-n1"),
    ("neoverse-n2"),
    ("neoverse-n3"),
]
)
@pytest.mark.parametrize("argument",
[
    ("ld_spec"),
    ("sw_incr"),
    ("ipc"),
    ("Miss_Ratio"),
    ("crypto_percentage"),
]
)
def test_wperf_man_ts_line_length(cpu, argument):
    """ Test `wperf man` when prompt is an event """
    cmd = ['wperf', 'man', f'{cpu}/{argument}']

    stdout,_ = run_command(cmd)

    lines = stdout.splitlines()

    for line in lines:
        assert len(line) <= 81, f"Line exceeds 81 characters: {line}"
