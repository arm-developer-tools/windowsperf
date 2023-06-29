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

import pytest
import re
import subprocess

### Test cases

def test_wperf_lib_app():
    """ Run wperf-lib-app.exe and verify a few invariants. """
    try:
        process = subprocess.run(["wperf-lib-app.exe"], capture_output=True, text=True)
    except:
        pytest.skip("Can not run wperf-lib-app.exe")

    # wperf-lib-app's return value should be 0
    assert process.returncode == 0

    # wperf-lib-app should print to stdout wperf_driver_version, wperf_version, etc.
    assert 'wperf_driver_version:' in process.stdout
    assert 'wperf_version:' in process.stdout
    assert 'wperf_list_num_events:' in process.stdout
    assert 'wperf_list_events:' in process.stdout
    assert 'wperf_list_num_metrics:' in process.stdout
    assert 'wperf_list_num_metrics_events:' in process.stdout
    assert 'wperf_list_metrics:' in process.stdout
    assert 'wperf_stat:' in process.stdout

    # The number of lines with 'wperf_list_events:' should match the value
    # returned by wperf_list_num_events.
    result = re.search(r"wperf_list_num_events: (\d+)", process.stdout)
    assert process.stdout.count('wperf_list_events:') == int(result.group(1))

    # The number of lines with 'wperf_list_metrics:' should match the value
    # returned by wperf_list_num_metrics_events.
    result = re.search(r"wperf_list_num_metrics_events: (\d+)", process.stdout)
    assert process.stdout.count('wperf_list_metrics:') == int(result.group(1))
