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
from common import get_result_from_test_results
from common import arm64_vendor_names

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

def test_wperf_test_MIDR_reg():
    """ Test if MIDR register is exposed with `wperf test`. """
    cmd = 'wperf test -json'
    stdout, _ = run_command(cmd.split())
    json_output = json.loads(stdout)

    midr_value = get_result_from_test_results(json_output, "PMU_CTL_QUERY_HW_CFG [midr_value]")
    assert len(midr_value) > 0
    assert midr_value.startswith("0x")
    assert int(midr_value, 16) != 0x00

def test_wperf_test_MIDR_vendor_id():
    """ Test if MIDR register field `vendor_id` value is correct. """
    cmd = 'wperf test -json'
    stdout, _ = run_command(cmd.split())
    json_output = json.loads(stdout)

    vendor_id = get_result_from_test_results(json_output, "PMU_CTL_QUERY_HW_CFG [vendor_id]")
    
    assert int(vendor_id, 16) in arm64_vendor_names.keys()
