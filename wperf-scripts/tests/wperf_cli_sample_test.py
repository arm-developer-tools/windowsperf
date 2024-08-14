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


"""Module is testing parts of `wperf sample` workflow."""
import os
import pytest
from common import run_command

def test_sample_cli_pdb_file():
    """ Test if --pdb_file <filename> checks if <filename> exist. """
    pdb_file = "pythasdasdasdason.db"
    _, stderr = run_command(f"wperf sample -e ld_spec:100000 --pdb_file {pdb_file} --pe_file foobarbaz.exe -c 1")

    if os.path.isfile(pdb_file):
        pytest.skip(f"File {pdb_file} already exists")

    assert b"unexpected arg" not in stderr
    assert b"PDB file 'pythasdasdasdason.db' doesn't exist" in stderr

def test_sample_cli_pe_file():
    """ Test if --pe_file <filename> checks if <filename> exist. """
    pe_file = "pythasdasdasdason.exe"
    _, stderr = run_command(f"wperf sample -e ld_spec:100000 --pe_file {pe_file} -c 1")

    if os.path.isfile(pe_file):
        pytest.skip(f"File {pe_file} already exists")

    assert b"unexpected arg" not in stderr
    assert b"PE file 'pythasdasdasdason.exe' doesn't exist" in stderr

@pytest.mark.parametrize("interval",
[
    ("a"),
    ("text"),
    ("_123"),
    ("x1"),
    ("0xp"),
    ("0xp123"),
]
)
def test_sample_cli_event_interval_text(interval):
    """ Test sampling event interval for parsing error. """
    cmd = f"wperf sample -e ld_spec:{interval} --pe_file file.exe -c 1"
    _, stderr = run_command(cmd)

    assert b"unexpected arg" not in stderr
    assert b"event interval: %s is invalid!" % str.encode(str(interval)) in stderr, cmd

@pytest.mark.parametrize("interval",
[
    (0x1FFFFFFFF),
    (8589934590),   # 0xffffffff * 2
]
)
def test_sample_cli_event_interval_overflow(interval):
    """ Test sampling event interval for parsing error.
        Max interval is: 0xFFFFFFFF.
    """
    cmd = f"wperf sample -e ld_spec:{interval} --pe_file file.exe -c 1"
    _, stderr = run_command(cmd)

    assert b"unexpected arg" not in stderr
    assert b"event interval: %s is out of range!" % str.encode(str(interval)) in stderr, cmd
