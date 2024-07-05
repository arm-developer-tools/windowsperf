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


"""
This script prompts a user to merge the PMU events stored in either:

    https://gitlab.arm.com/telemetry-solution/telemetry-solution/-/tree/main/data/pmu/cpu

or in a local directory
"""

import sys
import os
import argparse
import json
import shunting_yard as sy
import requests
from requests.compat import urljoin

URL = "https://gitlab.arm.com/telemetry-solution/telemetry-solution/-/raw/main/data/pmu/cpu/"
FILE_PATH = r"tests\telemetry-solution\data\pmu\cpu" # local PATH to telemetry solution data folder (e.g. containing 'neoverse' folder)
# For example
# https://gitlab.arm.com/telemetry-solution/telemetry-solution/-/raw/main/data/pmu/cpu/neoverse/neoverse-n1.json

PMU_CPU_MAPPING = {
    "neoverse" : ["neoverse-n1.json",
                  "neoverse-n2-r0p0.json",
                  "neoverse-n2-r0p1.json",
                  "neoverse-n2-r0p3.json",
                  "neoverse-n2.json",
                  "neoverse-v1.json",
                  "neoverse-v2.json"]
}

input_mode = "url"          # default input is presumed to be URL, unless specified otherwise
output_file = sys.stdout    # default output is presumed stdout, unless specified otherwise

def ts_quote(s):
     return '"' + s.replace('\n', '\\n') + '"'

def ts_align(s, size):
    return s + ' '*(size - len(s))

def ts_print_as_comment(s):
    """ Print C style of comment string"""
    print ('// ' + s, file=output_file)

def ts_print_define(name, values):
    """ Print C style define like string:
        NAME(values)
    """
    print (name.upper() + '(' + values + ')', file=output_file)

def ts_download_json(url, name):
    """ Download JSON file locally, return None if file is not JSON """
    DEFINE = "WPERF_TS_ALIAS"

    response = requests.get(url, timeout=10)
    content = response.content
    try:
        j = json.loads(content)
    except json.decoder.JSONDecodeError:
        alias = content.decode("utf-8")
        alias = alias.split('.')[0]
        values = [ts_quote(name),
                  ts_quote(alias)]

        values = ','.join(values)
        ts_print_define(DEFINE, values)
        print (file=output_file)
        return None
    return j

def ts_load_json(filepath, name):
    """ Load JSON file locally, return None if file is not JSON """
    DEFINE = "WPERF_TS_ALIAS"

    try:
        with open(filepath, 'r', encoding='utf-8') as f:
            j = json.load(f)
    except json.decoder.JSONDecodeError:
        with open(filepath, 'r', encoding='utf-8') as f:
            alias = f.read().split('.')[0]
        values = [ts_quote(name),
                  ts_quote(alias)]

        values = ','.join(values)
        ts_print_define(DEFINE, values)
        print (file=output_file)
        return None

    return j

def ts_parse_metrics(j, name):
    """ Parse 'metrics'  section of CPU description. """
    DEFINE = "WPERF_TS_METRICS"
    metrics = j["metrics"]

    metric_name_max_len = 0
    for metric in metrics:
        if len(metric) > metric_name_max_len:
            metric_name_max_len = len(metric)
    metric_name_max_len += 4

    formula_name_max_len = 0
    for metric in metrics:
        if len(metrics[metric]["formula"]) > formula_name_max_len:
            formula_name_max_len = len(metrics[metric]["formula"])
    formula_name_max_len += 4

    units_name_max_len = 0
    for metric in metrics:
        if len(metrics[metric]["units"]) > units_name_max_len:
            units_name_max_len = len(metrics[metric]["units"])
    units_name_max_len += 4

    events_name_max_len = 0
    for metric in metrics:
        if len(','.join(metrics[metric]["events"]).lower()) > events_name_max_len:
            events_name_max_len = len(','.join(metrics[metric]["events"]).lower())
    events_name_max_len += 4

    for metric in metrics:
        formula = metrics[metric]["formula"]
        formula_sy = sy.shunting_yard(formula)

        values = [ts_quote(name),
                  ts_align(ts_quote(metric), metric_name_max_len),
                  ts_align(ts_quote(','.join(metrics[metric]["events"]).lower()), events_name_max_len),
                  ts_align(ts_quote(formula).lower(), formula_name_max_len),
                  ts_align(ts_quote(formula_sy).lower(), formula_name_max_len),
                  ts_align(ts_quote(metrics[metric]["units"]), units_name_max_len),
                  ts_quote(metrics[metric]["title"])]
        values = ','.join(values)
        ts_print_define(DEFINE, values)

def ts_parse_groups_metrics(j, name):
    """ Parse 'group metrics'  section of CPU description. """
    DEFINE = "WPERF_TS_GROUPS_METRICS"
    metrics = j["groups"]["metrics"]

    metric_name_max_len = 0
    for metric in metrics:
        if len(metric) > metric_name_max_len:
            metric_name_max_len = len(metric)
    metric_name_max_len += 4

    events_name_max_len = 0
    for metric in metrics:
        if len(','.join(metrics[metric]["metrics"]).lower()) > events_name_max_len:
            events_name_max_len = len(','.join(metrics[metric]["metrics"]).lower())
    events_name_max_len += 4

    for metric in metrics:
        values = [ts_quote(name),
                  ts_align(ts_quote(metric), metric_name_max_len),
                  ts_align(ts_quote(','.join(metrics[metric]["metrics"]).lower()), events_name_max_len),
                  ts_quote(metrics[metric]["title"]),
                  ts_quote(metrics[metric]["description"])]
        values = ','.join(values)
        ts_print_define(DEFINE, values)


def ts_parse_events(j, name):
    """ Parse 'events'  section of CPU description. """
    DEFINE = "WPERF_TS_EVENTS"
    events = j["events"]

    event_name_max_len = 0
    for event in events:
        if len(event) > event_name_max_len:
            event_name_max_len = len(event)

    event_name_max_len += 4

    for event in events:
        values = [ts_quote(name),
                  ts_align(event.upper(), event_name_max_len),
                  events[event]["code"],
                  ts_align(ts_quote(event.lower()), event_name_max_len),
                  ts_quote(events[event]["title"]),
                  ts_quote(events[event]["description"])]

        values = ','.join(values)
        ts_print_define(DEFINE, values)

def ts_parse_product_configuration(j, name):
    """ Parse 'product_configuration' section of CPU description. """
    DEFINE = "WPERF_TS_PRODUCT_CONFIGURATION"
    product_configuration = j["product_configuration"]

    def ts_print_field(s):
        """ Print field in correct format depending on content. """
        txt = str(s)
        if txt[0].isdigit():
            return txt
        return '"' + txt + '"'

    keys = sorted(product_configuration.keys())

    # We will add name of the json as the unique name of the product
    values = ','.join([ts_print_field(product_configuration[key]) for key in keys])
    values = '"' + name + '"' + ',' + values

    ts_print_as_comment ('name,' + ','.join(keys))
    ts_print_define(DEFINE, values)

def ts_parse_cpu_json(j, name):
    """ Parse JSON data describing CPU and its other attributes. """

    if j is not None:
        ts_print_as_comment ('Product configuration for: ' + name)
        ts_parse_product_configuration(j, name)
        print(file=output_file)

    if j is not None:
        ts_print_as_comment ('Events for: ' + name)
        ts_parse_events(j, name)
        print(file=output_file)

    if j is not None:
        ts_print_as_comment ('Metrics for: ' + name)
        ts_parse_metrics(j, name)
        print(file=output_file)

    if j is not None:
        ts_print_as_comment ('Metric Groups for: ' + name)
        ts_parse_groups_metrics(j, name)
        print(file=output_file)

def main(argv):
    """the entry for arch events update"""

    if input_mode == "file":
        for family in PMU_CPU_MAPPING: # e.g neoverse
            file_family = os.path.join(FILE_PATH, family) # join the FILE_PATH with the family
            for cpu_json in PMU_CPU_MAPPING[family]:
                name = cpu_json.split('.')[0]   # name of the cpu via JSON filename e.g neoverse-n2
                filepath = os.path.join(file_family, cpu_json)  # assuming the json files are in a directory named after the family
                ts_print_as_comment (filepath)
                j = ts_load_json(filepath, name)
                ts_parse_cpu_json(j, name)
    elif input_mode == "url":
        for family in PMU_CPU_MAPPING:
            url_family = urljoin(URL, family + '/')
            for cpu_json in PMU_CPU_MAPPING[family]:
                name = cpu_json.split('.')[0]   # name of the cpu via JSON filename
                url = urljoin(url_family, cpu_json)
                ts_print_as_comment (url)
                j = ts_download_json(url, name)
                ts_parse_cpu_json(j, name)
    else:
        raise Exception("unrecognised parameters with arg '-i', expected 'url' or'file'")


if __name__ == "__main__":
    """ Process events from online sources from Telemetry Solution team, or from local JSON files. """

    parser = argparse.ArgumentParser(description="update the cpu's pmu events file!")
    parser.add_argument("-l","--list_cpu", action = "store_true", help="list all cpus in local directory")
    parser.add_argument("-c","--cpu", type=str,help="cpu type that to update")
    parser.add_argument("-o","--output", type=str,help="pmu events file for wperf")
    parser.add_argument("--license", type=str,help="license file added to the script header")
    parser.add_argument("--url", action = "store_true",help="selects URL as input source (on by default!)")
    parser.add_argument("--file", action = "store_true",help="selects file as input source")
    args = parser.parse_args()

    if args.output:
        output_file = open(args.output, 'w', encoding='utf-8')    # open output file if one is provided, else stdout

    if args.url and args.file:
        raise Exception("'--url' and '--file' boolean flags must be used mutually exclusively")
    input_mode = "file" if args.file else "url"

    try:
        main(args)
    finally:
        if output_file is not sys.stdout:
            output_file.close()
