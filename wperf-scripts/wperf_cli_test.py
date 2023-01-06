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
import subprocess

import pytest

N_CORES = os.cpu_count()

### Test runner code

def is_json(str_to_test):
    """ Test if string is in JSON format. """
    try:
        json.loads(str_to_test)
    except ValueError:
        return False
    return True

def run_command(args):
    """ Run command and capture stdout and stderr for parsing. """
    process = subprocess.Popen(args,
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE)
    stdout, stderr = process.communicate()
    return stdout, stderr


### Test cases

def test_wperf_test_json():
    """ Test `wperf test` JSON output  """
    cmd = 'wperf test -json'
    stdout, _ = run_command(cmd.split())
    assert is_json(stdout)

def test_wperf_list_json():
    """ Test `wperf list` JSON output  """
    cmd = 'wperf list -json'
    stdout, _ = run_command(cmd.split())
    assert is_json(stdout)

@pytest.mark.parametrize("events,cores,metric,sleep",
[
    (b"inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec", "0", "", 1),
    (b"inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec", "0,1", "", 1),
    (b"inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec", ','.join(str(cores) for cores in range(0, N_CORES)), "", 1),

    (b"inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec,br_immed_spec,crypto_spec", "0", "", 1),
    (b"inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec,br_immed_spec,crypto_spec", "0,1", "", 1),
    (b"inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec,br_immed_spec,crypto_spec", ','.join(str(cores) for cores in range(0, N_CORES)), "", 1),
]
)
def test_wperf_stat_json(events,cores,metric,sleep):
    """ Test `wperf stat -json` command line output.

        Use pytest.mark.parametrize to set up below command line switches:

            ( -e <events>, -c <cores>, -m <metric>, sleep <value> )
    """
    cmd = 'wperf stat'.split()
    if events:
        cmd += ['-e', events]
    if cores:
        cmd += ['-c', cores]
    if metric:
        cmd += ['-m', metric]
    if sleep:
        cmd += ['sleep', str(sleep)]

    cmd += ['-json']

    stdout, _ = run_command(cmd)
    assert is_json(stdout)

@pytest.mark.parametrize("events,cores,metric,sleep",
[
    (b"inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec", "0", "", 1),
    (b"inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec", "0,1", "", 1),
    (b"inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec", ','.join(str(cores) for cores in range(0, N_CORES)), "", 1),
    (b"inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec", ','.join(str(cores) for cores in range(0, N_CORES,N_CORES//2)), "", 1),
    (b"inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec", ','.join(str(cores) for cores in range(0, N_CORES,N_CORES//6)), "", 1),
    (b"inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec", ','.join(str(cores) for cores in range(0, N_CORES,N_CORES//8)), "", 1),

    (b"inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec,br_immed_spec,crypto_spec", "0", "", 1),
    (b"inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec,br_immed_spec,crypto_spec", "0,1", "", 1),
    (b"inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec,br_immed_spec,crypto_spec", ','.join(str(cores) for cores in range(0, N_CORES)), "", 1),
    (b"inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec,br_immed_spec,crypto_spec", ','.join(str(cores) for cores in range(0, N_CORES,N_CORES//2)), "", 1),
    (b"inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec,br_immed_spec,crypto_spec", ','.join(str(cores) for cores in range(0, N_CORES,N_CORES//6)), "", 1),
    (b"inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec,br_immed_spec,crypto_spec", ','.join(str(cores) for cores in range(0, N_CORES,N_CORES//8)), "", 1),
]
)
def test_wperf_stat(events,cores,metric,sleep):
    """ Test `wperf stat` command line output.

        Use pytest.mark.parametrize to set up below command line switches:

            ( -e <events>, -c <cores>, -m <metric>, sleep <value> )
    """
    cmd = 'wperf stat'.split()
    if events:
        cmd += ['-e', events]
    if cores:
        cmd += ['-c', cores]
    if metric:
        cmd += ['-m', metric]
    if sleep:
        cmd += ['sleep', str(sleep)]

    stdout, _ = run_command(cmd)

    # Common CLI outputs
    assert b'seconds time elapsed' in stdout

    # Core number message
    if cores:
        for core in cores.split(','):
            assert b'Performance counter stats for core %d' % int(core) in stdout

    # Pretty table basic columns (no multiplexing)
    for col in [b'counter value' , b'event name', b'event idx', b'event note']:
        assert re.search(b'[\\s]+%s[\\s]+' % col, stdout)

    # Pretty table basic columns (multiplexing)
    if b', multiplexed' in stdout:
        for col in [b'multiplexed' , b'scaled value']:
            assert re.search(b'[\\s]+%s[\\s]+' % col, stdout)

    # Event names in pretty table
    if events:
        for event in events.split(b','):
            assert re.search(b'[\\s]+%s[\\s]+' % event, stdout)
        assert re.search(b'[\\s]+cycle[\\s]+', stdout)
