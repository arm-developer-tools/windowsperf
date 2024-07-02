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

"""Module is testing `wperf record` with CPython executables in debug mode."""
import os
import json
from statistics import median
from time import sleep
import pytest
from common import run_command, is_json, check_if_file_exists
from common import get_spe_version, wperf_event_is_available
from common_cpython import CPYTHON_EXE_DIR

### Test cases

@pytest.mark.parametrize("EVENT,SPE_FILTERS,HOT_SYMBOL,HOT_MINIMUM,PYTHON_ARG",
[
    ("arm_spe_0", "", "x_mul:python312_d.dll", 65, "10**10**100"),
]
)
def test_cpython_bench_spe_hotspot(EVENT,SPE_FILTERS,HOT_SYMBOL,HOT_MINIMUM,PYTHON_ARG):
    """ Test `wperf record` for python_d.exe call for example `Googolplex` calculation.
        We will sample with SPE for one event + filters and we expect one hottest symbol
        with some minimum sampling %."""

    ## Do we have FEAT_SPE
    spe_device = get_spe_version()
    assert spe_device is not None
    if not spe_device.startswith("FEAT_SPE"):
        pytest.skip(f"no SPE support in HW, see spe_device.version_name={spe_device}")

    ## Is SPE enabled in `wperf` CLI?
    if not wperf_event_is_available("arm_spe_0//"):
        pytest.skip(f"no SPE support in `wperf`, see spe_device.version_name={spe_device}")

    ## Execute benchmark
    pyhton_d_exe_path = os.path.join(CPYTHON_EXE_DIR, "python_d.exe")

    if not check_if_file_exists(pyhton_d_exe_path):
        pytest.skip(f"Can't locate CPython native executable in {pyhton_d_exe_path}")

    overheads = []  # Results of sampling of the symbol
    #
    # Run test N times and check for media sampling output
    #
    sleep(2)    # Cool-down the core

    cmd = f"wperf record -e {EVENT}/{SPE_FILTERS}/ -c 3 --timeout 5 --json -- {pyhton_d_exe_path} -c {PYTHON_ARG}"
    stdout, _ = run_command(cmd)

    # Sanity checks
    assert is_json(stdout), f"in {cmd}"
    json_output = json.loads(stdout)

    # Check sampling JSON output for expected functions
    assert "sampling" in json_output
    assert "events" in json_output["sampling"]

    events = json_output["sampling"]["events"]  # List of dictionaries, for each event
    assert len(events) >= 1  # We expect at least one record

    overheads = []
    """
    "events": [
        {
            "type": "BRANCH_OR_EXCEPTION-UNCONDITIONAL-DIRECT/retired",
            "samples": [
                {
                    "overhead": 100,
                    "count": 7,
                    "symbol": "x_mul:python312_d.dll"
                }
            ],
            "interval": 0,
            "printed_sample_num": 1,
            "annotate": []
        },
    """
    for event in events:    # Gather all symbol overheads for all events for given symbol
        for sample in event["samples"]:
            if sample["symbol"] == HOT_SYMBOL:
                overheads.append(int(sample["overhead"]))

    #
    # We want to see at least e.g. 70% of samples in e.g `x_mul`:
    #
    assert median(overheads) >= HOT_MINIMUM, f"expected {HOT_MINIMUM}% SPE sampling hotspot in {HOT_SYMBOL}, overheads={overheads}, cmd={cmd}"
