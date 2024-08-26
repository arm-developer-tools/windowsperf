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


"""Module is testing `wperf list` features."""
import json
import pytest
from common import run_command, is_json, check_if_file_exists
from common import get_spe_version, wperf_event_is_available

### Test cases

def test_wperf_list_json():
    """ Test `wperf list` JSON output  """
    cmd = 'wperf list --json'
    stdout, _ = run_command(cmd.split())
    assert is_json(stdout)

def test_wperf_list_json_file_output_exists(tmp_path):
    """ Test `wperf list` JSON output to file """
    file_path = tmp_path / 'test.json'
    cmd = ['wperf', 'list', '--output', str(file_path)]
    _, _ = run_command(cmd)
    assert check_if_file_exists(str(file_path))

def test_wperf_list_json_file_output_valid(tmp_path):
    """ Test `wperf list` JSON output to file validity"""
    file_path = tmp_path / 'test.json'
    cmd = ['wperf', 'list', '--output', str(file_path)]
    _, _ = run_command(cmd)
    try:
        with open(file_path) as f:
            json_obj = f.read()
            assert is_json(json_obj)
    except:
        assert 0

def test_wperf_list_metric_json_verbose(tmp_path):
    """ Test `wperf list` JSON output in verbose mode """

    """
    "Predefined_Metrics": [
        {
            "Metric": "backend_stalled_cycles",
            "Events": "{cpu_cycles,stall_backend}",
            "Formula": "((stall_backend / cpu_cycles) * 100)",
            "Unit": "percent of cycles",
            "Description": "Backend Stalled Cycles"
        },
    """

    cmd = 'wperf list -v --json'
    stdout, _ = run_command(cmd.split())
    assert is_json(stdout)

    json_output = json.loads(stdout)
    assert 'Predefined_Metrics' in json_output
    assert  len(json_output['Predefined_Metrics'])

    for metric in json_output['Predefined_Metrics']:
        assert 'Metric' in metric
        assert 'Events' in metric
        assert 'Formula' in metric
        assert 'Unit' in metric
        assert 'Description' in metric

def test_wperf_list_events_json_verbose(tmp_path):
    """ Test `wperf list` JSON output in verbose mode """

    """
    "Predefined_Events": [
        {
            "Alias_Name": "sw_incr",
            "Raw_Index": "0x0000",
            "Event_Type": "[core PMU event]",
            "Description": "Instruction architecturally executed, Condition code check pass, software increment"
        },
    """

    cmd = 'wperf list -v --json'
    stdout, _ = run_command(cmd.split())
    assert is_json(stdout)

    json_output = json.loads(stdout)
    assert 'Predefined_Events' in json_output
    assert  len(json_output['Predefined_Events'])

    for metric in json_output['Predefined_Events']:
        assert 'Alias_Name' in metric
        assert 'Raw_Index' in metric
        assert 'Event_Type' in metric
        assert 'Description' in metric

def test_wperf_list_spe_available():
    """ Test for SPE in --help usage string
    """
    spe_device = get_spe_version()
    assert spe_device is not None
    if not spe_device.startswith("FEAT_SPE"):
        pytest.skip(f"no SPE support in HW, see spe_device.version_name={spe_device}")

    ## Is SPE enabled in `wperf` CLI?
    if not wperf_event_is_available("arm_spe_0//"):
        pytest.skip(f"no SPE support in `wperf`, see spe_device.version_name={spe_device}")

    cmd = 'wperf list'
    stdout, _ = run_command(cmd.split())

    """
        arm_spe_0//                                        [Kernel PMU event]
        load_filter                                        [SPE filter]
        store_filter                                       [SPE filter]
        branch_filter                                      [SPE filter]
    """
    for line in stdout.splitlines():
        if b"load_filter" in line:
            assert b"[SPE filter]" in line
        if b"store_filter" in line:
            assert b"[SPE filter]" in line
        if b"branch_filter" in line:
            assert b"[SPE filter]" in line
