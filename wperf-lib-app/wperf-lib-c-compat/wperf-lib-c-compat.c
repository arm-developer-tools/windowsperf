#include "wperf-lib.h"

// This app should contain all the available functions in wperf-lib,
// so we can make sure linking this C file with wperf-lib.lib is OK.

int main()
{
    wperf_init();
    wperf_close();
    wperf_set_verbose(true);
    wperf_driver_version(NULL);
    wperf_version(NULL);
    wperf_list_events(NULL, NULL);
    wperf_list_num_events(NULL, NULL);
    wperf_list_metrics(NULL, NULL);
    wperf_list_num_metrics(NULL, NULL);
    wperf_list_num_metrics_events(NULL, NULL);
    wperf_stat(NULL, NULL);
    wperf_sample(NULL, NULL);
    wperf_sample_annotate(NULL, NULL);
    wperf_sample_stats(NULL, NULL);
    wperf_num_cores(NULL);
    wperf_test(NULL, NULL);
    return 0;
}
