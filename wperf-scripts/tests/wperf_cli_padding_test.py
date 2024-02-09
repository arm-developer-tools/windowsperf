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

import json
import pytest
from common import run_command, is_json, check_if_file_exists
from common import get_result_from_test_results
from common import wperf_test_no_params
from fixtures import fixture_is_enough_GPCs

### Test cases


def test_wperf_padding_1_event():
    """ Test one event, no multiplexing """
    cmd = 'wperf test -e ase_spec --json'
    stdout, _ = run_command(cmd.split())
    json_output = json.loads(stdout)

    evt_core_index = get_result_from_test_results(json_output, "ioctl_events[EVT_CORE].index").split(',')
    evt_core_note = get_result_from_test_results(json_output, "ioctl_events[EVT_CORE].note").split(',')

    assert (len(evt_core_index) == 1)       #   116
    assert (len(evt_core_note) == 1)        #   (e),(e)
    assert (all(elem == '(e)' for elem in evt_core_note))    #   only '(e)' in note

    assert ('116' in evt_core_index)        #   ase_spec


def test_wperf_padding_8_events():
    """ Test two events, multiplexing """
    cmd = 'wperf test -e inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec,br_immed_spec,crypto_spec --json'
    stdout, _ = run_command(cmd.split())
    json_output = json.loads(stdout)

    evt_core_index = get_result_from_test_results(json_output, "ioctl_events[EVT_CORE].index").split(',')
    evt_core_note = get_result_from_test_results(json_output, "ioctl_events[EVT_CORE].note").split(',')

    assert (len(evt_core_index) == 8)       #   27,117,116,115,112,113,120,119
    assert (len(evt_core_note) == 8)        #   (e),(e),(e),(e),(e),(e),(e),(e)
    assert (all(elem == '(e)' for elem in evt_core_note))    #   only '(e)' in note

    assert ('27' in evt_core_index)
    assert ('117' in evt_core_index)
    assert ('116' in evt_core_index)
    assert ('115' in evt_core_index)
    assert ('112' in evt_core_index)
    assert ('113' in evt_core_index)
    assert ('120' in evt_core_index)
    assert ('119' in evt_core_index)


def test_wperf_padding_2_events():
    """ Test two events, no multiplexing """
    cmd = 'wperf test -e vfp_spec,ase_spec --json'
    stdout, _ = run_command(cmd.split())
    json_output = json.loads(stdout)

    evt_core_index = get_result_from_test_results(json_output, "ioctl_events[EVT_CORE].index").split(',')
    evt_core_note = get_result_from_test_results(json_output, "ioctl_events[EVT_CORE].note").split(',')

    assert (len(evt_core_index) == 2)       #   117,116
    assert (len(evt_core_note) == 2)        #   (e),(e)
    assert (all(elem == '(e)' for elem in evt_core_note))    #   only '(e)' in note

    assert ('117' in evt_core_index)        #   vfp_spec
    assert ('116' in evt_core_index)        #   ase_spec


def test_wperf_padding_3_events_2repeated():
    """ Test three events where two are the same, no multiplexing """
    cmd = 'wperf test -e vfp_spec,ase_spec,ase_spec --json'
    stdout, _ = run_command(cmd.split())
    json_output = json.loads(stdout)

    evt_core_index = get_result_from_test_results(json_output, "ioctl_events[EVT_CORE].index").split(',')
    evt_core_note = get_result_from_test_results(json_output, "ioctl_events[EVT_CORE].note").split(',')

    assert (len(evt_core_index) == 3)               #   117,116,116
    assert (len(evt_core_note) == 3)                #   (e),(e),(e)
    assert (all(elem == '(e)' for elem in evt_core_note))    #   only 'e' in note

    assert ('117' in set(evt_core_index))           #   vfp_spec
    assert ('116' in set(evt_core_index))           #   ase_spec


def test_wperf_padding_gpc_num_max():
    """ Fill -e with N events, where N is gpc_num so that we fill all general purpose counters, no multiplexing """
    list_of_events = ['inst_spec','vfp_spec','ase_spec','dp_spec','ld_spec','st_spec','br_immed_spec','crypto_spec']
    list_of_events_idx = [27,117,116,115,112,113,120,119]

    # get gpc_num
    json_output = wperf_test_no_params()      # get output from `wperf test`
    gpc_num = get_result_from_test_results(json_output, "PMU_CTL_QUERY_HW_CFG [gpc_num]")
    gpc_num = int(gpc_num, 16)  # it's a hex string e,g,. 0x0005

    # gpc_num may be different on different machines. E.g. 5 or 6.
    cmd = 'wperf test -e '
    cmd += ','.join(list_of_events[:gpc_num])
    cmd += ' --json'
    stdout, _ = run_command(cmd.split())
    json_output = json.loads(stdout)

    evt_core_index = get_result_from_test_results(json_output, "ioctl_events[EVT_CORE].index").split(',')
    evt_core_note = get_result_from_test_results(json_output, "ioctl_events[EVT_CORE].note").split(',')

    assert (len(evt_core_index) == gpc_num)       #   subset of: 27,117,116,115,112,113,120,119
    assert (len(evt_core_note) == gpc_num)        #   subset of: (e),(e),(e),(e),(e),(e),(e),(e)
    assert (all(elem == '(e)' for elem in evt_core_note))    #   only '(e)' in note

    for event_idx in list_of_events_idx[:gpc_num]:
        assert (str(event_idx) in evt_core_index)


@pytest.mark.xfail("not fixture_is_enough_GPCs(5)", reason="this test requires at least 5 GPCs")
def test_wperf_padding_groups_1():
    """ Test groups {crypto_spec,vfp_spec} """
    cmd = 'wperf test -e {crypto_spec,vfp_spec} --json'
    stdout, _ = run_command(cmd.split())
    json_output = json.loads(stdout)

    evt_core_index = get_result_from_test_results(json_output, "ioctl_events[EVT_CORE].index").split(',')
    evt_core_note = get_result_from_test_results(json_output, "ioctl_events[EVT_CORE].note").split(',')
    gpc_num = get_result_from_test_results(json_output, "PMU_CTL_QUERY_HW_CFG [gpc_num]")
    gpc_num = int(gpc_num, 16)  # it's a hex string e,g,. 0x0005

    assert (len(set(evt_core_index)) == 3)       #   119,117,27,27,27...
    assert (len(set(evt_core_note)) == 2)        #   (g0),(g0),(p),(p),(p),...

    assert ('27' in evt_core_index)              #   inst_spec       (padding)
    assert ('119' in evt_core_index)             #   crypto_spec
    assert ('117' in evt_core_index)             #   vfp_spec

    assert ('(g0)' in evt_core_note)             # Group 0
    assert ('(p)' in evt_core_note)              # padding


@pytest.mark.xfail("not fixture_is_enough_GPCs(5)", reason="this test requires at least 5 GPCs")
def test_wperf_padding_groups_2():
    """ Test groups crypto_spec,{vfp_spec,ase_spec} """
    cmd = 'wperf test -e crypto_spec,{vfp_spec,ase_spec} --json'
    stdout, _ = run_command(cmd.split())
    json_output = json.loads(stdout)

    evt_core_index = get_result_from_test_results(json_output, "ioctl_events[EVT_CORE].index").split(',')
    evt_core_note = get_result_from_test_results(json_output, "ioctl_events[EVT_CORE].note").split(',')
    gpc_num = get_result_from_test_results(json_output, "PMU_CTL_QUERY_HW_CFG [gpc_num]")
    gpc_num = int(gpc_num, 16)  # it's a hex string e,g,. 0x0005

    assert (len(set(evt_core_index)) == 4)      #   117,116,119,27,27,27,...
    assert (len(set(evt_core_note)) == 3)       #   (g0),(g0),(e),(p),(p),(p),....

    assert (evt_core_note.count('(g0)') == 2)   #   Two elements in group
    assert (evt_core_note.count('(e)') == 1)    #   Two extra events outside groups

    assert ('27' in evt_core_index)             #   inst_spec       (padding)
    
    assert ('119' in evt_core_index)            #   crypto_spec
    assert ('116' in evt_core_index)            #   ase_spec
    assert ('117' in evt_core_index)            #   vfp_spec

    assert ('(g0)' in evt_core_note)            # Group 0
    assert ('(p)' in evt_core_note)             # padding
    assert ('(e)' in evt_core_note)             # event

    assert (len(evt_core_index) == len(evt_core_note))  # list must have same length so w can test below

    # Below simple checks to make sure events belonging to 'e', 'p' and 'g0' are in the right place in index
    i = 0
    for note in evt_core_note:
        if note == '(e)':
            assert (evt_core_index[i] == '119')
        if note == '(p)':
            assert (evt_core_index[i] == '27')
        if note == '(g0)':
            assert ('117' in evt_core_index[i] or '116' in evt_core_index[i])
        i += 1  # progress that index ;)


@pytest.mark.xfail("not fixture_is_enough_GPCs(5)", reason="this test requires at least 5 GPCs")
def test_wperf_padding_groups_3():
    """ Test groups crypto_spec,{vfp_spec,ase_spec},dp_spec """
    cmd = 'wperf test -e crypto_spec,{vfp_spec,ase_spec},dp_spec --json'
    stdout, _ = run_command(cmd.split())
    json_output = json.loads(stdout)

    evt_core_index = get_result_from_test_results(json_output, "ioctl_events[EVT_CORE].index").split(',')
    evt_core_note = get_result_from_test_results(json_output, "ioctl_events[EVT_CORE].note").split(',')
    gpc_num = get_result_from_test_results(json_output, "PMU_CTL_QUERY_HW_CFG [gpc_num]")
    gpc_num = int(gpc_num, 16)  # it's a hex string e,g,. 0x0005

    assert (len(set(evt_core_index)) == 5)          #   117,116,119,115,27,27
    assert (len(set(evt_core_note)) == 3)           #   (g0),(g0),(e),(e),(p),(p)

    assert (evt_core_note.count('(g0)') == 2)       #   Two elements in group
    assert (evt_core_note.count('(e)') == 2)        #   Two extra events outside groups

    assert ('27' in evt_core_index)                 #   inst_spec       (padding)
    
    assert ('119' in evt_core_index)                #   crypto_spec
    assert ('116' in evt_core_index)                #   ase_spec
    assert ('115' in evt_core_index)                #   dp_spec
    assert ('117' in evt_core_index)                #   vfp_spec

    assert ('(g0)' in evt_core_note)                # Group 0
    assert ('(p)' in evt_core_note)                 # padding
    assert ('(e)' in evt_core_note)                 # event

    assert (len(evt_core_index) == len(evt_core_note))  # list must have same length so w can test below

    # Below simple checks to make sure events belonging to 'e', 'p' and 'g0' are in the right place in index
    i = 0
    for note in evt_core_note:
        if note == '(e)':
            assert ('119' in evt_core_index[i] or '115' in evt_core_index[i])
        if note == '(p)':
            assert (evt_core_index[i] == '27')
        if note == '(g0)':
            assert ('117' in evt_core_index[i] or '116' in evt_core_index[i])
        i += 1  # progress that index ;)


@pytest.mark.xfail("not fixture_is_enough_GPCs(5)", reason="this test requires at least 5 GPCs")
def test_wperf_padding_groups_4():
    """ Test groups crypto_spec,{vfp_spec,ase_spec,dp_spec} """
    cmd = 'wperf test -e crypto_spec,{vfp_spec,ase_spec,dp_spec} --json'
    stdout, _ = run_command(cmd.split())
    json_output = json.loads(stdout)

    evt_core_index = get_result_from_test_results(json_output, "ioctl_events[EVT_CORE].index").split(',')
    evt_core_note = get_result_from_test_results(json_output, "ioctl_events[EVT_CORE].note").split(',')
    gpc_num = get_result_from_test_results(json_output, "PMU_CTL_QUERY_HW_CFG [gpc_num]")
    gpc_num = int(gpc_num, 16)  # it's a hex string e,g,. 0x0005

    assert (len(set(evt_core_index)) == 5)          #   117,116,115,119,27,27
    assert (len(set(evt_core_note)) == 3)           #   (g0),(g0),(g0),(e),(p),(p)

    assert (evt_core_note.count('(g0)') == 3)       #   Two elements in group
    assert (evt_core_note.count('(e)') == 1)        #   Two extra events outside groups

    assert ('27' in evt_core_index)                 #   inst_spec       (padding)
    
    assert ('119' in evt_core_index)                #   crypto_spec
    assert ('116' in evt_core_index)                #   ase_spec
    assert ('115' in evt_core_index)                #   dp_spec
    assert ('117' in evt_core_index)                #   vfp_spec

    assert ('(g0)' in evt_core_note)                # Group 0
    assert ('(p)' in evt_core_note)                 # padding
    assert ('(e)' in evt_core_note)                 # event

    assert (len(evt_core_index) == len(evt_core_note))  # list must have same length so w can test below

    # Below simple checks to make sure events belonging to 'e', 'p' and 'g0' are in the right place in index
    i = 0
    for note in evt_core_note:
        if note == '(e)':
            assert (evt_core_index[i] == '119')
        if note == '(p)':
            assert (evt_core_index[i] == '27')
        if note == '(g0)':
            assert (evt_core_index[i] == '117' or evt_core_index[i] == '116' or evt_core_index[i] == '115')
        i += 1  # progress that index ;)


@pytest.mark.xfail("not fixture_is_enough_GPCs(5)", reason="this test requires at least 5 GPCs")
def test_wperf_padding_m_imix():
    """ Test one metric (imix), this test is not checking if indexes are OK """
    cmd = 'wperf test -m imix --json'
    stdout, _ = run_command(cmd.split())
    json_output = json.loads(stdout)

    evt_core_index = get_result_from_test_results(json_output, "ioctl_events[EVT_CORE].index").split(',')
    evt_core_note = get_result_from_test_results(json_output, "ioctl_events[EVT_CORE].note").split('),(')

    # get gpc_num
    json_output = wperf_test_no_params()      # get output from `wperf test`
    gpc_num = get_result_from_test_results(json_output, "PMU_CTL_QUERY_HW_CFG [gpc_num]")
    gpc_num = int(gpc_num, 16)  # it's a hex string e,g,. 0x0005

    assert (len(evt_core_index) == gpc_num)       #   27,115,117,116,112,113
    assert (len(evt_core_note) == gpc_num)        #   (g0,imix),(g0,imix),(g0,imix),(g0,imix),(g0,imix),(g0,imix)
    assert (all('g0,imix' in elem for elem in evt_core_note))    #   only '(g0,imix)' in note


@pytest.mark.xfail("not fixture_is_enough_GPCs(5)", reason="this test requires at least 5 GPCs")
def test_wperf_padding_m_imix_icache():
    """ Test two metrics (imix + icache), this test is not checking if indexes are OK """
    cmd = 'wperf test -m imix,icache --json'
    stdout, _ = run_command(cmd.split())
    json_output = json.loads(stdout)

    evt_core_index = get_result_from_test_results(json_output, "ioctl_events[EVT_CORE].index").split(',')
    evt_core_note = get_result_from_test_results(json_output, "ioctl_events[EVT_CORE].note").split('),(')

    # get gpc_num
    json_output = wperf_test_no_params()      # get output from `wperf test`
    gpc_num = get_result_from_test_results(json_output, "PMU_CTL_QUERY_HW_CFG [gpc_num]")
    gpc_num = int(gpc_num, 16)  # it's a hex string e,g,. 0x0005

    assert ((len(evt_core_index) % gpc_num) == 0)       #   e.g. 27,115,117,116,112,113,20,1,39,40,8,27
    assert ((len(evt_core_note) % gpc_num) == 0)        #   e.g. (g0,imix),(g0,imix),(g0,imix),(g0,imix),(g0,imix),(g0,imix),(g1,icache),(g1,icache),(g1,icache),(g1,icache),(g1,icache),(p)

    imix_cnt = 0
    icache_cnt = 0

    for elem in evt_core_note:
        if 'g0,imix' in elem:
            imix_cnt += 1
        if 'g1,icache' in elem:
            icache_cnt += 1

    assert imix_cnt > 0
    assert icache_cnt > 0


TOPDOWN_L1_EVENTS = "{l1i_tlb_refill,l1i_tlb},{inst_spec,ld_spec},{st_spec,inst_spec},{inst_retired,br_mis_pred_retired},{l1d_tlb_refill,inst_retired},{itlb_walk,l1i_tlb},{inst_spec,dp_spec},{l2d_tlb_refill,inst_retired},{stall_backend,cpu_cycles},{crypto_spec,inst_spec},{l1i_tlb_refill,inst_retired},{ll_cache_rd,ll_cache_miss_rd},{inst_retired,cpu_cycles},{l2d_cache_refill,l2d_cache},{br_indirect_spec,inst_spec,br_immed_spec},{l2d_tlb_refill,l2d_tlb},{inst_retired,itlb_walk},{dtlb_walk,inst_retired},{l1i_cache,l1i_cache_refill},{l1d_cache_refill,l1d_cache},{br_mis_pred_retired,br_retired},{inst_retired,l2d_cache_refill},{vfp_spec,inst_spec},{stall_frontend,cpu_cycles},{inst_spec,ase_spec},{ll_cache_rd,ll_cache_miss_rd},{inst_retired,l1d_cache_refill},{l1d_tlb,dtlb_walk},{inst_retired,ll_cache_miss_rd},{l1i_cache_refill,inst_retired},{l1d_tlb_refill,l1d_tlb}"
LARGE_SET_1 = "{CPU_CYCLES},{cpu_cycles,stall_backend},{br_mis_pred_retired,inst_spec,dtlb_walk},{cpu_cycles,stall_frontend},{l1d_tlb,l1d_tlb_refill},{ld_spec,st_spec,ldst_spec},{unaligned_ld_spec,unaligned_st_spec,unaligned_ldst_spec}"

@pytest.mark.parametrize("events,expected_evt_core_index,expected_evt_core_note",
[
    # Generated for HW with 6 GPCs:
    (TOPDOWN_L1_EVENTS,
        "2,38,27,112,113,27,8,34,5,8,53,38,27,115,45,8,36,17,119,27,2,8,54,55,8,17,23,22,27,27,122,27,120,45,47,27,8,53,52,8,20,1,3,4,34,33,8,23,117,27,35,17,27,116,54,55,8,3,37,52,8,55,1,8,5,37",
        "(g0),(g0),(g1),(g1),(g2),(g2),(g3),(g3),(g4),(g4),(g5),(g5),(g6),(g6),(g7),(g7),(g8),(g8),(g9),(g9),(g10),(g10),(g11),(g11),(g12),(g12),(g13),(g13),(p),(p),(g14),(g14),(g14),(g15),(g15),(p),(g16),(g16),(g17),(g17),(g18),(g18),(g19),(g19),(g20),(g20),(g21),(g21),(g22),(g22),(g23),(g23),(g24),(g24),(g25),(g25),(g26),(g26),(g27),(g27),(g28),(g28),(g29),(g29),(g30),(g30)"),
    # Generated for HW with 5 GPCs:
    (TOPDOWN_L1_EVENTS,
        "2,38,27,112,27,113,27,8,34,27,5,8,53,38,27,27,115,45,8,27,36,17,119,27,27,2,8,54,55,27,8,17,23,22,27,122,27,120,45,47,8,53,52,8,27,20,1,3,4,27,34,33,8,23,27,117,27,35,17,27,27,116,54,55,27,8,3,37,52,27,8,55,1,8,27,5,37,27,27,27",
        "(g0),(g0),(g1),(g1),(p),(g2),(g2),(g3),(g3),(p),(g4),(g4),(g5),(g5),(p),(g6),(g6),(g7),(g7),(p),(g8),(g8),(g9),(g9),(p),(g10),(g10),(g11),(g11),(p),(g12),(g12),(g13),(g13),(p),(g14),(g14),(g14),(g15),(g15),(g16),(g16),(g17),(g17),(p),(g18),(g18),(g19),(g19),(p),(g20),(g20),(g21),(g21),(p),(g22),(g22),(g23),(g23),(p),(g24),(g24),(g25),(g25),(p),(g26),(g26),(g27),(g27),(p),(g28),(g28),(g29),(g29),(p),(g30),(g30),(p),(p),(p)"),
    (LARGE_SET_1,
        "17,36,17,27,27,34,27,52,17,35,37,5,112,113,114,104,105,106,27,27",
        "(g0),(g0),(e),(p),(p),(g1),(g1),(g1),(g2),(g2),(g3),(g3),(g4),(g4),(g4),(g5),(g5),(g5),(p),(p)"),
]
)
@pytest.mark.xfail("not fixture_is_enough_GPCs(5)", reason="this test requires at least 5 GPCs")
def test_wperf_padding_large_topdown_l1(events,expected_evt_core_index,expected_evt_core_note):
    """ Test large set of event groups (example for top_down l1-level) """
    cmd = f"wperf test -e {events} --json"
    stdout, _ = run_command(cmd.split())
    json_output = json.loads(stdout)

    evt_core_index = get_result_from_test_results(json_output, "ioctl_events[EVT_CORE].index")
    evt_core_note = get_result_from_test_results(json_output, "ioctl_events[EVT_CORE].note")

    # get gpc_num
    json_output = wperf_test_no_params()      # get output from `wperf test`
    gpc_num = get_result_from_test_results(json_output, "PMU_CTL_QUERY_HW_CFG [gpc_num]")
    gpc_num = int(gpc_num, 16)  # it's a hex string e,g,. 0x0005

    assert ((len(evt_core_index.split(",")) % gpc_num) == 0)
    assert ((len(evt_core_note.split("),(")) % gpc_num) == 0)

    evt_core_index_no_padding = [int (n) for n in evt_core_index.split(",") if int(n) != 27]    # event 27 is padding
    evt_core_note_no_padding  = [str (s) for s in evt_core_note.split(",") if str(s) != "(p)"]    # "(p)" is for padding

    expected_evt_core_index_no_padding = [int (n) for n in expected_evt_core_index.split(",") if int(n) != 27]    # event 27 is padding
    expected_evt_core_note_no_padding  = [str (s) for s in expected_evt_core_note.split(",") if str(s) != "(p)"]    # "(p)" is for padding

    # We will compare both without padding (27 or (p) events)
    assert expected_evt_core_index_no_padding == evt_core_index_no_padding
    assert expected_evt_core_note_no_padding == evt_core_note_no_padding
