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

import sys
from urllib import request
import urllib.error
import socket
import re
import os
import wget
import tempfile
import argparse
import json
import shutil

url = "https://github.com/ARM-software/data/blob/master/pmu/"
url_raw = "https://raw.githubusercontent.com/ARM-software/data/master/pmu/"

class arch_events_update:
    wperf_arch_events = "WPERF_ARMV8_ARCH_EVENTS"
    tempdir = ""

    def __init__(self, argv):
        self.tempdir = tempfile.gettempdir() + "/"
        if argv.list_cpu == True:
            self.list_cpu()

        if argv.cpu != None:
            self.update(argv)

    def list_cpu(self):
        target_file = self.tempdir + "temp"
        self.get_webpage(url, target_file)
        f = open(target_file,"r",encoding='utf-8')
        content = f.read()
        cpus = re.findall('(?<=title=").*(?=.json" )',content)
        for item in cpus:
            print(item)
        f.close()



    def update(self,argv):
        url_temp = url_raw + argv.cpu + '.json'
        target_file = self.get_webfile(url_temp)

        if argv.output != None:
            foutput = open(argv.output, 'w')
            if argv.license != None:
                self.add_license(argv.license,foutput)

        with open(target_file,"r",encoding='utf-8') as f:
            data = json.load(f)
            for event in data['events']:
                line_output = (self.wperf_arch_events + "(" + (event['name'] + ',').ljust(50,' ')  \
                    + '0x' + (hex(int(event['code']))[2:]).rjust(4,'0') \
                    + ', \"' + event['name'].lower() + '\")\n')

                if argv.output != None:
                    foutput.write(line_output)
                    continue
                print(line_output,end="")

        os.remove(target_file)
        if argv.output != None:
            foutput.close()


    def add_license(self, license_file, f):
        with open(license_file,"r") as file:
            for line in file.readlines():
                line = "// " + line
                f.write(line)
            f.write("\n")


    def get_webpage(self, url_temp,target_file):
        try:
            request.urlretrieve(url_temp, target_file)
        except (socket.gaierror, urllib.error.URLError):
            print("Some error happen! May be the network instability, Please check it and try again!")
            sys.exit()

    def get_webfile(self,url_temp):
        try:
            file_name = wget.download(url_temp,bar=None)
        except:
            print("Some error happen! May be the network instability, Please check it and try again!")
            sys.exit()
        return file_name


def main(argv):
    update_class = arch_events_update(argv)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="update the cpu's pmu events file!")
    parser.add_argument("-l","--list_cpu", action = "store_true", help="list all cpus in " + url)
    parser.add_argument("-c","--cpu", type=str,help="cpu type that to update")
    parser.add_argument("-o","--output", type=str,help="pmu events file for wperf")
    parser.add_argument("--license", type=str,help="license file added to the script header")
    argv = parser.parse_args()
    if argv.list_cpu == False and argv.cpu == None:
        parser.print_help()
        sys.exit()
    main(argv)


