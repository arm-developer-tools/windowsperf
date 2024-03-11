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


""" Tests for `wperf` that involve use of `xperf`.
    - `xperf` is included in the Windows Assessment and Deployment Kit
      (Windows ADK) that can be downloaded at https://go.microsoft.com/fwlink/?linkid=2243390.
    - You can refer to https://learn.microsoft.com/en-us/windows-hardware/test/wpt/
      article for basic documentation and usage.

      Note: tests will use `xperf` and `wperf-devgen.exe` to allocate HW GPCs
            (general purpose PMU counters) and install / uninstall WindowsPerf driver.
"""

import pytest
from common import run_command
from conftest import PYTEST_CLI_OPTION_ENABLE_BENCH_XPERF

# Pytest fixture to get the value of user_input_01
@pytest.fixture
def enable_bench_xperf_p(request):
    """ Returns TRUE if user specified `--enable-bench-xperf` to enable this test bench.
    """
    return request.config.getoption(PYTEST_CLI_OPTION_ENABLE_BENCH_XPERF)

#
### Test cases
#

def test_wperf_xperf_bench_test(enable_bench_xperf_p):
    """ Test all dependencies for this test bench before we continue.
    """

    if not enable_bench_xperf_p:
        pytest.skip(f"skipping XPERF test bench, enable it with `{PYTEST_CLI_OPTION_ENABLE_BENCH_XPERF}` command line options")

    #
    # Check for `xperf` first
    #
    req_exec = "xperf.exe"
    cmd = f"{req_exec}"
    _, stderr = run_command(cmd.split())
    """
    >xperf

        Microsoft (R) Windows (R) Performance Analyzer Version 10.0.22621
        Performance Analyzer Command Line
        Copyright (c) 2022 Microsoft Corporation. All rights reserved.

        Usage: xperf options ...

            xperf -help start            for logger start options
            xperf -help providers        for known tracing flags
            xperf -help stackwalk        for stack walking options
            xperf -help stop             for logger stop options
            xperf -help merge            for merge multiple trace files
            xperf -help processing       for trace processing options
            xperf -help symbols          for symbol decoding configuration
            xperf -help query            for query options
            xperf -help mark             for mark and mark-flush
            xperf -help format           for time and timespan formats on the command line
            xperf -help profiles         for profile options
    """

    assert b"Performance Analyzer Version" in stderr, f"XPERF test bench: can't execute {req_exec}"
    assert b"Performance Analyzer Command Line" in stderr, f"XPERF test bench: can't execute {req_exec}"

    #
    # Check for `wperf-devgen.exe` tool
    #
    """
    >wperf-devgen.exe --version
    WindowsPerf-DevGen ver. 3.4.3 (04a2824b-dirty/Debug) WindowsPerf's Kernel Driver Installer.
    Report bugs to: https://gitlab.com/Linaro/WindowsPerf/windowsperf/-/issues
    """
    req_exec = "wperf-devgen.exe"
    cmd = f"{req_exec}  --version"
    stdout, _ = run_command(cmd.split())
    assert b"WindowsPerf-DevGen" in stdout, f"XPERF test bench: can't execute {req_exec}"
    assert b"WindowsPerf's Kernel Driver Installer" in stdout, f"XPERF test bench: can't execute {req_exec}"
