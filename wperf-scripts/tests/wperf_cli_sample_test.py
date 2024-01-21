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


import os
from common import run_command

import pytest

def test_sample_cli_pdb_file():
    """ Test if --pdb_file <filename> checks if <filename> exist. """
    pdb_file = "pythasdasdasdason.db"
    _, stderr = run_command(f"wperf sample -e ld_spec:100000 --pdb_file {pdb_file} --pe_file foobarbaz.exe -c 1")

    if os.path.isfile(pdb_file):
        pytest.skip(f"File {pdb_file} already exists")

    assert b"PDB file 'pythasdasdasdason.db' doesn't exist" in stderr

def test_sample_cli_pe_file():
    """ Test if --pe_file <filename> checks if <filename> exist. """
    pe_file = "pythasdasdasdason.exe"
    _, stderr = run_command(f"wperf sample -e ld_spec:100000 --pe_file {pe_file} -c 1")

    if os.path.isfile(pe_file):
        pytest.skip(f"File {pdb_file} already exists")

    assert b"PE file 'pythasdasdasdason.exe' doesn't exist" in stderr
