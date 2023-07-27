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

from common import run_command, is_json, check_if_file_exists

### Test cases

def test_wperf_wrong_argument():
    """ Test `wperf` invalid argument output  """
    cmd = 'wperf WRONG_ARGUMENT'
    stdout, _ = run_command(cmd.split())
    assert b'unexpected arg' in stdout

def test_wperf_version_json():
    """ Test `wperf --version` JSON output  """
    cmd = 'wperf --version --json'
    stdout, _ = run_command(cmd.split())
    assert is_json(stdout)

def test_wperf_version_json_file_output_exists(tmp_path):
    """ Test `wperf --version` JSON output to file"""
    file_path = tmp_path / 'test.json'
    cmd = 'wperf --version --output ' + str(file_path)
    stdout, _ = run_command(cmd.split())
    assert check_if_file_exists(str(file_path))

def test_wperf_version_json_file_output_valid(tmp_path):
    """ Test `wperf --version` JSON output to file validity """
    file_path = tmp_path / 'test.json'
    cmd = 'wperf --version --output ' + str(file_path)
    stdout, _ = run_command(cmd.split())
    try:
        f = open(file_path)
        json = f.read()
        f.close()
        assert is_json(json)
    except:
        assert 0

