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
from jsonschema import validate
from common import run_command, is_json, check_if_file_exists, get_schema
from common import get_spe_version, wperf_event_is_available
from common_cpython import CPYTHON_EXE_DIR

#
# Skip whole module if SPE is not supported in this configuration
#

# Skip whole module if ustress is not supported by this CPU
spe_device = get_spe_version()
assert spe_device is not None
if not spe_device.startswith("FEAT_SPE"):
    pytest.skip(f"unsupported configuration: no SPE support in HW, see spe_device.version_name={spe_device}",
        allow_module_level=True)

## Is SPE enabled in `wperf` CLI?
if not wperf_event_is_available("arm_spe_0//"):
    pytest.skip(f"unsupported configuration: no SPE support in `wperf`, see spe_device.version_name={spe_device}",
        allow_module_level=True)

### Test cases

@pytest.mark.parametrize("spe_filters",
[
    ("x"),
    ("_"),
    ("asdasd"),
    ("b"),                  # Should be e.g. `b=0` or `b=1`
    ("ld"),                 # Should be e.g. `ld=0`
    ("st"),                 # Should be e.g. `st=0`
    ("load_filter"),        # Should be e.g. `load_filter=0`
    ("store_filter"),       # Should be e.g. `store_filter=0`
    ("branch_filter"),      # Should be e.g. `branch_filter=0`

    ("load_filter=0,x"),
    ("load_filter=0,_"),
    ("load_filter=0,asdasd"),
    ("load_filter=0,b"),                  # Should be e.g. `b=0` or `b=1`
    ("load_filter=0,ld"),                 # Should be e.g. `ld=0`
    ("load_filter=0,st"),                 # Should be e.g. `st=0`
    ("load_filter=0,load_filter"),        # Should be e.g. `load_filter=0`
    ("load_filter=0,store_filter"),       # Should be e.g. `store_filter=0`
    ("load_filter=0,branch_filter"),      # Should be e.g. `branch_filter=0`
]
)
def test_cpython_bench_spe_cli_incorrect_filter(spe_filters):
    """ Test `wperf record` with SPE CLI not OK filters, we expect:

    "incorrect SPE filter: '<spe_filters>' in <spe_filters>"

    """
    #
    # Run for CPython payload but we should fail when we hit CLI parser errors
    #
    cmd = f"wperf record -e arm_spe_0/{spe_filters}/ -c 4 --timeout 3 --json -- python_d.exe -c 10**10**100"
    _, stderr = run_command(cmd)

    assert b"unexpected arg" not in stderr
    assert b"incorrect SPE filter:" in stderr

@pytest.mark.parametrize("spe_filters",
[
    ("b=2"),
    ("ld=3"),
    ("st=4"),
    ("load_filter=10"),
    ("store_filter=0x2"),
    ("branch_filter=0xf1da"),
    ("b=1,load_filter=10"),
    ("b=0,store_filter=0x2"),
    ("b=1,branch_filter=0xf1da"),

    ("ld=1,min=0x10000"),
    ("load_filter=1,min=0x10000"),
    ("ld=1,min=0x10000,store_filter=0"),
    ("load_filter=1,min=0x10000,store_filter=0"),
    ("ld=1,min_latency=0x10000"),
    ("load_filter=1,min_latency=0x10000"),
    ("ld=1,min_latency=0x10000,store_filter=0"),
    ("load_filter=1,min_latency=0x10000,store_filter=0"),
]
)
def test_cpython_bench_spe_cli_filter_value_out_of_range(spe_filters):
    """ Test `wperf record` with SPE CLI filter value

    "SPE filter 'ts_enable' value out of range, use: 0-1"

    """
    #
    # Run for CPython payload but we should fail when we hit CLI parser errors
    #
    cmd = f"wperf record -e arm_spe_0/{spe_filters}/ -c 4 --timeout 3 --json -- python_d.exe -c 10**10**100"
    _, stderr = run_command(cmd)

    assert b"unexpected arg" not in stderr
    assert b"value out of range, use:" in stderr

@pytest.mark.parametrize("spe_filters",
[
    ("=0"),
    ("=1"),

    ("load_filter=0,=0"),
    ("load_filter=0,=1"),
]
)
def test_cpython_bench_spe_cli_incorrect_filter_name(spe_filters):
    """ Test `wperf record` with SPE CLI not OK filters, we expect:

    "incorrect SPE filter name: '<spe_filters>' in <spe_filters>"

    This error is for "empty" filter name.
    """
    #
    # Run for CPython payload but we should fail when we hit CLI parser errors
    #
    cmd = f"wperf record -e arm_spe_0/{spe_filters}/ -c 4 --timeout 3 --json -- python_d.exe -c 10**10**100"
    _, stderr = run_command(cmd)

    assert b"unexpected arg" not in stderr
    assert b"incorrect SPE filter name:" in stderr

@pytest.mark.parametrize("spe_filters",
[
    ("load_filter="),
    ("store_filter="),
    ("branch_filter="),
    ("ts_enable="),
    ("ld="),
    ("st="),
    ("b="),
    ("ts="),
    ("load_filter=a"),
    ("load_filter=one"),
    ("ts_enable=one"),

    ("load_filter=0,load_filter="),
    ("load_filter=0,store_filter="),
    ("load_filter=0,branch_filter="),
    ("load_filter=0,ld="),
    ("load_filter=0,st="),
    ("load_filter=0,b="),
    ("load_filter=0,ts="),
    ("load_filter=0,load_filter=a"),
    ("load_filter=0,load_filter=one"),
    ("ts_enable=1,ts_enable=one"),

    ("b=-1"),
    ("ld=-1"),
    ("st=-1"),
    ("load_filter=-10"),
    ("store_filter=-0x2"),
    ("branch_filter=-0xf"),

    ("load_filter=0,load_filter=-1"),
    ("load_filter=0,store_filter=-1"),
    ("load_filter=0,branch_filter=-1"),
    ("load_filter=0,ld=-1"),
    ("load_filter=0,st=-1"),
    ("load_filter=0,b=-1"),
]
)
def test_cpython_bench_spe_cli_incorrect_filter_value(spe_filters):
    """ Test `wperf record` with SPE CLI not OK filters, we expect:

    "incorrect SPE filter value: '<spe_filters>' in <spe_filters>"
    """
    #
    # Run for CPython payload but we should fail when we hit CLI parser errors
    #
    cmd = f"wperf record -e arm_spe_0/{spe_filters}/ -c 4 --timeout 3 --json -- python_d.exe -c 10**10**100"
    _, stderr = run_command(cmd)

    assert b"unexpected arg" not in stderr
    assert b"incorrect SPE filter value:" in stderr

@pytest.mark.parametrize("spe_filters,hot_symbol,hot_minimum,python_arg",
[
    ("",                           "x_mul:python312_d.dll", 65, "10**10**100"),
    ("load_filter=1",              "x_mul:python312_d.dll", 65, "10**10**100"),
    ("ts_enable=1",                "x_mul:python312_d.dll", 65, "10**10**100"),
    ("load_filter=1,ts_enable=1",  "x_mul:python312_d.dll", 65, "10**10**100"),
]
)
def test_cpython_bench_spe_hotspot(spe_filters, hot_symbol, hot_minimum, python_arg):
    """ Test `wperf record` for python_d.exe call for example `Googolplex` calculation.
        We will sample with SPE for one event + filters and we expect one hottest symbol
        with some minimum sampling %."""
    ## Execute benchmark
    pyhton_d_exe_path = os.path.join(CPYTHON_EXE_DIR, "python_d.exe")

    if not check_if_file_exists(pyhton_d_exe_path):
        pytest.skip(f"Can't locate CPython native executable in {pyhton_d_exe_path}")

    overheads = []  # Results of sampling of the symbol
    core_no = 3
    for _ in range(3):
        #
        # Run test N times and check for media sampling output
        #
        sleep(2)    # Cool-down the core

        cmd = f"wperf record -e arm_spe_0/{spe_filters}/ -c {core_no} --timeout 5 --json -- {pyhton_d_exe_path} -c {python_arg}"
        stdout, _ = run_command(cmd)
        core_no += 1

        # Sanity checks
        assert is_json(stdout), f"in {cmd}"
        json_output = json.loads(stdout)

        # Check sampling JSON output for expected functions
        assert "sampling" in json_output
        assert "counting" in json_output
        assert "sampling" in json_output["sampling"]
        assert "events" in json_output["sampling"]["sampling"]

        events = json_output["sampling"]["sampling"]["events"]  # List of dictionaries, for each event
        assert len(events) >= 1  # We expect at least one record

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
                if sample["symbol"] == hot_symbol:
                    overheads.append(int(sample["overhead"]))

    #
    # We want to see at least e.g. 70% of samples in e.g `x_mul`:
    #
    assert median(overheads) >= hot_minimum, f"expected {hot_minimum}% SPE sampling hotspot in {hot_symbol}, overheads={overheads}, cmd='{cmd}'"

@pytest.mark.parametrize("verbose",
[
    ("-v"),
    (""),
]
)
@pytest.mark.parametrize("event,spe_filters,python_arg",
[
    ("arm_spe_0", "",              "10**10**100"),
    ("arm_spe_0", "load_filter=1", "10**10**100"),
]
)
def test_cpython_bench_spe_json_schema(request, tmp_path, verbose, event, spe_filters, python_arg):
    """ Test SPE JSON output against scheme """
    ## Execute benchmark
    pyhton_d_exe_path = os.path.join(CPYTHON_EXE_DIR, "python_d.exe")

    if not check_if_file_exists(pyhton_d_exe_path):
        pytest.skip(f"Can't locate CPython native executable in {pyhton_d_exe_path}")

    test_path = os.path.dirname(request.path)
    file_path = tmp_path / 'test.json'

    cmd = f"wperf record -e {event}/{spe_filters}/ -c 2 {verbose} --timeout 5 --output {str(file_path)} -- {pyhton_d_exe_path} -c {python_arg}"
    _, _ = run_command(cmd.split())

    json_output = {}

    try:
        with open(str(file_path)) as json_file:
            json_output = json.loads(json_file.read())
        validate(instance=json_output, schema=get_schema("spe", test_path))
    except Exception as err:
        assert False, f"Unexpected {err=}, {type(err)=}, cmd='{cmd}'"

@pytest.mark.parametrize("verbose",
[
    ("-v"),
    (""),
]
)
@pytest.mark.parametrize("event,spe_filters,python_arg",
[
    ("arm_spe_0", "",              "10**10**100"),
    ("arm_spe_0", "load_filter=1", "10**10**100"),
]
)
def test_cpython_bench_spe_json_stdout_schema(request, tmp_path, verbose, event, spe_filters, python_arg):
    """ Test SPE JSON output against stdout scheme """
    ## Execute benchmark
    pyhton_d_exe_path = os.path.join(CPYTHON_EXE_DIR, "python_d.exe")

    if not check_if_file_exists(pyhton_d_exe_path):
        pytest.skip(f"Can't locate CPython native executable in {pyhton_d_exe_path}")

    test_path = os.path.dirname(request.path)

    cmd = f"wperf record -e {event}/{spe_filters}/ -c 3 {verbose} --timeout 5 --json -- {pyhton_d_exe_path} -c {python_arg}"
    stdout, _ = run_command(cmd.split())

    json_output = json.loads(stdout)

    try:
        validate(instance=json_output, schema=get_schema("spe", test_path))
    except Exception as err:
        assert False, f"Unexpected {err=}, {type(err)=}, cmd='{cmd}'"

@pytest.mark.parametrize("spe_filters,python_arg",
[
    ("",                   "10**10**100"),
    ("load_filter=1",      "10**10**100"),
    ("load_filter=1,st=1", "10**10**100"),
]
)
def test_cpython_bench_spe_consistency(request, tmp_path, spe_filters, python_arg):
    """ Test SPE JSON output against stdout scheme """
    ## Execute benchmark
    pyhton_d_exe_path = os.path.join(CPYTHON_EXE_DIR, "python_d.exe")

    if not check_if_file_exists(pyhton_d_exe_path):
        pytest.skip(f"Can't locate CPython native executable in {pyhton_d_exe_path}")

    cmd = f"wperf record -e arm_spe_0/{spe_filters}/ -c 2 --timeout 5 --json -- {pyhton_d_exe_path} -c {python_arg}"
    stdout, _ = run_command(cmd.split())

    json_output = json.loads(stdout)

    total_hits = 0
    for event in json_output["sampling"]["sampling"]["events"]:
        event_hits = 0
        #assert False, f"{event["samples"]=}"
        for sample in event["samples"]:
            event_hits = event_hits + sample["count"]
        total_hits = total_hits + event_hits

    total_hits_pmu = 0
    for events in json_output["counting"]["core"]["cores"][0]["Performance_counter"]:
        if events["event_name"] == "sample_filtrate":
            total_hits_pmu = events["counter_value"]

    assert total_hits == total_hits_pmu

    #
    # Check if `samples_generated` and `samples_dropped` match SPE PMU event values in counting
    #
    samples_generated = json_output["sampling"]["sampling"]["samples_generated"]
    samples_dropped = json_output["sampling"]["sampling"]["samples_dropped"]

    sample_filtrate = 0

    for events in json_output["counting"]["core"]["cores"][0]["Performance_counter"]:
        if events["event_name"] == "sample_filtrate":
            sample_filtrate = events["counter_value"]

    assert sample_filtrate >= 0
    assert samples_dropped >= 0
    assert samples_generated == sample_filtrate
