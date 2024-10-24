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
from common import run_command

### Test cases

def test_wperf_man_help():
    """ Test `wperf man` with no args"""
    cmd = 'wperf man'
    _, stderr = run_command(cmd.split())

    assert b"unexpected arg" not in stderr
    expected_warning = b'warning: no argument(s) specified, specify an'
    assert expected_warning in stderr

@pytest.mark.parametrize("cpu, argument",
[
    ("armv8-a", "Operation_Mix"),
    ("armv9-a", "Operation_Mix"),
]
)
def test_wperf_man_not_compatible_cpu_throws(cpu, argument):
    """Test `wperf man` when prompted with invalid CPUs throws the necessary error"""
    cmd = f'wperf man {cpu}/{argument}'
    _, stderr = run_command(cmd.split())

    assert b"unexpected arg" not in stderr
    expected_error = f"warning: \"{argument}\" not found! Ensure it is compatible with the specified CPU".encode()
    assert expected_error in stderr

# Note - stacked parameterizations creates permutations
@pytest.mark.parametrize("cpu",
[
    (""),
    (".!!33"),
    ("never-verse-n1"),
    ("new_arm_cpu"),
]
)
@pytest.mark.parametrize("argument",
[
    ("ld_spec"),
    ("ipc"),
    ("sw_incr"),
    ("SAMPLE_COLLISION"),
    ("Miss_Ratio"),
    ("MEMORY_ERROR"),
    ("SVE_INST_SPEC"),
]
)
def test_wperf_man_invalid_cpu_throws(cpu, argument):
    """Test `wperf man` when prompted with invalid CPUs throws the necessary error"""
    cmd = f'wperf man {cpu}/{argument}'
    _, stderr = run_command(cmd.split())

    assert b"unexpected arg" not in stderr
    expected_error = f"warning: CPU name: \"{cpu}\" not found, use".encode()
    assert expected_error in stderr

#
# NOK cases for armv8-a / armv9-a
#

@pytest.mark.parametrize("cpu, argument",
[
    # This events are not present in given CPU
    ("armv8-a", "trcextout0"),
    ("armv8-a", "trcextout1"),
    ("armv8-a", "trcextout2"),
    ("armv8-a", "trcextout3"),
]
)
def test_wperf_man_not_compatible_cpu_event_err(cpu, argument):
    """Test `wperf man` when prompted with invalid CPUs throws the necessary error"""
    cmd = f'wperf man {cpu}/{argument}'
    _, stderr = run_command(cmd.split())

    assert b"unexpected arg" not in stderr
    expected_error = f"warning: \"{argument}\" not found! Ensure it is compatible with the specified CPU".encode()
    assert expected_error in stderr

#
# OK cases for armv8-a / armv9-a
#

# Note - stacked parameterizations creates permutations
@pytest.mark.parametrize("cpu",
[
    ("armv8-a"),
    ("armv9-a"),
]
)
@pytest.mark.parametrize("argument",
[
    ("ld_spec"),
    ("remote_access_rd"),
    ("l3d_cache_lmiss_rd"),
    ("stall_frontend_membound"),
    ("br_immed_mis_pred_retired"),
]
)
def test_wperf_man_armv89_common_events(cpu, argument):
    """Test `wperf man` when prompted with invalid CPUs throws the necessary error"""
    cmd = f'wperf man {cpu}/{argument}'
    stdout, stderr = run_command(cmd.split())

    assert b"unexpected arg" not in stderr
    assert b"warning:" not in stderr
    assert b"CPU" in stdout
    assert b"NAME" in stdout
    assert b"DESCRIPTION" in stdout

# Note - stacked parameterizations creates permutations
@pytest.mark.parametrize("cpu",
[
    ("armv9-a"),
    ("neoverse-n2"),
    ("neoverse-v2"),
    ("neoverse-n3"),
]
)
@pytest.mark.parametrize("argument",
[
    ("trb_wrap"),
    ("trcextout0"),
    ("trcextout1"),
    ("trcextout2"),
    ("trcextout3"),
]
)
def test_wperf_man_armv9_n23_v2_common_events(cpu, argument):
    """Test `wperf man` when prompted with invalid CPUs throws the necessary error"""
    cmd = f'wperf man {cpu}/{argument}'
    stdout, stderr = run_command(cmd.split())

    assert b"unexpected arg" not in stderr
    assert b"warning:" not in stderr
    assert b"CPU" in stdout
    assert b"NAME" in stdout
    assert b"DESCRIPTION" in stdout
