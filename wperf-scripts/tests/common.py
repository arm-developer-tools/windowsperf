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
import subprocess
import os
import re

### Common test runner code

# See https://developer.arm.com/documentation/ddi0601/2020-12/AArch64-Registers/MIDR-EL1--Main-ID-Register
arm64_vendor_names = {
    0x41: "Arm Limited",
    0x42: "Broadcomm Corporation",
    0x43: "Cavium Inc",
    0x44: "Digital Equipment Corporation",
    0x46: "Fujitsu Ltd",
    0x49: "Infineon Technologies AG",
    0x4D: "Motorola or Freescale Semiconductor Inc",
    0x4E: "NVIDIA Corporation",
    0x50: "Applied Micro Circuits Corporation",
    0x51: "Qualcomm Inc",
    0x56: "Marvell International Ltd",
    0x69: "Intel Corporation",
    0xC0: "Ampere Computing"
}

def is_json(str_to_test):
    """ Test if string is in JSON format. """
    try:
        json.loads(str_to_test)
    except ValueError:
        return False
    return True

def get_schema(schema_name, test_path):
    """ Get JSON Object for schema with name `schema_name` """
    with open(f"{test_path}/schemas/wperf.{schema_name}.schema") as file:
        json_schema = json.loads(file.read())
    return json_schema

def run_command(args, cwd = None):
    """ Run command and capture stdout and stderr for parsing. """
    process = subprocess.Popen(args,
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE,
                         cwd=cwd)
    stdout, stderr = process.communicate()
    return stdout, stderr

def check_if_file_exists(filename):
    """ Return True if FILENAME exists. """
    return os.path.isfile(filename)

def wperf_list():
    """ Test `wperf list` JSON output """
    cmd = 'wperf list --json'
    stdout, _ = run_command(cmd.split())
    json_output = json.loads(stdout)
    return json_output


def wperf_metric_is_available(metric):
    """ Check if given `metric` is available """
    json_output = wperf_list()
    for metrics in json_output["Predefined_Metrics"]:
        if metric == metrics["Metric"]:
            return True
    return False


def wperf_metric_events(metric):
    """ Return string with list of events for given `metric` """
    json_output = wperf_list()
    for metrics in json_output["Predefined_Metrics"]:
        if metric == metrics["Metric"]:
            return metrics["Events"]    #   "{inst_spec,dp_spec,vfp_spec,ase_spec,ld_spec,st_spec}"
    return None

def wperf_metric_events_list(metric):
    """ Return string with list of events for given `metric` """
    json_output = wperf_list()
    for metrics in json_output["Predefined_Metrics"]:
        if metric == metrics["Metric"]:
            return metrics["Events"].strip("{}").split(",") # Make result a list
    return None

def get_result_from_test_results(j, Test_Name):
    """ Return 'Result' from Test_Results[Test_Name] """
    for test_result in j["Test_Results"]:
        if Test_Name in test_result["Test_Name"]:
            return test_result["Result"]
    return None

def wperf_test_no_params():
    """ We use this fixture to get values from `wperf test` which do not change with execution like gpc_num """
    cmd = 'wperf test --json'
    stdout, _ = run_command(cmd.split())
    json_output = json.loads(stdout)
    return json_output

def wperf_test_get_key_val(k, v):
    """ Get e.g. PMU_CTL_QUERY_HW_CFG [midr_value] where:
        key: PMU_CTL_QUERY_HW_CFG
        value: midr_value
    """
    json_output = wperf_test_no_params()
    return get_result_from_test_results(json_output, f"{k} [{v}]")

def get_product_name():
    """ Get `product_name` form `wperf test` command. """
    json_wperf_test = wperf_test_no_params()
    return get_result_from_test_results(json_wperf_test, "pmu_device.product_name")    # e.g. "armv8-a" or "neoverse-n1"

def get_make_CPU_name(product_name):
    """ Get CPU name used in Makefile for ustress. """
    product_name = product_name.replace('-', '_').upper()
    return product_name

def get_CPUs_supported_by_ustress(ts_ustress_header):
    """ Get list of CPUs supported by ustress. """
    cpus = []
    with open(ts_ustress_header) as couinfo_h:
        cpuinfo_header = couinfo_h.read()
        cpus = re.findall(r'defined\(CPU_([A-Z0-9_]+)\)', cpuinfo_header)    # E.g. ['NEOVERSE_V1', 'NEOVERSE_N1', 'NEOVERSE_N2', 'NONE']
    return cpus
