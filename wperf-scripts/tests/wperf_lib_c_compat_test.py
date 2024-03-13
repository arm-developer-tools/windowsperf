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


""" This simple module calls `wperf-lib-c-compat.exe` to check if its running correctly.
"""

import subprocess
import pytest
from common import run_command


### Test cases

def test_wperf_lib_c_compat():
    """ Run wperf-lib-c-compat.exe and verify a few invariants. """
    try:
        process = subprocess.run(["wperf-lib-c-compat.exe"], capture_output=True, text=True)
    except Exception as error:
        run_command("wperf --force-lock".split())
        pytest.skip(f"Can not run wperf-lib-c-compat.exe, reason={error}")

    #
    # If this executable fails we will clear the lock
    #
    run_command("wperf --force-lock".split())

    # wperf-lib-c-compat's return value should be 0
    assert process.returncode == 0

    """
    >wperf-lib-c-compat.exe
    Test wperf_init()        ... PASS
    Test wperf_set_verbose(true)     ... PASS
    Test wperf_driver_version(&driver_ver)   ... PASS
    Test wperf_version(&driver_ver)  ... PASS
    Test wperf_list_events(&list_conf, NULL)         ... PASS
    Test wperf_list_num_events(&list_conf, &num_events)      ... PASS
    Test wperf_list_metrics(&list_conf, NULL)        ... PASS
    Test wperf_list_num_metrics(&list_conf, &num_metrics)    ... PASS
    Test wperf_list_num_metrics_events(&list_conf, &num_metric_events)       ... PASS
    Test wperf_num_cores(&num_cores)         ... PASS
    Test wperf_test(&test_conf, NULL)        ... PASS
    Test wperf_close()       ... PASS
    """

    assert '... FAIL' not in process.stdout
    assert process.stdout.count("... PASS") > 0
