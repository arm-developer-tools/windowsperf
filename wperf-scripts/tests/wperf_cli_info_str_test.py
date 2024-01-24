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

import re
from common import run_command

### Test cases


def test_wperf_cli_no_commands(record_property):
    """ Test no command line options output. """
    cmd = 'wperf'
    stdout, _ = run_command(cmd.split())

    """
    Output should be something like this:

    WindowsPerf ver. 2.5.1 (4153c04e/Release) WOA profiling with performance counters.
    Report bugs to: https://gitlab.com/Linaro/WindowsPerf/windowsperf/-/issues
    Use --help for help.
    """

    ver_str = re.findall(rb'WindowsPerf ver\. \d+\.\d+\.\d+ \(([0-9a-fA-F]+(-[a-zA-Z]+)?\/[ReleaseDebug]+)\)', stdout)
    record_property("wperf_version_str", ver_str)

    assert len(ver_str) == 1    # ['WindowsPerf ver. 2.5.1 (4153c04e/Release)']
    assert b'WOA profiling with performance counters' in stdout
    assert b'Report bugs to: https://gitlab.com/Linaro/WindowsPerf/windowsperf/-/issues' in stdout
    assert b'Use --help for help.' in stdout
