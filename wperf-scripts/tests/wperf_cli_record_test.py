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

"""Module is testing `wperf record` workflow."""
import pytest
from common import run_command

@pytest.mark.parametrize("cores",
[
    ("0,1"),
    ("1,0"),
    ("1,2"),
    ("1,2,3"),
    ("1,1,2"),
    ("0,1,1,2,3,5"),
    ("7,6,5,4,3,2,1"),
]
)
def test_record_many_cores_selected(cores):
    """ It is not allowed to specify more than one core for sampling. """
    _, stderr = run_command(f"wperf record -c {cores} -- TEST")

    assert b"you can specify only one core for sampling" in stderr

@pytest.mark.parametrize("cores",
[
    ("0,1"),
    ("1,0"),
    ("1,2"),
    ("1,2,3"),
    ("1,1,2"),
    ("0,1,1,2,3,5"),
    ("7,6,5,4,3,2,1"),
]
)
def test_record_many_cores_selected_ext(cores):
    """ It is not allowed to specify more than one core for sampling. """
    _, stderr = run_command(f"wperf record -v -e ld_spec:100000 -c {cores} --timeout 30 -- python_d.exe -c 10**10**100")

    assert b"you can specify only one core for sampling" in stderr
