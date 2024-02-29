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

"""
This script runs simple wperf stat test to determine if counting values
from core PMU counters are sane. Both standard CLI output and JSON output
are checked.

Requires:
    pytest -    install with `pip install -U pytest`

Usage:
    >pytest wperf_cli_stat_value_test.py

"""

import json
import os
import re
from itertools import permutations
import pytest
from fixtures import fixture_is_enough_GPCs
from common import run_command, is_json


N_CORES = os.cpu_count()
ALL_CORES = ','.join(str(cores) for cores in range(0, N_CORES))

### Test cases

def wperf_stat_values_perm_events(events, timeout, cores):
    """ Generates pytest.mark.parametrize lists with parametrized tests.
        1) Create all permutations of `events` comman separated string.
        2) Return tuple matching input for `test_wperf_stat_values_for_inst_spec`.
        3) Return all permutations of `events` as string (so it can be
           printed in name of the test).
    """
    ret = []
    for p in permutations(events.split(",")):
        ret.append((",".join(p), timeout, cores))
    return ret


@pytest.mark.parametrize("events_to_count,timeout,cores",
    wperf_stat_values_perm_events("ld_spec,st_spec,inst_spec", 3, 0) +
    wperf_stat_values_perm_events("ld_spec,st_spec,vfp_spec,inst_spec", 3, 0) +

    wperf_stat_values_perm_events("ld_spec,st_spec,inst_spec", 3, ALL_CORES) +
    wperf_stat_values_perm_events("ld_spec,st_spec,vfp_spec,inst_spec", 3, ALL_CORES)
)
@pytest.mark.xfail("not fixture_is_enough_GPCs(5)", reason="this test requires at least 5 GPCs")
def test_wperf_stat_json_values_for_inst_spec(events_to_count,timeout,cores):
    """ Test if events in a only group are following common sense size. E.g.
    * inst_spec >= ld_spec
    * inst_spec >= st_spec
    * inst_spec >= ld_spec + st_spec
    * inst_spec >= vfp_spec - total number of FP speculation must me less that all instructions ran.
    """
    events_to_count = events_to_count.split(",")

    assert "inst_spec" in events_to_count   #   This test require 'inst_spec' event

    #
    # Try all permutations (order of) the events so that we make sure position
    # of the event doesn't matter.
    #
    events = ",".join(events_to_count)
    events = "{" + events + "}"

    cmd = f"wperf stat -e {events} -c {cores} --timeout {timeout} --json"
    stdout, _ = run_command(cmd.split())
    assert is_json(stdout), f"cmd={cmd}"

    json_output = json.loads(stdout)

    #
    # Sanity check
    #
    assert "core" in json_output, f"cmd={cmd}"
    assert json_output["core"]["cores"], f"cmd={cmd}"

    total_counts = 0
    for core in json_output["core"]["cores"]:
        #
        # Sanity checks
        #
        assert len(core["Performance_counter"]) == len(events_to_count) + 1 # +1 for FIXED counter that is always there

        event_counts = {}   # Look-aside buffer for event names and their values
        for pc in core["Performance_counter"]:
            #
            # Gather all the data about all events
            #
            event_name = pc["event_name"]
            event_count = pc["counter_value"]

            event_counts[event_name] = event_count
            total_counts += event_count                 # Accumulate all counter values

        #
        # Proper test checks
        #
        for e in events_to_count:    # Check if all event counts are smaller or equal to 'insn-spec'
            if e != "inst_spec":
                inst_spec_val = event_counts["inst_spec"]
                e_val = event_counts[e]

                #
                # If there were inst_spec on the core:
                #   - make sure count(inst_spec) >= count(event)
                # else
                #   - all event count values must be zero.
                #
                if inst_spec_val > 0:
                    assert inst_spec_val >= e_val, f"inst_spec={inst_spec_val} <= {e}={e_val}, events={events}, cmd={cmd}, stdout={stdout}"
                else:
                    assert e_val == 0, f"event {e}={e_val} counting value should be zero if inst_spec={inst_spec_val}, events={events}, cmd={cmd}"

    assert total_counts > 0, "All counter values were zeros, total_counts={total_counts}"     # Something is wrong if all event counters are ZERO


@pytest.mark.parametrize("events_to_count,timeout,core",
    wperf_stat_values_perm_events("ld_spec,st_spec,inst_spec", 3, 0) +
    wperf_stat_values_perm_events("ld_spec,st_spec,inst_spec", 3, 1) +
    wperf_stat_values_perm_events("ld_spec,st_spec,inst_spec", 3, 2) +
    wperf_stat_values_perm_events("ld_spec,st_spec,inst_spec", 3, 3) +

    wperf_stat_values_perm_events("ld_spec,st_spec,vfp_spec,inst_spec", 3, 0) +
    wperf_stat_values_perm_events("ld_spec,st_spec,vfp_spec,inst_spec", 3, 1) +
    wperf_stat_values_perm_events("ld_spec,st_spec,vfp_spec,inst_spec", 3, 2) +
    wperf_stat_values_perm_events("ld_spec,st_spec,vfp_spec,inst_spec", 3, 3)
)
@pytest.mark.xfail("not fixture_is_enough_GPCs(5)", reason="this test requires at least 5 GPCs")
def test_wperf_stat_cli_values_for_inst_spec(events_to_count,timeout,core):
    """ Test if events in a only group are following common sense size. E.g.
    * inst_spec >= ld_spec
    * inst_spec >= st_spec
    * inst_spec >= ld_spec + st_spec
    * inst_spec >= vfp_spec - total number of FP speculation must me less that all instructions ran.

    Note: this function uses byte-strings to conform to CLI byte-like output.
    """
    events_to_count = events_to_count.split(",")

    assert "inst_spec" in events_to_count   #   This test require 'inst_spec' event
    assert int(core), f"specify only one core! `core` argument must be INTEGER, now is core={core}"

    #
    # Try all permutations (order of) the events so that we make sure position
    # of the event doesn't matter.
    #
    events = ",".join(events_to_count)
    events = "{" + events + "}"

    cmd = f"wperf stat -e {events} -c {core} --timeout {timeout}"
    stdout, _ = run_command(cmd.split())

    events_in_cli = re.findall(rb"([0-9,]+)  ([a-z_]+)", stdout)

    #
    # Store events with counts in this dictionary for processing
    #
    total_counts = 0
    event_counts = {}
    for count, name in events_in_cli:
        count = int(count.replace(b",", b""))
        event_counts[name] = count
        total_counts += count       # Store total count

    for e in events_to_count:       # Check if all event counts are smaller or equal to 'inst_spec'
        e = str.encode(e)           # Make sure to use byte-strings for event names
        if e != b"inst_spec":
            inst_spec_val = event_counts[b"inst_spec"]
            e_val = event_counts[e]

            #
            # If there were inst_spec on the core:
            #   - make sure count(inst_spec) >= count(event)
            # else
            #   - all event count values must be zero.
            #
            if inst_spec_val > 0:
                assert inst_spec_val >= e_val, f"inst_spec={inst_spec_val} <= {e}={e_val}, events={events}, cmd={cmd}, stdout={stdout}"
            else:
                assert e_val == 0, f"event {e}={e_val} counting value should be zero if inst_spec={inst_spec_val}, events={events}, cmd={cmd}"

    assert total_counts > 0, "All counter values were zeros, total_counts={total_counts}"     # Something is wrong if all event counters are ZERO
