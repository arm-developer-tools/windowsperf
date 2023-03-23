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

""" Module testing basic output from `wperf stat -m <metric> ... """
import json
import re
import pytest
from common import run_command
from common import wperf_list, wperf_metric_events, wperf_metric_is_available

### Test cases


def test_metrics_exist():
    """ Test if metrics are available in `list` command """
    json_output = wperf_list()
    assert "Predefined_Events" in json_output
    assert "Predefined_Metrics" in json_output

    metrics = json_output["Predefined_Metrics"]

    for m in metrics:
        assert "Events" in m
        assert "Metric" in m
        assert len(m["Events"]) > 0
        assert len(m["Metric"]) > 0

@pytest.mark.parametrize("metric",
[
    ("dcache"),
    ("ddr_bw"),
    ("dtlb"),
    ("icache"),
    ("imix"),
    ("itlb"),
    ("l3_cache"),
]
)
def test_metric_(metric):
    """ Run known metrics and check if defined events are present """
    if not wperf_metric_is_available(metric):
        pytest.skip("unsupported configuration")
        return

    cmd = 'wperf stat -m ' + metric + ' -c 1 sleep 1'
    stdout, _ = run_command(cmd.split())

    events = wperf_metric_events(metric)
    assert events

    # Event names in pretty table
    if events:
        for event in events.split(','):
            event = event.strip("{}/")   # Some events may be part of groups
            if "/" in event:
                event = event.split('/')[1]    # e.g. "/dsu/<event_name>" -> "<event_name>"
            assert re.search(b'[\\d]+[\\s]+%s[\\s]+0x[0-9a-f]+' % str.encode(event), stdout)
