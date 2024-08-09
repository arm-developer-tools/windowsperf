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
import pytest
from common import run_command, check_if_file_exists
from common_cpython import CPYTHON_EXE_DIR

def test_cpython_bench_record_time():
    """ Test trivial stdout printouts for `wperf sample` command. """
    python_d_exe_path = os.path.join(CPYTHON_EXE_DIR, "python_d.exe")

    if not check_if_file_exists(python_d_exe_path):
        pytest.skip(f"Can't locate CPython native executable in {python_d_exe_path}")

    cmd = f"wperf sample -e ld_spec:100000 --pe_file {python_d_exe_path} -c 1 --timeout 3"
    stdout, _ = run_command(cmd)

    #
    #    Example output for sampling:
    #
    #    >wperf sample -c 0 -e ld_spec:100000 --pe_file cpython\PCbuild\arm64\python_d.exe
    #    failed to query base address of 'cpython\PCbuild\arm64\python_d.exe' with 6
    #    sampling .....Ctrl-C received, quit counting... done!
    #    ======================== sample source: ld_spec, top 50 hot functions ========================
    #            overhead  count  symbol
    #            ========  =====  ======
    #              100.00      2  unknown
    #              100.00%     2  top 1 in total
    #
    #                   2.247 seconds time elapsed

    #
    # Sampling standard output smoke tests
    #
    assert b'sampling ...' in stdout
    assert b'sample source: ld_spec' in stdout
    assert b'seconds time elapsed' in stdout
