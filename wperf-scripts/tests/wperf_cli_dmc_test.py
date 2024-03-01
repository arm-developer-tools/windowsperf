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
These are basic DMC test. Please note that DMC (Dynamic Memory Controller)
aka DDR is not present or supported by WindowsPerf on all platforms.

Requires:
    pytest -    install with `pip install -U pytest`

Usage:
    >pytest wperf_cli_cmd_test.py

"""

import json
import pytest
from common import run_command, is_json
from common import wperf_metric_is_available


@pytest.mark.parametrize("metric",
[
    ("ddr_bw"),     # Example of supported DMC metric
]
)
def test_wperf_dmc_json_output(metric):
    """ Simple smoke test to make sure we can export DMC only data to JSON.
        Note: we do not have to define -c when we use DMC as DMC is not
              using CPU cores and has its own channels.
    """
    if not wperf_metric_is_available(metric):
        pytest.skip(f"unsupported metric: {metric}")

    cmd = f"wperf stat -m {metric} --json --timeout 3"
    stdout, _ = run_command(cmd)
    assert is_json(stdout)

    #
    # Smoke-tests for JSON output
    # These tests below are not any metric specific!
    #
    json_output = json.loads(stdout)

    assert "dmc" in json_output
    assert "pmu" in json_output["dmc"]
    assert "PMU_Performance_Counters" in json_output["dmc"]["pmu"]

    """
    "PMU_Performance_Counters": [
        {
            "pmu_id": "dmc 0",
            "counter_value": 353337,
            "event_name": "rdwr",
            "event_idx": "0x0012",
            "event_note": "e,ddr_bw"
        },
    """
    PMU_Performance_Counters = json_output["dmc"]["pmu"]["PMU_Performance_Counters"]
    for counter in PMU_Performance_Counters:
        assert "pmu_id" in counter
        assert "counter_value" in counter
        assert "event_name" in counter
        assert "event_idx" in counter
        assert "event_note" in counter

        #
        # Check if this is INTEGER
        #
        assert isinstance(counter["counter_value"], int)    # it's a dec integer value
        assert int(ounter["event_idx"], 16)                 # it's a hex string
