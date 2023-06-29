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

"""
This script runs simple wperf CLI tests.

Requires:
    pytest -    install with `pip install -U pytest`

Usage:
    >py.test wperf_cli_test.py

"""

import os
import re
from common import run_command
from common import get_result_from_test_results
from common import wperf_test_no_params

import pytest

### Test cases

def test_wperf_timeline_core_n3():
    """ Test timeline (one core) output.  """
    cmd = 'wperf stat -m imix -c 1 -t -i 1 -n 3 sleep 1'
    stdout, _ = run_command(cmd.split())

    assert stdout.count(b"counting ...") == 3
    assert stdout.count(b"sleeping ...") == 3

def test_wperf_timeline_system_n3():
    """ Test timeline (system == all cores) output.  """
    cmd = 'wperf stat -m imix -t -i 1 -n 3 sleep 1'
    stdout, _ = run_command(cmd.split())

    assert stdout.count(b"counting ...") == 3
    assert stdout.count(b"sleeping ...") == 3

@pytest.mark.parametrize("C, N, SLEEP",
[
    (0,3,1),
    (1,5,2),
]
)
def test_wperf_timeline_core_n_file_output(C, N, SLEEP):
    """ Test timeline (core X) file format output.  """
    cmd = 'wperf stat -m imix -c %s -t -i 1 -n %s -v sleep %s' % (C, N, SLEEP)
    stdout, _ = run_command(cmd.split())

    json_output = wperf_test_no_params()      # get output from `wperf test`
    gpc_num = get_result_from_test_results(json_output, "PMU_CTL_QUERY_HW_CFG [gpc_num]")
    gpc_num = int(gpc_num, 16)  # it's a hex string e,g,. 0x0005

    assert stdout.count(b"counting ...") == N
    assert stdout.count(b"sleeping ...") == N

    # Test for timeline file content
    assert b"timeline file: 'wperf_core_%s" % (str.encode(str(C))) in stdout    # e.g. timeline file: 'wperf_core_1_2023_06_29_09_09_05.core.csv'

    cvs_files = re.findall(rb'wperf_core_%s[a-z0-9_.]+' % (str.encode(str(C))), stdout)   # e.g. ['wperf_core_1_2023_06_29_09_09_05.core.csv']
    assert len(cvs_files) == 1

    with open(cvs_files[0], 'r') as file:
        cvs = file.read()

        assert cvs.count("Multiplexing,FALSE") == 1
        assert cvs.count("Kernel mode,FALSE") == 1
        assert cvs.count("Count interval,1.00") == 1
        assert cvs.count("Event class,core") == 1
        assert cvs.count("cycle,inst_spec,dp_spec,vfp_spec,") == 1  # This should be checked dynamically
        assert cvs.count("core %s," % (C)) == gpc_num + 1  # +1 for cycle fixed counter

        # Find lines with counter values, e.g.. 80182394,86203106,38111732,89739,61892,20932002,
        pattern = r'([0-9]+,){%s}\n' % (gpc_num + 1)
        assert len(re.findall(pattern, cvs, re.DOTALL)) == N

@pytest.mark.parametrize("N, SLEEP",
[
    (2,1),
    (4,2),
]
)
def test_wperf_timeline_system_n_file_output(N, SLEEP):
    """ Test timeline (system - all cores) file format output.  """
    cmd = 'wperf stat -m imix -t -i 1 -n %s -v sleep %s' % (N, SLEEP)
    stdout, _ = run_command(cmd.split())

    json_output = wperf_test_no_params()      # get output from `wperf test`
    gpc_num = get_result_from_test_results(json_output, "PMU_CTL_QUERY_HW_CFG [gpc_num]")
    gpc_num = int(gpc_num, 16)  # it's a hex string e,g,. 0x0005

    N_CORES = os.cpu_count()

    assert stdout.count(b"counting ...") == N
    assert stdout.count(b"sleeping ...") == N

    # Test for timeline file content
    assert b"timeline file: 'wperf_system_side_" in stdout    # e.g. timeline file: 'wperf_system_side_2023_06_29_10_05_27.core.csv'

    cvs_files = re.findall(rb'wperf_system_side_[a-z0-9_.]+', stdout)   # e.g. ['wperf_system_side_2023_06_29_10_05_27.core.csv']
    assert len(cvs_files) == 1

    with open(cvs_files[0], 'r') as file:
        cvs = file.read()

        assert cvs.count("Multiplexing,FALSE") == 1
        assert cvs.count("Kernel mode,FALSE") == 1
        assert cvs.count("Count interval,1.00") == 1
        assert cvs.count("Event class,core") == 1
        assert cvs.count("cycle,inst_spec,dp_spec,vfp_spec,") == N_CORES  # This should be checked dynamically

        cores_str = str()
        for C in range(0, N_CORES):
            for i in range(0, gpc_num + 1):
                cores_str += "core %s," % (C)

        assert cvs.count(cores_str) == 1

        # Find lines with counter values, e.g.. 80182394,86203106,38111732,89739,61892,20932002,
        pattern = r'([0-9]+,){%s}\n' % (gpc_num + 1)
        assert len(re.findall(pattern, cvs, re.DOTALL)) == N