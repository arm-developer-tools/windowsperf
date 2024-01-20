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

import json
import multiprocessing
import platform
import sys

from common import is_json, run_command
from datetime import datetime
from os import linesep

def pytest_terminal_summary(terminalreporter, exitstatus, config):
    """ Create sections for terminal summary. 
        See https://docs.pytest.org/en/6.2.x/reference.html#pytest.hookspec.pytest_terminal_summary
    """
    terminalreporter.section("WindowsPerf Test Configuration")

    # OS information
    p = platform.platform()
    m = platform.machine()
    v = sys.version
    proc = platform.processor()
    cnt = multiprocessing.cpu_count()
    terminalreporter.write(f"OS: {p}, {m} {linesep}")
    terminalreporter.write(f"CPU: {cnt} x {proc} {linesep}")
    terminalreporter.write(f"Python: {v} {linesep}")

    # Test time
    now = datetime.now()
    date_time = now.strftime("%d/%m/%Y, %H:%M:%S")
    terminalreporter.write(f"Time: {date_time} {linesep}")

    # Get WindowsPerf versions
    cmd = 'wperf --version --json'
    stdout, _ = run_command(cmd.split())
    assert is_json(stdout)
    json_output = json.loads(stdout)

    for w in json_output["Version"]:
        component = w["Component"]
        version = w["Version"]
        gitver = w["GitVer"]
        # Add section entry
        terminalreporter.write(f"{component}: {version}.{gitver} {linesep}")
