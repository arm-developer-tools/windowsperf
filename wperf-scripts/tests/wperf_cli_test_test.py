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

import json
from common import run_command, is_json, check_if_file_exists

### Test cases

def test_wperf_test_json():
    """ Test `wperf test` JSON output  """
    cmd = 'wperf test -json'
    stdout, _ = run_command(cmd.split())
    assert is_json(stdout)

def test_wperf_test_json_file_output_exists(tmp_path):
    """ Test `wperf test` JSON output to file"""
    file_path = tmp_path / 'test.json'
    cmd = 'wperf test --output ' + str(file_path)
    stdout, _ = run_command(cmd.split())
    assert check_if_file_exists(str(file_path))

def test_wperf_test_json_file_output_valid(tmp_path):
    """ Test `wperf test` JSON output to file validity """
    file_path = tmp_path / 'test.json'
    cmd = 'wperf test --output ' + str(file_path)
    stdout, _ = run_command(cmd.split())
    try:
        f = open(file_path)
        json = f.read()
        f.close()
        assert is_json(json)
    except:
        assert 0

def test_wperf_test_event_sched():
    """ Test `wperf test -e ...` ioctl_events output (event schedules) """
    cmd = 'wperf test -e {inst_spec,vfp_spec},{ase_spec,br_immed_spec},crypto_spec,{ld_spec,st_spec} -json'
    stdout, _ = run_command(cmd.split())
    json_output = json.loads(stdout)
    num_gpc = int(json_output["Test_Results"][17]["Result"])
    evt_indexes = json_output["Test_Results"][21]["Result"].split(',')
    evt_indexes_set = set(evt_indexes)
    evt_indexes_ref = {'27', '117', '119', '116', '120', '112', '113'}
    evt_notes = json_output["Test_Results"][22]["Result"].split(',')
    evt_notes_set = set(evt_notes)
    evt_notes_ref = {'g0', 'g1', 'g2', 'e'}
    num_paddings = evt_notes.count('p')
    assert (len(evt_indexes) % num_gpc == 0
            and evt_indexes_ref.issubset(evt_indexes_set)
            and evt_notes_ref.issubset(evt_notes_set)
            and num_paddings < 12)
