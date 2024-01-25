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


### Common Telemetry-Solutions / ustress code

import csv
import os

## Module variables, import `common_ustress` to access these

# Where we keep Telemetry Solution code
TS_USTRESS_DIR = r"telemetry-solution\tools\ustress"

# Header with supported by `ustress` CPUs:
TS_USTRESS_HEADER = os.path.join(TS_USTRESS_DIR, "cpuinfo.h")


### Module functions

def get_metric_values(cvs_file_path, metric):
    """ Return list of values from timeline CVS file (for one core only). """
    result = []
    metric_column = "M@" + metric   # This is how we encode metric column in CVS timeline file

    with open(cvs_file_path, newline='') as csvfile:
        spamreader = csv.reader(csvfile, delimiter=',', quotechar='|')

        """ Example 'row'(s):
            ['core', '1,core', '1,core', '1,core', '1,']
            ['cycle,l1d_cache,l1d_cache_refill,M@l1d_cache_miss_ratio,']
            ['163757124,46340967,2007747,0.043,']
            ['77863232,31287908,780320,0.025,']
            ['34097420,8487530,356686,0.042,']
            ['41752921,9459244,411182,0.043,']
            ['54506416,17393294,576923,0.033,']
        """
        metric_col_index = -1       # Nothing found yet ( < 0 )
        for row in spamreader:
            if metric_col_index >= 0 and len(row) > metric_col_index:
                val = float(row[metric_col_index])
                result.append(val)

            # Find row with event names and metric(s) names
            # Below you will find rows with event count and metric values
            if metric_column in row:
                metric_col_index = row.index(metric_column)

    return result
