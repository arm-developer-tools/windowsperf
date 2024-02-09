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


import json
from common import get_result_from_test_results
from common import run_command, wperf_test_get_key_val

import pytest

@pytest.mark.parametrize("cores",
[
    ("0,1"),
    ("1,0"),
    ("1,2"),
    ("1,2,3"),
    ("1,1,2"),
    ("0,1,1,2,3,5"),
    ("7,6,5,4,3,2,1"),
]
)
def test_record_many_cores_selected(cores):
    """ It is not allowed to specify more than one core for sampling. """
    _, stderr = run_command(f"wperf record -c {cores} -- TEST")

    assert b"you can specify only one core for sampling" in stderr

@pytest.mark.parametrize("cores",
[
    ("0,1"),
    ("1,0"),
    ("1,2"),
    ("1,2,3"),
    ("1,1,2"),
    ("0,1,1,2,3,5"),
    ("7,6,5,4,3,2,1"),
]
)
def test_record_many_cores_selected_ext(cores):
    """ It is not allowed to specify more than one core for sampling. """
    _, stderr = run_command(f"wperf record -v -e ld_spec:100000 -c {cores} --timeout 30 -- python_d.exe -c 10**10**100")

    assert b"you can specify only one core for sampling" in stderr

def test_record_pe_file_not_specified():
    """ Test for error if we can't deduce PE file name or name missing.
    """

    gpc_num = wperf_test_get_key_val("PMU_CTL_QUERY_HW_CFG", "gpc_num")
    gpc_num = int(gpc_num, 16)  # it's a hex string e,g,. 0x0005

    #
    ## Event number equal to available GPCs, we should error about PE file not deduced
    #

    events = ["ld_spec:20000"] * gpc_num
    events = ",".join(events)
    cmd = f"wperf record -e {events}"

    # We will check this in CLI parsing, so we do not have to provide other parameters like `-c`
    _, stderr = run_command(cmd.split())
    assert b"no pid or process name specified, sample address are not de-ASLRed" in stderr, f"cmd='{cmd}'"

@pytest.mark.parametrize("gpc_name,error_msg",
[
    ("total_gpc_num", b"number of events requested exceeds the number of hardware PMU counters"),
]
)
def test_record_too_many_events_to_sample_total_gpc_num(gpc_name,error_msg):
    """ This test checks we guard against tool many events in sampling.
        We do not multiplex in sampling users can only specify:

            N events, where N <= all available GPCs.

        Block user from request events while sampling that require more
        than the available number of GPCs.

        Note: test `test_record_pe_file_not_specified` checks for len(events) == gpc_num, so OK case.
        This test checks (in this order!) for:
        - len(events) > total_gpc_num so we should report error with too many events specified.
        - len(events) > gpc_num so we should report error with too many events specified.
    """

    gpc_val = wperf_test_get_key_val("PMU_CTL_QUERY_HW_CFG", gpc_name)
    gpc_val = int(gpc_val, 16)  # it's a hex string e,g,. 0x0005

    #
    ## One more event than available GPC number
    #

    events = " vfp_spec:100000" + (",ld_spec:20000" * gpc_val)
    cmd = f"wperf record -e {events}"

    # We will check this in CLI parsing, so we do not have to provide other parameters like `-c`
    _, stderr = run_command(cmd.split())
    assert error_msg in stderr, f"gpc_name={gpc_name} cmd='{cmd}'"

@pytest.mark.parametrize("gpc_name,error_msg",
[
    ("gpc_num",       b"number of events requested exceeds the number of free hardware PMU counters"),
]
)
def test_record_too_many_events_to_sample_gpc_num(gpc_name,error_msg):
    """ This test checks we guard against tool many events in sampling.
        We do not multiplex in sampling users can only specify:

            N events, where N <= all available GPCs.

        Block user from request events while sampling that require more
        than the available number of GPCs.

        Note: test `test_record_pe_file_not_specified` checks for len(events) == gpc_num, so OK case.
        This test checks (in this order!) for:
        - len(events) > total_gpc_num so we should report error with too many events specified.
        - len(events) > gpc_num so we should report error with too many events specified.
    """
    gpc_val = wperf_test_get_key_val("PMU_CTL_QUERY_HW_CFG", gpc_name)
    gpc_val = int(gpc_val, 16)  # it's a hex string e,g,. 0x0005

    gpc_total = wperf_test_get_key_val("PMU_CTL_QUERY_HW_CFG", "total_gpc_num")
    gpc_total = int(gpc_total, 16)  # it's a hex string e,g,. 0x0005

    assert gpc_val <= gpc_total     # Sanity check

    if gpc_val == gpc_total:
        pytest.skip(f"this test is applicable only if `gpc_num` < `total_gpc_num`"
                    f", now: gpc_num={gpc_val} and total_gpc_num={gpc_total}")

    #
    ## One more event than available GPC number
    #

    events = " vfp_spec:100000" + (",ld_spec:20000" * gpc_val)
    cmd = f"wperf record -e {events}"

    # We will check this in CLI parsing, so we do not have to provide other parameters like `-c`
    _, stderr = run_command(cmd.split())
    assert error_msg in stderr, f"gpc_name={gpc_name} cmd='{cmd}'"
