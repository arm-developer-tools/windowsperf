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

r"""Module is testing `wperf sample` and `wperf record` with native CPython,
   built from sourced debug executables.

# Description
This is `cpython` test bench build script. It contains all functions needed to:
* Check for basic dependencies and build dependencies.
* Build `cpython` for WOA target with project's `build.bat` build script.

# Build dependencies
* MSVC cross/native arm64 build environment, see `vcvarsall.bat`.

See example configuration needed to build those tests:
> %comspec% /k "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" arm64
**********************************************************************
** Visual Studio 2022 Developer Command Prompt v17.7.1
** Copyright (c) 2022 Microsoft Corporation
**********************************************************************
[vcvarsall.bat] Environment initialized for: 'arm64'

"""
import os
import pytest
from common import run_command
from common_cpython import CPYTHON_DIR, CPYTHON_EXE_DIR


### Test cases

def test_cpython_bench_build__deps():
    """ Check if we have enough dependencies to run the CPython build."""
    assert os.path.isdir(CPYTHON_DIR), f"submodule with CPython source-code '{CPYTHON_DIR}' doesn't exist"

def test_cpython_bench_build__make():
    """ Build CPython using its own build script. """

    ### Build CPython for this platform, with debug on (-d) and for ARM64 platform (-p ARM64)
    build_bat_path = os.path.join(CPYTHON_DIR, "build.bat")
    cmd = f"{build_bat_path} -d -p ARM64"
    stdout, _ = run_command(cmd)   # Build all tests

    # Build sanity checks:
    """
    Build succeeded.
    0 Warning(s)
    0 Error(s)

    Time Elapsed 00:00:58.42
    """

    # Sanity checks for the build success
    assert b"Build succeeded." in stdout, "CPython build script failed"
    assert b"0 Error(s)" in stdout, "CPython build script failed with error(s)"
    assert os.path.isdir(CPYTHON_EXE_DIR), "CPython ARM64 build directory {CPYTHON_EXE_DIR} doesn't exist"

    # Check for expected build output files
    build_artifacts = ["python_d.exe", "python_d.pdb"]
    for f in build_artifacts:
        p = os.path.join(CPYTHON_EXE_DIR, f)
        assert os.path.isfile(p)
