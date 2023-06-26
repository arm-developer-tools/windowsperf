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
from common import run_command, is_json, check_if_file_exists
from common import get_result_from_test_results
from common import wperf_test_no_params

### Test cases


def test_wperf_padding_1_event():
    """ Test one event, no multiplexing """
    cmd = 'wperf test -e ase_spec -json'
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
    cmd = 'wperf test -e inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec,br_immed_spec,crypto_spec -json'
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
    cmd = 'wperf test -e vfp_spec,ase_spec -json'
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
    cmd = 'wperf test -e vfp_spec,ase_spec,ase_spec -json'
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
    cmd += ' -json'
    stdout, _ = run_command(cmd.split())
    json_output = json.loads(stdout)

    evt_core_index = get_result_from_test_results(json_output, "ioctl_events[EVT_CORE].index").split(',')
    evt_core_note = get_result_from_test_results(json_output, "ioctl_events[EVT_CORE].note").split(',')

    assert (len(evt_core_index) == gpc_num)       #   subset of: 27,117,116,115,112,113,120,119
    assert (len(evt_core_note) == gpc_num)        #   subset of: (e),(e),(e),(e),(e),(e),(e),(e)
    assert (all(elem == '(e)' for elem in evt_core_note))    #   only '(e)' in note

    for event_idx in list_of_events_idx[:gpc_num]:
        assert (str(event_idx) in evt_core_index)


def test_wperf_padding_groups_1():
    """ Test groups {crypto_spec,vfp_spec} """
    cmd = 'wperf test -e {crypto_spec,vfp_spec} -json'
    stdout, _ = run_command(cmd.split())
    json_output = json.loads(stdout)

    evt_core_index = get_result_from_test_results(json_output, "ioctl_events[EVT_CORE].index").split(',')
    evt_core_note = get_result_from_test_results(json_output, "ioctl_events[EVT_CORE].note").split(',')
    gpc_num = get_result_from_test_results(json_output, "PMU_CTL_QUERY_HW_CFG [gpc_num]")
    gpc_num = int(gpc_num, 16)  # it's a hex string e,g,. 0x0005

    assert (gpc_num >= 5)                        #   This test assumes number of General Purpose Counter is at least 5

    assert (len(set(evt_core_index)) == 3)       #   119,117,27,27,27...
    assert (len(set(evt_core_note)) == 2)        #   (g0),(g0),(p),(p),(p),...

    assert ('27' in evt_core_index)              #   inst_spec       (padding)
    assert ('119' in evt_core_index)             #   crypto_spec
    assert ('117' in evt_core_index)             #   vfp_spec

    assert ('(g0)' in evt_core_note)             # Group 0
    assert ('(p)' in evt_core_note)              # padding


def test_wperf_padding_groups_2():
    """ Test groups crypto_spec,{vfp_spec,ase_spec} """
    cmd = 'wperf test -e crypto_spec,{vfp_spec,ase_spec} -json'
    stdout, _ = run_command(cmd.split())
    json_output = json.loads(stdout)

    evt_core_index = get_result_from_test_results(json_output, "ioctl_events[EVT_CORE].index").split(',')
    evt_core_note = get_result_from_test_results(json_output, "ioctl_events[EVT_CORE].note").split(',')
    gpc_num = get_result_from_test_results(json_output, "PMU_CTL_QUERY_HW_CFG [gpc_num]")
    gpc_num = int(gpc_num, 16)  # it's a hex string e,g,. 0x0005

    assert (gpc_num >= 5)                       #   This test assumes number of General Purpose Counter is at least 5

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


def test_wperf_padding_groups_3():
    """ Test groups crypto_spec,{vfp_spec,ase_spec},dp_spec """
    cmd = 'wperf test -e crypto_spec,{vfp_spec,ase_spec},dp_spec -json'
    stdout, _ = run_command(cmd.split())
    json_output = json.loads(stdout)

    evt_core_index = get_result_from_test_results(json_output, "ioctl_events[EVT_CORE].index").split(',')
    evt_core_note = get_result_from_test_results(json_output, "ioctl_events[EVT_CORE].note").split(',')
    gpc_num = get_result_from_test_results(json_output, "PMU_CTL_QUERY_HW_CFG [gpc_num]")
    gpc_num = int(gpc_num, 16)  # it's a hex string e,g,. 0x0005

    assert (gpc_num >= 5)                           #   This test assumes number of General Purpose Counter is at least 5

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


def test_wperf_padding_groups_4():
    """ Test groups crypto_spec,{vfp_spec,ase_spec,dp_spec} """
    cmd = 'wperf test -e crypto_spec,{vfp_spec,ase_spec,dp_spec} -json'
    stdout, _ = run_command(cmd.split())
    json_output = json.loads(stdout)

    evt_core_index = get_result_from_test_results(json_output, "ioctl_events[EVT_CORE].index").split(',')
    evt_core_note = get_result_from_test_results(json_output, "ioctl_events[EVT_CORE].note").split(',')
    gpc_num = get_result_from_test_results(json_output, "PMU_CTL_QUERY_HW_CFG [gpc_num]")
    gpc_num = int(gpc_num, 16)  # it's a hex string e,g,. 0x0005

    assert (gpc_num >= 5)                           #   This test assumes number of General Purpose Counter is at least 5

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

def test_wperf_padding_m_imix():
    """ Test one metric (imix), this test is not checking if indexes are OK """
    cmd = 'wperf test -m imix -json'
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

def test_wperf_padding_m_imix_icache():
    """ Test two metrics (imix + icache), this test is not checking if indexes are OK """
    cmd = 'wperf test -m imix,icache -json'
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
