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
import re
from common import run_command, is_json, check_if_file_exists
from common import get_result_from_test_results
from common import wperf_list

### Test cases for -E

def test_wperf_padding_1_event():
    """ Test one event, no multiplexing """
    cmd = 'wperf test -e r80,r81,r82,r82,r83 --json'
    stdout, _ = run_command(cmd.split())
    json_output = json.loads(stdout)

    evt_core_index = get_result_from_test_results(json_output, "ioctl_events[EVT_CORE].index").split(',')
    evt_core_note = get_result_from_test_results(json_output, "ioctl_events[EVT_CORE].note").split(',')

    assert (len(evt_core_index) == 5)       #   128,129,130,130,131
    assert (len(evt_core_note) == 5)        #   (e),(e),(e),(e),(e)
    assert (all(elem == '(e)' for elem in evt_core_note))    #   only '(e)' in note

    assert ('128' in evt_core_index)
    assert ('129' in evt_core_index)
    assert ('130' in evt_core_index)
    assert ('131' in evt_core_index)

def test_wperf_E_list_add_event():
    """ Test if we can add new event 0xffaa names 'name1' """
    cmd = 'wperf list -E name1:0xffaa --json'
    stdout, _ = run_command(cmd.split())
    j = json.loads(stdout)

    for e in j["Predefined_Events"]:
        if e["Alias_Name"] == "name1":
            # Check if we've replaced existing event
            assert (e["Raw_Index"] == "0xffaa")
            assert (e["Event_Type"] != "[core PMU event]")  # Not replacing existing event
            assert (e["Event_Type"] == "[core PMU event*]") # Only new event with '*'


def test_wperf_E_list_add_event_verbose():
    """ Test if we can add new event 0xffaa names 'name1' """
    cmd = 'wperf list -E name1:0xffaa -v --json'
    stdout, _ = run_command(cmd.split())
    j = json.loads(stdout)

    for e in j["Predefined_Events"]:
        if e["Alias_Name"] == "name1":
            # Check if we've replaced existing event
            assert (e["Raw_Index"] == "0xffaa")
            assert (e["Event_Type"] != "[core PMU event]")  # Not replacing existing event
            assert (e["Event_Type"] == "[core PMU event*]") # Only new event with '*'
            assert (e["Description"] == "<extra event>")    # Extra in verbose mode

def test_wperf_E_list_add_2_event():
    """ Test if we can add new events:
    name1:0xffaa and
    xyz:0xeeee event.
    """
    cmd = 'wperf list -E name1:0xffaa,xyz:0xeeee --json'
    stdout, _ = run_command(cmd.split())
    j = json.loads(stdout)

    for e in j["Predefined_Events"]:
        if e["Alias_Name"] == "name1":
            # Check if we've replaced existing event
            assert (e["Raw_Index"] == "0xffaa")
            assert (e["Event_Type"] != "[core PMU event]")  # Not replacing existing event
            assert (e["Event_Type"] == "[core PMU event*]") # Only new event with '*'

    for e in j["Predefined_Events"]:
        if e["Alias_Name"] == "xyz":
            # Check if we've replaced existing event
            assert (e["Raw_Index"] == "0xeeee")
            assert (e["Event_Type"] != "[core PMU event]")  # Not replacing existing event
            assert (e["Event_Type"] == "[core PMU event*]") # Only new event with '*'
