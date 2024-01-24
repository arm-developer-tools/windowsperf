#usr/bin/env python3

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

"""
This script runs simple wperf CLI tests.

Requires:
    pytest -    install with `pip install -U pytest`

Usage:
    >pytest wperf_cli_lock_test.py

"""


import time
import subprocess

### Test cases

def test_wperf_locking_locked():
    """ Test locking:
    1) Run simple counting as "process" in the background
    2) Wait and run "process2" while "process" is running, expect lock.
    3) Wait for "process" and "process2" finish and run "process3", expect no lock
    """

    process = subprocess.Popen(["wperf.exe", "stat",  "-m",  "imix",  "-c",  "1",  "--timeout",  "3"], text=True)
    time.sleep(.2)
    process2 = subprocess.run(["wperf.exe", "--help"], text=True, capture_output=True)
    s =  process2.stderr

    while True:
        retcode = process.poll()
        if retcode is not None: # Process finished.
            break
        time.sleep(.2)
        continue

    process.kill()

    assert 'warning: other WindowsPerf process acquired the wperf-driver.' in s, "test_wperf_locking_locked failed, process #2"

    # Test that we are unlocked after test is finished
    process3 = subprocess.run(["wperf.exe", "--help"], capture_output=True, text=True)
    s = process3.stderr

    assert 'warning: other WindowsPerf process acquired the wperf-driver.' not in s, "test_wperf_locking_unlocked failed, process #3"


def test_wperf_locking_force():
    """ Test locking:
    1) Run "process" with 3 sec counting.
    2) Force lock over "process" with "process2" using --force-lock flag
    """
    process = subprocess.Popen(["wperf.exe", "stat",  "-m",  "imix",  "-c",  "1",  "-t",  "--timeout",  "3"],
                               stdout=subprocess.PIPE,
                               stderr=subprocess.PIPE, text=True)
    time.sleep(.2)
    process2 = subprocess.run(["wperf.exe", "--help", "--force-lock"], text=True, capture_output=True)

    while True:
        retcode = process.poll()
        if retcode is not None: # Process finished.
            break
        time.sleep(.2)
        continue

    process.kill()
    _, errs = process.communicate()

    assert 'warning: other WindowsPerf process hijacked' in errs, "test_wperf_locking_force failed"
