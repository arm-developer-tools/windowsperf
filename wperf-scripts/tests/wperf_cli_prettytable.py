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
This script runs simple wperf CLI tests.

Requires:
    pytest -    install with `pip install -U pytest`

Usage:
    >py.test wperf_cli_test.py

"""

import json
import os
import re
from common import run_command
from common import wperf_metric_is_available, wperf_metric_events_list

import pytest

@pytest.mark.parametrize("events,cores,metric,sleep",
[
    (b"l1i_cache",                                                                          "0", "", "1"),
    (b"inst_spec,vfp_spec,ASE_SPEC,dp_spec",                                                "1", "", "1"),
    (b"inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec,br_immed_spec,crypto_spec",      "2", "", "1"),

    (b"",          "0", "imix", "3"),
    (b"l1i_cache", "0", "imix", "4"),
]
)
def test_wperf_prettytable_format(events,cores,metric,sleep):
    """ Test `wperf stat` command line output.

        Use pytest.mark.parametrize to set up below command line switches:

            ( -e <events>, -c <cores>, -m <metric>, sleep <value> )
    """
    cmd = 'wperf stat'.split()
    if events:
        cmd += ['-e', events]
    if cores:
        cmd += ['-c', cores]
    if metric and wperf_metric_is_available(metric):
        cmd += ['-m', metric]
    if sleep:
        cmd += ['sleep', str(sleep)]

    stdout, _ = run_command(cmd)

    #
    # Columns we see when we print `wperf stat`
    #
    COLS = [b'counter value' , b'event name', b'event idx', b'event note']
    MCOLS = [b'multiplexed' , b'scaled value']  # These columns are added when multiplexing

    # Find all column names in one row
    rstr_events = b'[\\s]+'         # Regex for all column names on one row
    rstr_underline = b'[\\s]+'      # Regex for all columns' underline ("=====") on one row
    for col in COLS:
        rstr_events += col + b'[\\s]+'
        rstr_underline += b'=' * len(col) + b'[\\s]+'

    MULTIPLEXED = b', multiplexed' in stdout

    if MULTIPLEXED:  # Pretty table basic columns (multiplexing)
        for col in MCOLS:
            rstr_events += col + b'[\\s]+'
            rstr_underline += b'=' * len(col) + b'[\\s]+'

    assert re.search(rstr_events, stdout)  # Check for all columns in one line
    assert re.search(rstr_underline, stdout)  # Check for all columns' underline ("======") in one line

    #
    # Events in each line
    #

    all_events = list()
    # If event list is not empty add events
    if events:
        all_events.extend(events.split(b","))
    # If `metric` is specified also include metric events in the list
    metric_events = wperf_metric_events_list(metric)
    if metric_events:
        all_events.extend(list(map(str.encode, metric_events))) # wperf_metric_events_list() returns type(str), we need type(byte)

    for event in all_events:
        event = event.lower()
        rstr = b'[\\d,]+\\s+%s\\s+0x[0-9a-f]+\\s+[a-z,0-9]+' % event

        if MULTIPLEXED:
            # E.g. "7,139,186  inst_spec      0x1b       e                  9/11     8,725,671"
            assert re.search(rstr + b'\\s+\\d+\\/\\d+\\s+[\\d,]+', stdout)
        else:
            # E.g. "7,139,186  inst_spec      0x1b       e"
            assert re.search(rstr, stdout)
