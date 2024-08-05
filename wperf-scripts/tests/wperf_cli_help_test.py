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


"""Module is testing `wperf --help` features."""
import pytest
from common import run_command
from common import get_spe_version, wperf_event_is_available

### Test cases

def test_wperf_cli_help_format():
    """ wperf --help format smoke test. """
    cmd = "wperf --help"
    stdout, _ = run_command(cmd)

    # Help section captions
    assert b"NAME:" in stdout
    assert b"SYNOPSIS:" in stdout
    assert b"OPTIONS:" in stdout
    assert b"OPTIONS aliases:" in stdout
    assert b"EXAMPLES:" in stdout

    # wperf commands
    assert b"wperf stat " in stdout
    assert b"wperf record " in stdout
    assert b"wperf sample " in stdout
    assert b"wperf list " in stdout
    assert b"wperf test " in stdout
    assert b"wperf detect " in stdout
    assert b"wperf man " in stdout

    # Most popular command line options
    assert b"-h, --help"                b"\r\n" in stdout
    assert b"--version"                 b"\r\n" in stdout
    assert b"-v, --verbose"             b"\r\n" in stdout
    assert b"-q"                        b"\r\n" in stdout
    assert b"-e"                        b"\r\n" in stdout
    assert b"-m"                        b"\r\n" in stdout
    assert b"--timeout"                 b"\r\n" in stdout
    assert b"-t"                        b"\r\n" in stdout
    assert b"-i"                        b"\r\n" in stdout
    assert b"-n"                        b"\r\n" in stdout
    assert b"--annotate"                b"\r\n" in stdout
    assert b"--disassemble"             b"\r\n" in stdout
    assert b"--image_name"              b"\r\n" in stdout
    assert b"--pe_file"                 b"\r\n" in stdout
    assert b"--pdb_file"                b"\r\n" in stdout
    assert b"--sample-display-long"     b"\r\n" in stdout
    assert b"--sample-display-row"      b"\r\n" in stdout
    assert b"--record_spawn_delay"      b"\r\n" in stdout
    assert b"--force-lock"              b"\r\n" in stdout
    assert b"-c, --cpu"                 b"\r\n" in stdout
    assert b"-k"                        b"\r\n" in stdout
    assert b"--dmc"                     b"\r\n" in stdout
    assert b"-C"                        b"\r\n" in stdout
    assert b"-E"                        b"\r\n" in stdout
    assert b"--json"                    b"\r\n" in stdout
    assert b"--output, -o"              b"\r\n" in stdout
    assert b"--config"                  b"\r\n" in stdout

def test_wperf_cli_help_spe_usage_included():
    """ Test for SPE in --help usage string
    """
    spe_device = get_spe_version()
    assert spe_device is not None
    if not spe_device.startswith("FEAT_SPE"):
        pytest.skip(f"no SPE support in HW, see spe_device.version_name={spe_device}")

    ## Is SPE enabled in `wperf` CLI?
    if not wperf_event_is_available("arm_spe_0//"):
        pytest.skip(f"no SPE support in `wperf`, see spe_device.version_name={spe_device}")

    cmd = "wperf --help"
    stdout, _ = run_command(cmd)

    assert b"arm_spe_0/" in stdout
    assert b"wperf record -e arm_spe_0/" in stdout
