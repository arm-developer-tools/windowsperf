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

"""
This script runs simple wperf CLI tests.

These tests are designed to stress test (hammer) Kernel Driver and user-space CLI application.

"""

import json
import os
import re
from common import run_command, is_json, check_if_file_exists
from common import wperf_metric_is_available

import pytest

N_CORES = os.cpu_count()
CORES_ODD = ",".join(str(c) for c in range(0, N_CORES) if c & 1)
CORES_EVEN = ",".join(str(c) for c in range(0, N_CORES) if ~c & 1)
CORES_ALL = str()

### Test cases

TOPDOWN_L1_EVENTS = "{l1i_tlb_refill,l1i_tlb},{inst_spec,ld_spec},{st_spec,inst_spec},{inst_retired,br_mis_pred_retired},{l1d_tlb_refill,inst_retired},{itlb_walk,l1i_tlb},{inst_spec,dp_spec},{l2d_tlb_refill,inst_retired},{stall_backend,cpu_cycles},{crypto_spec,inst_spec},{l1i_tlb_refill,inst_retired},{ll_cache_rd,ll_cache_miss_rd},{inst_retired,cpu_cycles},{l2d_cache_refill,l2d_cache},{br_indirect_spec,inst_spec,br_immed_spec},{l2d_tlb_refill,l2d_tlb},{inst_retired,itlb_walk},{dtlb_walk,inst_retired},{l1i_cache,l1i_cache_refill},{l1d_cache_refill,l1d_cache},{br_mis_pred_retired,br_retired},{inst_retired,l2d_cache_refill},{vfp_spec,inst_spec},{stall_frontend,cpu_cycles},{inst_spec,ase_spec},{ll_cache_rd,ll_cache_miss_rd},{inst_retired,l1d_cache_refill},{l1d_tlb,dtlb_walk},{inst_retired,ll_cache_miss_rd},{l1i_cache_refill,inst_retired},{l1d_tlb_refill,l1d_tlb}"

@pytest.mark.parametrize("sleep,cores,metric,events",
[
    # Slowly progress towards insanity ;)
    (5, CORES_ODD, "",                   "CPU_CYCLES,ld_spec,st_spec,vfp_spec"),
    (5, CORES_ODD, "imix",               "CPU_CYCLES,{cpu_cycles,stall_backend}"),
    (5, CORES_ODD, "imix,icache",        "CPU_CYCLES,{cpu_cycles,stall_backend},br_mis_pred_retired,inst_spec,dtlb_walk,{cpu_cycles,stall_frontend}"),
    (5, CORES_ODD, "imix,icache",        "CPU_CYCLES,{cpu_cycles,stall_backend},br_mis_pred_retired,inst_spec,dtlb_walk,{cpu_cycles,stall_frontend},{l1d_tlb,l1d_tlb_refill}"),
    (5, CORES_ODD, "imix,icache,dcache", "CPU_CYCLES,{cpu_cycles,stall_backend},br_mis_pred_retired,inst_spec,dtlb_walk,{cpu_cycles,stall_frontend},{l1d_tlb,l1d_tlb_refill},{ld_spec,st_spec,ldst_spec}"),
    (5, CORES_ODD, "imix,icache,dcache", "CPU_CYCLES,{cpu_cycles,stall_backend},br_mis_pred_retired,inst_spec,dtlb_walk,{cpu_cycles,stall_frontend},{l1d_tlb,l1d_tlb_refill},{ld_spec,st_spec,ldst_spec},{unaligned_ld_spec,unaligned_st_spec,unaligned_ldst_spec}"),
    (5, CORES_ALL, "imix,icache,dcache", "CPU_CYCLES,{cpu_cycles,stall_backend},br_mis_pred_retired,inst_spec,dtlb_walk,{cpu_cycles,stall_frontend},{l1d_tlb,l1d_tlb_refill},{ld_spec,st_spec,ldst_spec},{unaligned_ld_spec,unaligned_st_spec,unaligned_ldst_spec}"),

    (5, CORES_EVEN, "",                   "CPU_CYCLES,ld_spec,st_spec,vfp_spec"),
    (5, CORES_EVEN, "imix",               "CPU_CYCLES,{cpu_cycles,stall_backend}"),
    (5, CORES_EVEN, "imix,icache",        "CPU_CYCLES,{cpu_cycles,stall_backend},br_mis_pred_retired,inst_spec,dtlb_walk,{cpu_cycles,stall_frontend}"),
    (5, CORES_EVEN, "imix,icache",        "CPU_CYCLES,{cpu_cycles,stall_backend},br_mis_pred_retired,inst_spec,dtlb_walk,{cpu_cycles,stall_frontend},{l1d_tlb,l1d_tlb_refill}"),
    (5, CORES_EVEN, "imix,icache,dcache", "CPU_CYCLES,{cpu_cycles,stall_backend},br_mis_pred_retired,inst_spec,dtlb_walk,{cpu_cycles,stall_frontend},{l1d_tlb,l1d_tlb_refill},{ld_spec,st_spec,ldst_spec}"),
    (5, CORES_EVEN, "imix,icache,dcache", "CPU_CYCLES,{cpu_cycles,stall_backend},br_mis_pred_retired,inst_spec,dtlb_walk,{cpu_cycles,stall_frontend},{l1d_tlb,l1d_tlb_refill},{ld_spec,st_spec,ldst_spec},{unaligned_ld_spec,unaligned_st_spec,unaligned_ldst_spec}"),
    (5, CORES_ALL,  "imix,icache,dcache", "CPU_CYCLES,{cpu_cycles,stall_backend},br_mis_pred_retired,inst_spec,dtlb_walk,{cpu_cycles,stall_frontend},{l1d_tlb,l1d_tlb_refill},{ld_spec,st_spec,ldst_spec},{unaligned_ld_spec,unaligned_st_spec,unaligned_ldst_spec}"),

    # Level 1 Top down example events
    (5, CORES_ODD,  "", TOPDOWN_L1_EVENTS),
    (5, CORES_EVEN, "", TOPDOWN_L1_EVENTS),
    (5, CORES_ALL,  "", TOPDOWN_L1_EVENTS),
]
)
def test_wperf_hammer_core(sleep,cores,metric,events):
    """ Stress test CORE PMU events, with mix of metrics, groups and separate events.
    """
    cmd = 'wperf stat'.split()
    if events:
        cmd += ['-e', events]
    if cores:
        cmd += ['-c', cores]
    if metric and wperf_metric_is_available(metric):
        cmd += ['-m', metric]
    if sleep:
        cmd += ['sleep', str(sleep)]

    cmd += ['--json']

    stdout, _ = run_command(cmd)
    assert is_json(stdout), "in %s" % (' '.join(cmd))
