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

    process = subprocess.Popen(["wperf", "stat",  "-m",  "imix",  "-c",  "1",  "--timeout",  "3"], text=True)
    time.sleep(.2)
    process2 = subprocess.run(["wperf", "--version"], text=True, capture_output=True)
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
    process3 = subprocess.run(["wperf", "--version"], capture_output=True, text=True)
    s = process3.stderr

    assert 'warning: other WindowsPerf process acquired the wperf-driver.' not in s, "test_wperf_locking_unlocked failed, process #3"


def test_wperf_locking_locked_after_n_times():
    """ Test locking after we run "process" in the background and bounce off #n times with "process2"
        We expect to lock (without force) after "process" finishes.
    """
    process = subprocess.Popen(["wperf", "stat",  "-m",  "imix",  "-c",  "1",  "--timeout",  "6"], text=True)

    #
    # We will try to access the driver (but no force lock, we should fail)
    #
    for n in range(5):
        time.sleep(.5)
        process2 = subprocess.run(["wperf", "--version"], text=True, capture_output=True)
        s =  process2.stderr
        assert 'warning: other WindowsPerf process acquired the wperf-driver.' in s, f"test_wperf_locking_locked_after_n_times failed after {n} tries, process #2"

    #
    # Wait for the big process to finish
    #
    while True:
        retcode = process.poll()
        if retcode is not None: # Process finished.
            break
        time.sleep(.2)
        continue
    process.kill()

    # Test that we are unlocked after test is finished
    process3 = subprocess.run(["wperf", "--version"], capture_output=True, text=True)
    s = process3.stderr
    assert 'warning: other WindowsPerf process acquired the wperf-driver.' not in s, "test_wperf_locking_unlocked failed, process #3"


def test_wperf_locking_locked_after_n_times_and_force():
    """ Test locking after we run "process" in the background and bounce off #n times with "process2"
        We expect to force lock after we "knock-the-door" 5 times.
    """
    process = subprocess.Popen(["wperf", "stat",  "-m",  "imix",  "-c",  "1",  "--timeout",  "6"],
                                stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE, text=True)

    #
    # We will try to access the driver (but no force lock, we should fail)
    #
    for n in range(5):
        time.sleep(.5)
        process2 = subprocess.run(["wperf", "--version"], text=True, capture_output=True)
        s =  process2.stderr
        assert 'warning: other WindowsPerf process acquired the wperf-driver.' in s, f"test_wperf_locking_locked_after_n_times failed after {n} tries, process #2"

    time.sleep(.5)
    # Test that we are unlocked after test is finished
    process3 = subprocess.run(["wperf", "--version", "--force-lock"], capture_output=True, text=True)
    s = process3.stderr
    assert 'warning: other WindowsPerf process acquired the wperf-driver.' not in s, "test_wperf_locking_unlocked failed, process #3"

    #
    # Wait for the big process to finish, it was kicked out
    #
    while True:
        retcode = process.poll()
        if retcode is not None: # Process finished.
            break
        time.sleep(.2)
        continue
    process.kill()
    _, errs = process.communicate()
    assert errs is not None
    assert 'warning: other WindowsPerf process hijacked' in errs, "process failed"


def test_wperf_locking_force():
    """ Test locking:
    1) Run "process" with 3 sec counting.
    2) Force lock over "process" with "process2" using --force-lock flag
    """
    process = subprocess.Popen(["wperf", "stat",  "-m",  "imix",  "-c",  "1",  "-t",  "--timeout",  "3"],
                               stdout=subprocess.PIPE,
                               stderr=subprocess.PIPE, text=True)
    time.sleep(.2)
    process2 = subprocess.run(["wperf", "--version", "--force-lock"], text=True, capture_output=True)

    while True:
        retcode = process.poll()
        if retcode is not None: # Process finished.
            break
        time.sleep(.2)
        continue

    process.kill()
    _, errs = process.communicate()
    assert errs is not None
    assert 'warning: other WindowsPerf process hijacked' in errs, "test_wperf_locking_force failed"


def test_wperf_locking_force_n_times():
    """ Test locking: we will run "process" and force lock `n` times, one by one.
        First force-lock will kick out "process" and rest will just correctly execute.
        We test here if unnecessary force-lock is breaking feature.
    """
    process = subprocess.Popen(["wperf", "stat",  "-m",  "imix",  "-c",  "1",  "--timeout",  "10"],
                               stdout=subprocess.PIPE,
                               stderr=subprocess.PIPE, text=True)

    #
    # We will try to access the driver (with force lock) `n` times and always be successful.
    #
    for n in range(5):
        time.sleep(.5)
        process2 = subprocess.run(["wperf", "--version", "--force-lock"], text=True, capture_output=True)
        s = process2.stderr
        assert 'warning: other WindowsPerf process acquired the wperf-driver.' not in s, "test_wperf_locking_unlocked failed, process #3"

    time.sleep(.5)
    process2 = subprocess.run(["wperf", "--version"], text=True, capture_output=True)
    s = process2.stderr
    assert 'warning: other WindowsPerf process acquired the wperf-driver.' not in s, "test_wperf_locking_unlocked failed, process #3"

    #
    # Wait for the big process to finish, it was kicked out
    #
    while True:
        retcode = process.poll()
        if retcode is not None: # Process finished.
            break
        time.sleep(.2)
        continue
    process.kill()
    _, errs = process.communicate()
    assert errs is not None
    assert 'warning: other WindowsPerf process hijacked' in errs, "process failed"
