#!/usr/bin/env python3
"""this script prompts a user to merge the pmu events stored in \
   https://github.com/ARM-software/data/blob/master/pmu/ to a file by cpu """

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

import sys
from urllib import request
import urllib.error
import socket
import re
import os
import tempfile
import argparse
import json
import wget




URL = "https://github.com/ARM-software/data/blob/master/pmu/"
URL_RAW = "https://raw.githubusercontent.com/ARM-software/data/master/pmu/"

class ArchEventsUpdate:
    """update the pmu events into a new file"""
    wperf_arch_events = "WPERF_ARMV8_ARCH_EVENTS"
    tempdir = ""

    def __init__(self, argv):
        self.tempdir = tempfile.gettempdir() + "/"
        if argv.list_cpu is True:
            self.list_cpu()

        if argv.cpu is not None:
            self.update(argv)

    def list_cpu(self):
        """get all cpus from URL"""
        target_file = self.tempdir + "temp"
        self.get_webpage(URL, target_file)
        with open(target_file,"r",encoding='utf-8') as fhandle:
            content = fhandle.read()
            cpus = re.findall('(?<=title=").*(?=.json" )',content)
            for item in cpus:
                print(item)

    def parse_jsonfile(self, json_file):
        """parse pmu events in jsonfile to local"""
        line_output = ""
        with open(json_file,"r",encoding='utf-8') as fjson:
            data = json.load(fjson)
            architecture = '"' + data['architecture'] + '"'

            for event in data['events']:
                line_output = line_output + (self.wperf_arch_events + "(" \
                    + (architecture + ',').ljust(16,' ')  \
                    + (event['name'] + ',').ljust(50,' ')  \
                    + '0x' + (hex(int(event['code']))[2:]).rjust(4,'0') \
                    + ', \"' + (event['name'].lower() + '",').ljust(48, ' ')
                    + '"' + event['description'].lower()
                    + '\")\n')
        return line_output


    def update(self,argv):
        """update the pmu events info from URL_RAM by core"""
        url_temp = URL_RAW + argv.cpu + '.json'
        target_file = self.get_webfile(url_temp)
        lines = self.parse_jsonfile(target_file)
        os.remove(target_file)

        if argv.output is not None:
            with open(argv.output, 'w',encoding='utf-8') as foutput:
                if argv.license is not None:
                    self.add_license(argv.license,foutput)
                foutput.write(lines)
        else:
            for i in lines:
                print(i,end="")


    def add_license(self, license_file, ftarget):
        """add license to the file header"""
        with open(license_file,"r",encoding='utf-8') as file:
            for line in file.readlines():
                line = "// " + line
                ftarget.write(line)
            ftarget.write("\n")


    def get_webpage(self, url_temp,target_file):
        """get info from a webpage"""
        try:
            request.urlretrieve(url_temp, target_file)
        except (socket.gaierror, urllib.error.URLError):
            print("Some error happen! May be the network instability, \
                  Please check it and try again!")
            sys.exit()

    def get_webfile(self,url_temp):
        """get a webfile from a url"""
        try:
            file_name = wget.download(url_temp,bar=None)
        except (socket.gaierror, urllib.error.URLError):
            print("Some error happen! May be the network instability, \
                  Please check it and try again!")
            sys.exit()
        return file_name


def main(argv):
    """the entry for arch events update"""
    ArchEventsUpdate(argv)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="update the cpu's pmu events file!")
    parser.add_argument("-l","--list_cpu", action = "store_true", help="list all cpus in " + URL)
    parser.add_argument("-c","--cpu", type=str,help="cpu type that to update")
    parser.add_argument("-o","--output", type=str,help="pmu events file for wperf")
    parser.add_argument("--license", type=str,help="license file added to the script header")
    args = parser.parse_args()
    if args.list_cpu is False and args.cpu is None:
        parser.print_help()
        sys.exit()
    main(args)
