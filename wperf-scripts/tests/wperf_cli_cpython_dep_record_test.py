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
from common_cpython import CPYTHON_EXE_DIR


### Test cases

@pytest.mark.parametrize("EVENT,EVENT_FREQ,HOT_SYMBOL,HOT_MINIMUM,PYTHON_ARG",
[
    ("ld_spec", 10000, "x_mul:python312_d.dll", 65, "10**10**100"),
]
)
def test_cpython_bench_record_hotspot(EVENT,EVENT_FREQ,HOT_SYMBOL,HOT_MINIMUM,PYTHON_ARG):
    """ Test `wperf record` for python_d.exe call for example `Googolplex` calculation.
        We will sample for one event + sampling frequency and we expect one hottest symbol
        with some minimum sampling %."""

    ## Execute benchmark
    python_d_exe_path = os.path.join(CPYTHON_EXE_DIR, "python_d.exe")

    if not check_if_file_exists(python_d_exe_path):
        pytest.skip(f"Can't locate CPython native executable in {python_d_exe_path}")

    overheads = []  # Results of sampling of the symbol
    #
    # Run test N times and check for media sampling output
    #
    for _ in range(3):
        sleep(2)    # Cool-down the core

        cmd = f"wperf record -e {EVENT}:{EVENT_FREQ} -c 7 --timeout 5 --json -- {python_d_exe_path} -c {PYTHON_ARG}"
        stdout, _ = run_command(cmd)

        # Sanity checks
        assert is_json(stdout), f"in {cmd}"
        json_output = json.loads(stdout)

        # Check sampling JSON output for expected functions
        assert "sampling" in json_output
        assert "events" in json_output["sampling"]

        events = json_output["sampling"]["events"]  # List of dictionaries, for each event
        assert len(events) == 1  # We expect one record for e.g. `ld_spec`

        evt = json_output["sampling"]["events"][0]  # e.g. `ld_spec` data
        assert EVENT in evt["type"]

        samples = evt["samples"]

        def find_sample(samples, symbol):
            """ Find sample in samples and return it when we can find it for given `symbol`.
            See:

            "events": [
                {
                    "type": "ld_spec",
                    "samples": [
                        {
                            "overhead": 81.6406,
                            "count": 418,
                            "symbol": "x_mul:python312_d.dll"
                        },
                        {
                            "overhead": 4.88281,
                            "count": 25,
                            "symbol": "v_iadd:python312_d.dll"
                        },
            """
            for sample in samples:
                if sample["symbol"] == symbol:
                    return sample
            return None

        # {
        #     "overhead": 81.6406,
        #     "count": 418,
        #     "symbol": "x_mul:python312_d.dll"
        # },

        # We expect in events.samples[0] hottest sample (which we want to check for)
        hotest_symbol = samples[0]
        symbol_overhead = hotest_symbol["overhead"]
        symbol_count = hotest_symbol["count"]
        symbol_name = hotest_symbol["symbol"]

        if find_sample(samples, HOT_SYMBOL) is False:
            pytest.skip(f"{python_d_exe_path} hottest function sampled: '{symbol_name}' count={symbol_count} overhead={symbol_overhead}, expected '{HOT_SYMBOL}' -- sampling mismatch")

        evt_sample = find_sample(samples, HOT_SYMBOL)
        assert evt_sample is not None, f"Can't find `{HOT_SYMBOL}` symbol in sampling output!"
        overheads.append(evt_sample["overhead"])    # Store results for median calculation

    #
    # We want to see at least e.g. 70% of samples in e.g `x_mul`:
    #
    assert median(overheads) >= HOT_MINIMUM, f"expected {HOT_MINIMUM}% sampling hotspot in {HOT_SYMBOL}, overheads={overheads}"

def test_cpython_bench_record_time():
    """ Test if we print `seconds time elapsed` with sampling.  """
    python_d_exe_path = os.path.join(CPYTHON_EXE_DIR, "python_d.exe")

    if not check_if_file_exists(python_d_exe_path):
        pytest.skip(f"Can't locate CPython native executable in {python_d_exe_path}")

    cmd = f"wperf record -e ld_spec:100000 -c 1 --timeout 1 --annotate -- {python_d_exe_path} -c 10**10**100"
    stdout, _ = run_command(cmd)

    assert b'seconds time elapsed' in stdout

@pytest.mark.parametrize("arg",
[
   ("x_mul"),
   ("^x_"),
   ("_mul$"),
   ("X_MUL"),
   ("^x_mul$"),
]
)
def test_cpython_bench_record_symbol(arg):
    """ Test sampling filtering for record with `-s <symbol>` """
    python_d_exe_path = os.path.join(CPYTHON_EXE_DIR, "python_d.exe")

    if not check_if_file_exists(python_d_exe_path):
        pytest.skip(f"Can't locate CPython native executable in {python_d_exe_path}")
        
    cmd = f"wperf record -e ld_spec:100000 -c 1 --symbol \"{arg}\" --timeout 3 -- {python_d_exe_path} -c 10**10**100"
    stdout, _ = run_command(cmd)
    
    assert b'x_mul' in stdout
    assert b'x_mul:python' in stdout
