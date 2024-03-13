// BSD 3-Clause License
//
// Copyright (c) 2024, Arm Limited
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <stdio.h>
#include "wperf-lib.h"

// This app should contain all the available functions in wperf-lib,
// so we can make sure linking this C file with wperf-lib.lib is OK.

#define RUN_AND_CHECK(func) if (func == false) { printf("Execution of "#func "\t ... FAIL \n"); wperf_close(); return -1; } else { printf("Test "#func "\t ... PASS \n"); }

int main()
{
    LIST_CONF list_conf;
    TEST_CONF test_conf;
    VERSION_INFO driver_ver;
    int num_cores, num_events, num_metrics, num_metric_events;

    RUN_AND_CHECK(wperf_init());
    RUN_AND_CHECK(wperf_set_verbose(true));
    RUN_AND_CHECK(wperf_driver_version(&driver_ver));
    RUN_AND_CHECK(wperf_version(&driver_ver));
    RUN_AND_CHECK(wperf_list_events(&list_conf, NULL));
    RUN_AND_CHECK(wperf_list_num_events(&list_conf, &num_events));
    RUN_AND_CHECK(wperf_list_metrics(&list_conf, NULL));
    RUN_AND_CHECK(wperf_list_num_metrics(&list_conf, &num_metrics));
    RUN_AND_CHECK(wperf_list_num_metrics_events(&list_conf, &num_metric_events));

    {
        // Below functions will return false as they require some extra instrumentation
        // This functions are tested in wperf-lib-app so we will not test them here.
        wperf_stat(NULL, NULL);
        wperf_sample(NULL, NULL);
        wperf_sample_annotate(NULL, NULL);
        wperf_sample_stats(NULL, NULL);
    }

    RUN_AND_CHECK(wperf_num_cores(&num_cores));
    RUN_AND_CHECK(wperf_test(&test_conf, NULL));
    RUN_AND_CHECK(wperf_close());
    return 0;
}
