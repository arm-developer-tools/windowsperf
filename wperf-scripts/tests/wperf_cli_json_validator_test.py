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

from jsonschema import validate
import json
import os
from common import run_command, get_schema
import pytest

### Test cases

@pytest.mark.parametrize("scheme_name", [ "version", "list", "test", "stat" ])
def test_wperf_json_schema(request, tmp_path, scheme_name):
    """ Test `wperf` JSON output against scheme """
    test_path = os.path.dirname(request.path)
    file_path = tmp_path / 'test.json'
    if "version" in scheme_name:
        cmd_type = "-version"
    elif "list" in scheme_name:
        cmd_type = "list"
    elif "test" in scheme_name:
        cmd_type = "test"
    elif "stat" in scheme_name:
        cmd_type = "stat -e cpu_cycles sleep 1"
    cmd = 'wperf {} --output {}'.format(cmd_type, str(file_path))
    stdout, _ = run_command(cmd.split())

    with open(str(file_path)) as json_file:
        json_output = json.loads(json_file.read())
    try:
        validate(instance=json_output, schema=get_schema(scheme_name, test_path))
    except:
        assert False
    assert True