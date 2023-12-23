# wperf

[[_TOC_]]

# Build wperf CLI

You can build `wperf` project from command line:

```
> devenv windowsperf.sln /Rebuild "Debug|ARM64" /Project wperf\wperf.vcxproj
```

# Usage of wperf

```
> wperf --help
WindowsPerf ver. 2.5.1 (20661762/Debug) WOA profiling with performance counters.
Report bugs to: https://gitlab.com/Linaro/WindowsPerf/windowsperf/-/issues

usage: wperf [options]

    Options:
    list                   List supported events and metrics.
    stat                   Count events.If - e is not specified, then count default events.
    test                   Configuration information about driver and application configuration.
    sample                 Sample events. If -e is not specified, cycle counter will be the default sample source
    record                 Same as sample but also automatically spawns the process and pins it to the core specified by '-c'.
                           You can define the process to spawn via '--pe_file' or use the end of the command line to write the command.
                           All command line arguments afterwards are passed verbatim to the command.
    -e e1, e2...           Specify events to count.Event eN could be a symbolic name or in raw number.
                           Symbolic name should be what's listed by 'perf list', raw number should be rXXXX,
                           XXXX is hex value of the number without '0x' prefix.
                           when doing sampling, support -e e1:sample_freq1,e2:sample_freq2...
    -m m1, m2...           Specify metrics to count. 'imix', 'icache', 'dcache', 'itlb', 'dtlb' supported.
    --timeout SEC          Specify counting duration(in s). The accuracy is 0.1s.
    sleep N                Like --timeout, for compatibility with Linux perf.
    -i N                   Specify counting interval(in s). To be used with -t.
    -t                     Enable timeline mode. It specifies -i 60 --timeout 1 implicitly.
                           Means counting 1 second after every 60 second, and the result
                           is in.csv file in the same folder where wperf is invoked.
                           You can use -i and --timeout to change counting duration and interval.
    -n N                   How many times count in timeline mode (disabled by default).
    --image_name           Specify the image name you want to sample.
    --pe_file              Specify the PE file.
    --pdb_file             Specify the PDB file.
    --sample-display-long  Display decorated symbol names.
    --sample-display-row   Set how many samples you want to see in the summary (50 by default).
    --record_spawn_delay   Set the waiting time, in milliseconds, before reading process data after spawning it with 'record'.
                           Default value is 1000ms.
    -C config_file         Provide customized config file which describes metrics etc.
    -E config_file         Provide customized config file which describes custom events.
    -E event_list          Provide custom events from command line, e.g. '-E name1:0x1234,name2:0xABCD'
    -c core_idx            Profile on the specified core. Skip -c to count on all cores.
                           In sampling user must specify exactly one core with -c.
    -c cpu_list            Profile on the specified cores, 'cpu_list' is comma separated list e.g. '-c 0,1,2,3'.
    --dmc dmc_idx          Profile on the specified DDR controller. Skip -dmc to count on all DMCs.
    -k                     Count kernel mode as well (disabled by default).
    -h / --help            Show tool help.
    --output               Enable JSON output to file.
    --config               Specify configuration parameters, format NAME=VALUE.
    -q                     Quiet mode, no output is produced.
    --json                 Define output type as JSON.
    -l                     Alias of 'list'.
    --verbose              Enable verbose output.
    -v                     Alias of '-verbose'.
    --version              Show tool version.
```

# WindowsPerf auxiliary command line options

## List available PMU events with `list`
```
> wperf list
List of pre-defined events (to be used in -e):

Alias Name                    Raw Index   Event Type
sw_incr                       0x0         hardware core event
l1i_cache_refill              0x1         hardware core event
l1i_tlb_refill                0x2         hardware core event
l1d_cache_refill              0x3         hardware core event
l1d_cache                     0x4         hardware core event
l1d_tlb_refill                0x5         hardware core event
ld_retired                    0x6         hardware core event
st_retired                    0x7         hardware core event
inst_retired                  0x8         hardware core event
exc_taken                     0x9         hardware core event
exc_return                    0xa         hardware core event
cid_write_retired             0xb         hardware core event
pc_write_retired              0xc         hardware core event
br_immed_retired              0xd         hardware core event
...
```

## Obtain information about `WindowsPerf` configuration with `test`

```
> wperf test
        Test Name                                           Result
        =========                                           ======
        request.ioctl_events [EVT_CORE]                     False
        request.ioctl_events [EVT_DSU]                      False
        request.ioctl_events [EVT_DMC_CLK/EVT_DMC_CLKDIV2]  False
        pmu_device.vendor_name                              Arm Limited
        pmu_device.events_query(events) [EVT_CORE]          79
        pmu_device.events_query(events) [EVT_DSU]           9
        pmu_device.events_query(events) [EVT_DMC_CLK]       3
        pmu_device.events_query(events) [EVT_DMC_CLKDIV2]   26
        PMU_CTL_QUERY_HW_CFG [arch_id]                      0x000f
        PMU_CTL_QUERY_HW_CFG [core_num]                     0x0050
        PMU_CTL_QUERY_HW_CFG [fpc_num]                      0x0001
        PMU_CTL_QUERY_HW_CFG [gpc_num]                      0x0006
        PMU_CTL_QUERY_HW_CFG [part_id]                      0x0d0c
        PMU_CTL_QUERY_HW_CFG [pmu_ver]                      0x0004
        PMU_CTL_QUERY_HW_CFG [rev_id]                       0x0001
        PMU_CTL_QUERY_HW_CFG [variant_id]                   0x0003
        PMU_CTL_QUERY_HW_CFG [vendor_id]                    0x0041
        gpc_nums[EVT_CORE]                                  6
        gpc_nums[EVT_DSU]                                   6
        gpc_nums[EVT_DMC_CLK]                               2
        gpc_nums[EVT_DMC_CLKDIV2]                           8
        ioctl_events[EVT_CORE].index
        ioctl_events[EVT_CORE].note
        ioctl_events[EVT_DSU].index
        ioctl_events[EVT_DSU].note
        ioctl_events[EVT_DMC_CLK].index
        ioctl_events[EVT_DMC_CLK].note
        ioctl_events[EVT_DMC_CLKDIV2].index
        ioctl_events[EVT_DMC_CLKDIV2].note
```

# Counting model

## Counting core 0 (Ctrl-C to stop counting)
```
> wperf stat -e inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec -c 0
counting core 0...done
Performance counter stats for core 0, no multiplexing, kernel mode excluded:

       counter value event name       event idx
       ============= ==========       =========
            77166198 cycle            fixed
           115949155 inst_spec        0x1b
               94917 vfp_spec         0x75
              811426 ase_spec         0x74
            58864530 dp_spec          0x73
            20454268 ld_spec          0x70
            10034711 st_spec          0x71

               1.746 seconds time elapsed
```

## Counting core 0 for 1 second
```
> wperf stat -e inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec -c 0 sleep 1
counting core 0...done
Performance counter stats for core 0, no multiplexing, kernel mode excluded:

       counter value event name       event idx
       ============= ==========       =========
            74735309 cycle            fixed
           140081548 inst_spec        0x1b
              171192 vfp_spec         0x75
             8184936 ase_spec         0x74
            74158397 dp_spec          0x73
            20907507 ld_spec          0x70
            13349062 st_spec          0x71

               1.093 seconds time elapsed
```

## Specify up to 127 events, they will get multiplexed automatically, for example:
```
> wperf stat -e inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec,br_immed_spec,crypto_spec -c 0 sleep 1
counting core 0...done
Performance counter stats for core 0, multiplexed, kernel mode excluded:

       counter value event name       event idx multiplexed       scaled value
       ============= ==========       ========= ==========       ============
            48759549 cycle            fixed          17/17       48759549
            60333515 inst_spec        0x1b           13/17       78897673
               33565 vfp_spec         0x75           13/17       43892
              584058 ase_spec         0x74           13/17       763768
            28984802 dp_spec          0x73           13/17       37903202
            11258067 ld_spec          0x70           13/17       14722087
             9864771 st_spec          0x71           13/17       12900085
            10906825 br_immed_spec    0x78           12/17       15451335
                 361 crypto_spec      0x77           12/17       511

               1.089 seconds time elapsed
```

## Count using event group
```
> wperf stat -e {inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec},br_immed_spec,crypto_spec -c 0 sleep 1
counting core 0...done

Performance counter stats for core 0, multiplexed, kernel mode included, on Arm Limited core implementation:
note: 'e' - normal event, 'gN' - grouped event with group number N, metric name will be appended if 'e' or 'g' comes from it

       counter value event name       event idx event note   multiplexed scaled value
       ============= ==========       ========= ============ ========== ============
             5540244 cycle            fixed     e              10/10    5540244
             3680561 inst_spec        0x1b      g0              5/10    7361122
               22790 vfp_spec         0x75      g0              5/10    45580
              492283 ase_spec         0x74      g0              5/10    984566
             1689775 dp_spec          0x73      g0              5/10    3379550
              540477 ld_spec          0x70      g0              5/10    1080954
              327594 st_spec          0x71      g0              5/10    655188
              261660 br_immed_spec    0x78      e               5/10    523320
                   0 crypto_spec      0x77      e               5/10    0

               1.092 seconds time elapsed
```

## Count using pre-defined metrics, metric could be used together with -e, no restriction
```
> wperf stat -m imix -e l1i_cache -c 0 sleep 1
counting core 0...done

Performance counter stats for core 0, multiplexed, kernel mode included, on Arm Limited core implementation:
note: 'e' - normal event, 'gN' - grouped event with group number N, metric name will be appended if 'e' or 'g' comes from it

       counter value event name       event idx event note   multiplexed scaled value
       ============= ==========       ========= ============ ========== ============
            17251969 cycle            fixed     e              11/11    17251969
            10807986 inst_spec        0x1b      g0,imix         6/11    19814641
             4504151 dp_spec          0x73      g0,imix         6/11    8257610
              204434 vfp_spec         0x75      g0,imix         6/11    374795
              613224 ase_spec         0x74      g0,imix         6/11    1124244
             2251789 ld_spec          0x70      g0,imix         6/11    4128279
             1076690 st_spec          0x71      g0,imix         6/11    1973931
             1795012 l1i_cache        0x14      e               5/11    3949026

               1.106 seconds time elapsed
```

You can create your own metrics and enable them via custom configuration file. Provide customized config file which describes metrics with `-C <filename>` command line option.

Create a config file named `customized_config`, and add metric(s):
```
> cat customized_config
customizedmetric:{inst_spec,dp_spec,vfp_spec,ase_spec,ldst_spec}
```

Use command line options `-C <filename>` to select metrics configuration file and option `-m` to use new metric, see:
```
> wperf stat -C customized_config -m customizedmetric -c 0 sleep 1
counting ... done

Performance counter stats for core 0, no multiplexing, kernel mode included, on Arm Limited core implementation:
note: 'e' - normal event, 'gN' - grouped event with group number N, metric name will be appended if 'e' or 'g' comes from it

        counter value  event name  event idx  event note
        =============  ==========  =========  ==========
             69080156  cycle       fixed      e
             40601284  inst_spec   0x1b       g0,customizedmetric
             24713551  dp_spec     0x73       g0,customizedmetric
             22479115  vfp_spec    0x75       g0,customizedmetric
               640582  ase_spec    0x74       g0,customizedmetric
             11362693  ldst_spec   0x72       g0,customizedmetric

                1.17 seconds time elapsed
```

## Count on multiple cores simultaneously with -c

In below example we specify events with `-e` and schedule counting on cores 0, 1, 6 and 7. This is done with `-c 0,1,6,7 ` command line option.
Skip `-c` to count on all cores.

Note: when you specify more than one core overall summary will be also printed. See last table with counted events in below listing.

```
>wperf stat -e inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec,br_immed_spec,crypto_spec -c 0,1,6,7 sleep 1
counting ... done

Performance counter stats for core 0, multiplexed, kernel mode included, on Arm Limited core implementation:
note: 'e' - normal event, 'gN' - grouped event with group number N, metric name will be appended if 'e' or 'g' comes from it

        counter value  event name     event idx  event note  multiplexed  scaled value
        =============  ==========     =========  ==========  ===========  ============
            459768620  cycle          fixed      e                 11/11     459768620
             20149697  inst_spec      0x1b       e                  9/11      24627407
            345630022  vfp_spec       0x75       e                  9/11     422436693
            129948697  ase_spec       0x74       e                  8/11     178679458
             61383968  dp_spec        0x73       e                  8/11      84402956
            183535105  ld_spec        0x70       e                  8/11     252360769
               811588  st_spec        0x71       e                  8/11       1115933
                58576  br_immed_spec  0x78       e                  8/11         80542
                  266  crypto_spec    0x77       e                  8/11           365

Performance counter stats for core 1, multiplexed, kernel mode included, on Arm Limited core implementation:
note: 'e' - normal event, 'gN' - grouped event with group number N, metric name will be appended if 'e' or 'g' comes from it

        counter value  event name     event idx  event note  multiplexed  scaled value
        =============  ==========     =========  ==========  ===========  ============
            562807691  cycle          fixed      e                 11/11     562807691
           1107009142  inst_spec      0x1b       e                  9/11    1353011173
               155230  vfp_spec       0x75       e                  9/11        189725
             11943376  ase_spec       0x74       e                  8/11      16422142
            497478218  dp_spec        0x73       e                  8/11     684032549
            175669868  ld_spec        0x70       e                  8/11     241546068
             75332147  st_spec        0x71       e                  8/11     103581702
                42596  br_immed_spec  0x78       e                  8/11         58569
                    0  crypto_spec    0x77       e                  8/11             0

Performance counter stats for core 6, multiplexed, kernel mode included, on Arm Limited core implementation:
note: 'e' - normal event, 'gN' - grouped event with group number N, metric name will be appended if 'e' or 'g' comes from it

        counter value  event name     event idx  event note  multiplexed  scaled value
        =============  ==========     =========  ==========  ===========  ============
              1368553  cycle          fixed      e                 11/11       1368553
               211413  inst_spec      0x1b       e                  9/11        258393
                    0  vfp_spec       0x75       e                  9/11             0
              1424127  ase_spec       0x74       e                  8/11       1958174
              1424127  dp_spec        0x73       e                  8/11       1958174
              1424127  ld_spec        0x70       e                  8/11       1958174
              1424127  st_spec        0x71       e                  8/11       1958174
                    0  br_immed_spec  0x78       e                  8/11             0
                    0  crypto_spec    0x77       e                  8/11             0

Performance counter stats for core 7, multiplexed, kernel mode included, on Arm Limited core implementation:
note: 'e' - normal event, 'gN' - grouped event with group number N, metric name will be appended if 'e' or 'g' comes from it

        counter value  event name     event idx  event note  multiplexed  scaled value
        =============  ==========     =========  ==========  ===========  ============
                    0  cycle          fixed      e                 11/11             0
                    0  inst_spec      0x1b       e                  9/11             0
                    0  vfp_spec       0x75       e                  9/11             0
                    0  ase_spec       0x74       e                  8/11             0
                    0  dp_spec        0x73       e                  8/11             0
                    0  ld_spec        0x70       e                  8/11             0
                    0  st_spec        0x71       e                  8/11             0
                    0  br_immed_spec  0x78       e                  8/11             0
                    0  crypto_spec    0x77       e                  8/11             0

System-wide Overall:
        counter value  event name     event idx  event note  scaled value
        =============  ==========     =========  ==========  ============
           1023944864  cycle          fixed      e             1023944864
           1127370252  inst_spec      0x001b     e             1377896973
            345785252  vfp_spec       0x0075     e              422626418
            143316200  ase_spec       0x0074     e              197059774
            560286313  dp_spec        0x0073     e              770393679
            360629100  ld_spec        0x0070     e              495865011
             77567862  st_spec        0x0071     e              106655809
               101172  br_immed_spec  0x0078     e                 139111
                  266  crypto_spec    0x0077     e                    365

               1.134 seconds time elapsed
```

## Timeline (count multiple times between intervals)

Timeline feature allow users to perform continuous counting (defined with `--timeout <SEC>` command line option) between intervals (defined with `-i <SEC>`) for `N` times (defined with `-n <N>`). For example command:

```
>>wperf stat -m imix -c 1 -t -i 2 -n 3 --timeout 5
counting ... done
sleeping ... done
counting ... done
sleeping ... done
counting ... done
sleeping ... done
```

will perform:
1) Count of `imix` metric (`-m imix`) events for 5 seconds (`--timeout 5`) on CPU core #1 (`-c 1`).
2) Sleep for 2 seconds (`-i 2`)
3) Repeat above count and sleep 3 times (`-n 3`).

Note: use `-v` (verbose) command line option together with timeline to get access to CSV output file name:

```
>wperf stat -m imix -c 1 -t -i 2 -n 3 --timeout 5  -v
timeline file: 'wperf_core_1_2023_09_13_13_31_59.core.csv'
events to be counted:
     5              core events: 0x001b 0x0073 0x0075 0x0074 0x0070
...
```

Hint:
- use `-m <metric>` to capture metric events, and/or `-e <events>` to count additional events.
- use `--json` to additionally return timeline output data in JSON format, add `--output <FILENAME>` to capture output to given file.

Note: to check available events and metrics please use `wperf list` and `wperf list -v` commands. Latter one gives you a bit more information about events and metrics.

### Timeline JSON output file

See how timeline JSON file format looks like for below command:

```
>wperf stat -t -i 1.3 -n 7 --json -m imix --timeout 2.2
```

Please note that `"timeline"` list is an ordered list of all counting occurrences captured. You can see that we pass count interval, duration and timeline count from `wperf` CLI to timeline JSON.

```json
{
    "count_duration": 2.2,
    "count_interval": 1.3,
    "count_timeline": 7,
    "timeline": [
        {
            "core": {
                "Multiplexing": false,
                "Kernel_mode": false,
                "cores": [
                    {
...
```

Hint: you can find timeline JSON schema in [wperf-scripts/tests/schemas/wperf.timeline.schema](https://gitlab.com/Linaro/WindowsPerf/windowsperf/-/blob/main/wperf-scripts/tests/schemas/wperf.timeline.schema?ref_type=heads) file.

### Timeline CSV output file

Timeline command (`-t`) produces [CSV file](https://en.wikipedia.org/wiki/Comma-separated_values). It's format uses comma separated values to distinguish between columns. CSV file name contains core number, current timestamp, name of event counted.

Timeline stores results in a form of a CSV file. Below is an output from above timeline example. Please note that we

```
>type wperf_core_1_2023_09_13_13_24_59.core.csv
Multiplexing,FALSE
Kernel mode,FALSE
Count interval,2.00
Vendor,Arm Limited
Event class,core

core 1,core 1,core 1,core 1,core 1,core 1,
cycle,inst_spec,dp_spec,vfp_spec,ase_spec,ld_spec,
40577993383,1188456413,266887221,2912446099,3216069692,65046013,
339079492,373027981,168147826,3377237,311113,89129873,
385564403,497359205,231406201,4309027,350189,117594750,
```

Timeline file contains header with few counting setting values (these will increase in the future), and rows with column oriented values. These specify cores, events and metrics counted and computed during timeline pass:

#### Specify timeline CSV output file name with --output command line option

Support for `--output` command line in timeline (`-t`) is as follows:

Previously users had to specify `-v` (verbose mode on) with `-t` (timeline command line option) to retrieve from console name of timeline CSV file. Now users can also specify timeline output file name with `--output <FILENAME>` command line option, where `<FILENAME>` is template string for timeline CSV file.

User can specify in `<FILENAME>` few placeholders which can improve timeline file name:
* `{timestamp}` to add current timestamp to output file name. E.g. `2023_09_21_09_42_59` for 21st of September 2023, time: 09:42:59.
* `{class}` to add event class name (e.g. `core`, `dsu`, `dmc_clk`, `dmc_clkdiv2`). Multiple timeline files will be created if user specifies with `-e` events with different classes.
* `{core}` to add `<N>` from `-c <N>` command line option. Note: when more than one core is specified `{core}` will be replaced with 1st core specified.

Examples:

```
>wperf stat -e l1d_cache_rd -t -i 0 --timeout 1 -n 3 -c 1,2,3 -v --output timeline_{core}_{timestamp}_{class}.csv
timeline file: 'timeline_1_2023_09_21_12_21_46_core.csv'
```

```
>wperf stat -e l1d_cache_rd -t -i 0 --timeout 1 -n 3 -c 7 -v --output timeline--{core}--{class}.csv
timeline file: 'timeline--7--core.csv'
```

```
>wperf stat -e l1d_cache_rd -t -i 0 --timeout 1 -n 3 -c 7 -v --output {timestamp}.{core}.{class}.csv
timeline file: '2023_09_21_12_23_58.7.core.csv'
```

#### Timeline CSV file content schema

```
+------------------------------+
|                              |
|       timeline_headers       |
|                              |
+------------------------------+

+------------------------------+
|    timeline_header_cores     |
+------------------------------+
| timeline_header_event_names  | + timeline_header_metric_names
+------------------------------+
| timeline_header_event_values | + timeline_header_metric_values
+------------------------------+
```

### Example counting with Telemetry Solution metric

In case of targets supporting Telemetry Solution metrics users can specify those with `-m` command line option. Because TS metrics contain formulas `wperf` can calculate those based on event occurrences and present metric value in last columns. Metrics are available in CVS file and marked with leading `M@`, e.g. `M@l1d_cache_miss_ratio` or `M@l1d_tlb_mpki` in order to distinguish metric name from event name.

For below command which is using TS metrics `l1d_cache_miss_ratio` and `l1d_tlb_mpki` available on neoverse CPUs:

```
>wperf stat  -m l1d_cache_miss_ratio,l1d_tlb_mpki -c 1 -t -i 1 -n 3
```

We can see two new columns in CSV file on right-hand side: `M@l1d_cache_miss_ratio` and `M@l1d_tlb_mpki`:

```
core 1,     core 1,     core 1,             core 1,         core 1,         core 1,                 core 1,
cycle,      l1d_cache,  l1d_cache_refill,   inst_retired,   l1d_tlb_refill, M@l1d_cache_miss_ratio, M@l1d_tlb_mpki,
2672756503, 3429392628, 18679267,           3949525622,     3947728808,     0.005,                  999.545,
15319613,   6098497,    187612,             16320369,       108408,         0.031,                  6.642,
64449120,   32578964,   451811,             99540434,       283776,         0.014,                  2.851,
```

Where metrics:

* `l1d_cache_miss_ratio` = (`l1d_cache_refill` / `l1d_cache`) [per cache access] and
* `l1d_tlb_mpki` = ((`l1d_tlb_refill` / `inst_retired`) * 1000) [MPKI].

Note: use `wperf list -v` command line option to determine if your CPU supports TS metrics or metrics with defined formula.

## JSON output

You can output JSON instead of human readable tables with `wperf`. We've introduced three new command line flags which should help you emit JSON.
Flag `--json` will emit JSON for tables with values.
Quiet mode can be selected with `-q`. This will suppress human readable printouts. Please note that `--json` implies `-q`.
You can also emit JSON to file directly with `--output <filename>`.

Currently we support `--json` with `stat`, `list` and `test` commands.

### Emit JSON output for simple counting with -json

```
> wperf stat -e inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec -c 0 --json sleep 1
```

Will print on standard output:

```json
{
    "core": {
        "Multiplexing": true,
        "Kernel_mode": false,
        "cores": [
            {
                "core_number": 0,
                "Performance_counter": [
                    {
                        "counter_value": 24369582,
                        "event_name": "cycle",
                        "event_idx": "fixed",
                        "event_note": "e",
                        "multiplexed": "10/10",
                        "scaled_value": 24369582
                    },
                    {
                        "counter_value": 23814308,
                        "event_name": "inst_spec",
                        "event_idx": "0x1b",
                        "event_note": "e",
                        "multiplexed": "9/10",
                        "scaled_value": 26460342
                    },
                    {
                        "counter_value": 29646,
                        "event_name": "vfp_spec",
                        "event_idx": "0x75",
                        "event_note": "e",
                        "multiplexed": "9/10",
                        "scaled_value": 32940
                    },
                    {
                        "counter_value": 3088,
                        "event_name": "ase_spec",
                        "event_idx": "0x74",
                        "event_note": "e",
                        "multiplexed": "8/10",
                        "scaled_value": 3860
                    },
                    {
                        "counter_value": 18298763,
                        "event_name": "dp_spec",
                        "event_idx": "0x73",
                        "event_note": "e",
                        "multiplexed": "8/10",
                        "scaled_value": 22873453
                    },
                    {
                        "counter_value": 2457102,
                        "event_name": "ld_spec",
                        "event_idx": "0x70",
                        "event_note": "e",
                        "multiplexed": "8/10",
                        "scaled_value": 3071377
                    },
                    {
                        "counter_value": 1293874,
                        "event_name": "st_spec",
                        "event_idx": "0x71",
                        "event_note": "e",
                        "multiplexed": "8/10",
                        "scaled_value": 1617342
                    }
                ]
            }
        ],
        "overall": {},
        "ts_metric": {}
    },
    "dsu": {
        "l3metric": {},
        "overall": {}
    },
    "dmc": {
        "pmu": {},
        "ddr": {}
    },
    "Time_elapsed": 1.01
}
```

### Store counting results in JSON file

Print on standard output results of counting and at the same time store these results in JSON file `count.json`.
```
> wperf stat -e inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec -c 0 --output count.json sleep 1
```

### Only store counting results in JSON file

Store counting results in JSON file `count.json` and do not print anything on the screen. Printouts are suppressed with `-q` command line flag.
```
> wperf stat -e inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec -c 0 --output count.json -q sleep 1
```

# Sampling model

## CPython sampling example

In this example we will build CPython from sources and execute simple instructions in Python interactive mode to obtain sampling from CPython runtime image.
To achieve that we will:
* Build CPython binaries targeting ARM64 from sources in debug mode.
* Pin `python_d.exe` interactive console to core no. 1.
* Try to calculate absurdly large integer number [Googolplex](https://en.wikipedia.org/wiki/Googolplex) to stress CPython and get a simple workload.
* Run counting and sampling to obtain some simple event information.

Let's go...

### CPython cross-build on x64 machine targeting ARM64

Let's build CPython locally in debug mode. We will in this example cross-compile CPython to the ARM64 target. Build machine is x64.

```
> git clone git@github.com:python/cpython.git
> cd cpython
> git log -1
commit 1ff81c0cb67215694f084e51c4d35ae53b9f5cf9 (HEAD -> main, origin/main, origin/HEAD)
Author: Eric Snow <ericsnowcurrently@gmail.com>
Date:   Tue Mar 14 10:05:54 2023 -0600
> cd PCBuild
> build.bat -d -p ARM64
...
> arm64>python_d.exe
Python 3.12.0a6+ (heads/main:1ff81c0cb6, Mar 14 2023, 16:26:50) [MSC v.1935 64 bit (ARM64)] on win32
Type "help", "copyright", "credits" or "license" for more information.
>>>
```

Copy above CPython binaries from `PCbuild/arm64` directory to your ARM64 machine. Do not forget the `Lib` directory containing extra libs CPython uses.

### Example 1: sampling CPython executing Googolplex calculation

Pin new CPython process on core no. 1:

```
> start /affinity 2 python_d.exe
```

Check with for example Task Manager if `python_d.exe` is running on core no. 1. Newly created CPython interactive window will allow us to execute example workloads.
In the below example we will calculate a very large integer `10^10^100`.

Note: [start](https://learn.microsoft.com/en-us/windows-server/administration/windows-commands/start) command line switch `/affinity <hexaffinity>` applies the specified processor affinity mask (expressed as a hexadecimal number) to the new application. In our example decimal `2` is `0x02` or `0b0010`. This value denotes core no. 1 as 1 is a 1st bit in the mask, where the mask is indexed from 0 (zero).

```
Python 3.12.0a6+ (heads/main:1ff81c0cb6, Mar 14 2023, 16:26:50) [MSC v.1935 64 bit (ARM64)] on win32
Type "help", "copyright", "credits" or "license" for more information.
>>> 10**10**100
```

#### Counting to asses which events are "popular"

```
>wperf stat -m imix -c 1 sleep 3
counting ... done

Performance counter stats for core 1, no multiplexing, kernel mode excluded, on Arm Limited core implementation:
note: 'e' - normal event, 'gN' - grouped event with group number N, metric name will be appended if 'e' or 'g' comes from it

        counter value  event name  event idx  event note
        =============  ==========  =========  ==========
          23859193503  cycle       fixed      e
           8877337489  inst_spec   0x1b       g0,imix
            712165071  dp_spec     0x73       g0,imix
           3464962917  vfp_spec    0x75       g0,imix
              6647740  ase_spec    0x74       g0,imix
           9116947967  ld_spec     0x70       g0,imix
             13268033  st_spec     0x71       g0,imix

                3.31 seconds time elapsed
```

#### Sampling for `ld_spec` event which, by looking at counting is dominant (at least for `imix` metrics)

Let's sample the `ld_spec` event. Please note that you can specify the process image name and PDB file name with `--pdb_file python_d.pdb` and `--image_name python_d.exe`. In our case `wperf` is able to deduce image name (same as PE file name) and PDB file from PR file name.

We can stop sampling by pressing `Ctrl-C` in the `wperf` console or we can end the process we are sampling.

```
>wperf sample -e ld_spec:100000 --pe_file python_d.exe -c 1
base address of 'python_d.exe': 0x7ff6e0a41270, runtime delta: 0x7ff5a0a40000
sampling ....e.e.e.e.e.eCtrl-C received, quit counting... done!
======================== sample source: ld_spec, top 50 hot functions ========================
 75.39%       579  x_mul:python312_d.dll
  6.51%        50  v_isub:python312_d.dll
  5.60%        43  _Py_atomic_load_32bit_impl:python312_d.dll
  3.12%        24  v_iadd:python312_d.dll
  2.60%        20  PyErr_CheckSignals:python312_d.dll
  2.08%        16  unknown
  1.17%         9  x_add:python312_d.dll
  0.91%         7  _Py_atomic_load_64bit_impl:python312_d.dll
  0.52%         4  _Py_ThreadCanHandleSignals:python312_d.dll
  0.52%         4  _PyMem_DebugCheckAddress:python312_d.dll
  0.26%         2  read_size_t:python312_d.dll
  0.13%         1  _Py_DECREF_SPECIALIZED:python312_d.dll
  0.13%         1  k_mul:python312_d.dll
  0.13%         1  _PyErr_CheckSignalsTstate:python312_d.dll
  0.13%         1  write_size_t:python312_d.dll
  0.13%         1  _PyObject_Malloc:python312_d.dll
  0.13%         1  pymalloc_alloc:python312_d.dll
  0.13%         1  pymalloc_free:python312_d.dll
  0.13%         1  _PyObject_Init:python312_d.dll
  0.13%         1  _PyMem_DebugRawFree:python312_d.dll
  0.13%         1  _PyLong_New:python312_d.dll
```

In the above example we can see that the majority of code executed by CPython's `python_d.exe` executable resides inside the `python312_d.dll` DLL.

Note that in `sampling ....e.e.e.e.e.` progressing printout `.` represents sample payload (of 128 samples)  received from the driver. 'e' represents an unsuccessful attempt to fetch whole sample payload. `wperf` is polling `wperf-driver` awaiting sample payload.

### Example 2: sampling of CPython executable on ARM64 running simple Fibonacci lambda:

Let's execute a new portion of code to see a totally different sampling profile.
Please note that again CPython executes code from its `python312_d.dll`.

```
>>> fib = lambda n: n if n < 2 else fib(n-1) + fib(n-2)
>>> fib (100)
```

Sampling again for `ld_spec`:

```
>wperf sample -e ld_spec:10000 --pe_file python_d.exe --pdb_file python_d.pdb --image_name python_d.exe -c 1
base address of 'python_d.exe': 0x7ff6e0a41270, runtime delta: 0x7ff5a0a40000
sampling ....ee.e.eCtrl-C received, quit counting... done!
======================== sample source: ld_spec, top 50 hot functions ========================
 35.42%       136  _PyEval_EvalFrameDefault:python312_d.dll
  9.38%        36  unicodekeys_lookup_unicode:python312_d.dll
  5.47%        21  _PyFrame_Stackbase:python312_d.dll
  3.91%        15  GETITEM:python312_d.dll
  3.65%        14  dictkeys_get_index:python312_d.dll
  3.39%        13  _Py_DECREF_SPECIALIZED:python312_d.dll
  3.12%        12  _PyFrame_ClearExceptCode:python312_d.dll
  2.86%        11  _PyFrame_Initialize:python312_d.dll
  2.60%        10  DK_UNICODE_ENTRIES:python312_d.dll
  2.60%        10  _Py_dict_lookup:python312_d.dll
  2.60%        10  unicode_get_hash:python312_d.dll
  2.34%         9  clear_thread_frame:python312_d.dll
  2.08%         8  _PyFrame_StackPush:python312_d.dll
  2.08%         8  PyDict_Contains:python312_d.dll
  1.82%         7  Py_INCREF:python312_d.dll
  1.82%         7  _PyThreadState_PopFrame:python312_d.dll
  1.82%         7  _PyErr_Occurred:python312_d.dll
  1.82%         7  medium_value:python312_d.dll
  1.56%         6  get_small_int:python312_d.dll
  1.30%         5  PyTuple_GET_SIZE:python312_d.dll
  1.30%         5  _PyLong_FromSTwoDigits:python312_d.dll
  1.04%         4  Py_XDECREF:python312_d.dll
  1.04%         4  _Py_atomic_load_64bit_impl:python312_d.dll
  0.78%         3  Py_IS_TYPE:python312_d.dll
  0.78%         3  _Py_EnterRecursivePy:python312_d.dll
  0.52%         2  _PyFrame_GetStackPointer:python312_d.dll
  0.52%         2  read_u16:python312_d.dll
  0.52%         2  _PyLong_Add:python312_d.dll
  0.52%         2  _PyFrame_PushUnchecked:python312_d.dll
  0.52%         2  Py_SIZE:python312_d.dll
  0.26%         1  _Py_IncRefTotal:python312_d.dll
  0.26%         1  _PyFrame_SetStackPointer:python312_d.dll
  0.26%         1  unknown
```

## Verbose mode in sampling

We've also added extra prints for verbose mode (`-v`). These add more information about sampling.
See verbose mode on for example 1:

```
>wperf sample -e ld_spec:100000 --pe_file python_d.exe -c 1 -v
================================
                    ADVAPI32.dll          0x000000007fff934e0000          C:\Windows\System32\ADVAPI32.dll
                    KERNEL32.DLL          0x000000007fff92270000          C:\Windows\System32\KERNEL32.DLL
                  KERNELBASE.dll          0x000000007fff90550000          C:\Windows\System32\KERNELBASE.dll
                      RPCRT4.dll          0x000000007fff928f0000          C:\Windows\System32\RPCRT4.dll
               VCRUNTIME140D.dll          0x000000007fff5a040000          C:\Windows\SYSTEM32\VCRUNTIME140D.dll
                     VERSION.dll          0x000000007fff7c040000          C:\Windows\SYSTEM32\VERSION.dll
                      WS2_32.dll          0x000000007fff93410000          C:\Windows\System32\WS2_32.dll
                      bcrypt.dll          0x000000007fff8ee50000          C:\Windows\SYSTEM32\bcrypt.dll
            bcryptprimitives.dll          0x000000007fff900d0000          C:\Windows\System32\bcryptprimitives.dll
                      msvcrt.dll          0x000000007fff91f20000          C:\Windows\System32\msvcrt.dll
                       ntdll.dll          0x000000007fff946c0000          C:\Windows\SYSTEM32\ntdll.dll
                 python312_d.dll          0x000000007fff55cb0000          C:\Users\$USER\Desktop\wperf\merge-retquest\arm64\python312_d.dll
                    python_d.exe          0x000000007ff6e0a40000          C:\Users\$USER\Desktop\wperf\merge-retquest\arm64\python_d.exe
                     sechost.dll          0x000000007fff92470000          C:\Windows\System32\sechost.dll
                   ucrtbased.dll          0x000000007fff558e0000          C:\Windows\SYSTEM32\ucrtbased.dll
================================
                 python312_d.dll          C:\Users\$USER\Desktop\wperf\merge-retquest\arm64\python312_d.dll
                           .text          0x00000000000000001000                        0x87a90d
                          .rdata          0x0000000000000087c000                        0x28b2b1
                           .data          0x00000000000000b08000                        0x137f79
                          .pdata          0x00000000000000c40000                         0x18bf8
                          .idata          0x00000000000000c59000                          0x3ac8
                        PyRuntim          0x00000000000000c5d000                         0x86dec
                          .00cfg          0x00000000000000ce4000                          0x0151
                           .rsrc          0x00000000000000ce5000                          0x0d96
                          .reloc          0x00000000000000ce6000                         0x20332
                    python_d.exe          C:\Users\$USER\Desktop\wperf\merge-retquest\arm64\python_d.exe
                           .text          0x00000000000000001000                          0x6b81
                          .rdata          0x00000000000000008000                          0x1a36
                           .data          0x0000000000000000a000                          0x01e1
                          .pdata          0x0000000000000000b000                          0x0320
                          .idata          0x0000000000000000c000                          0x0b34
                          .00cfg          0x0000000000000000d000                          0x0151
                           .rsrc          0x0000000000000000e000                         0x17cc9
                          .reloc          0x00000000000000026000                          0x01af
base address of 'python_d.exe': 0x7ff6e0a41270, runtime delta: 0x7ff5a0a40000
sampling ....e.e.eCtrl-C received, quit counting... done!
=================
sample generated: 516
sample dropped  : 4
======================== sample source: ld_spec, top 50 hot functions ========================
 68.49%       263  x_mul:python312_d.dll
                   0x000000007fff56054b8c       82
                   0x000000007fff56054bbc       63
                   0x000000007fff56054be4       34
                   0x000000007fff56054b54       19
                   0x000000007fff56054bac       19
                   0x000000007fff56054b78       10
                   0x000000007fff56054b58       10
                   0x000000007fff56054b60        5
                   0x000000007fff56054bb8        3
                   0x000000007fff56054bec        2
  8.33%        32  v_isub:python312_d.dll
                   0x000000007fff560532b0        9
                   0x000000007fff560532ac        7
                   0x000000007fff560532f0        4
                   0x000000007fff560532c4        4
                   0x000000007fff560532e4        2
                   0x000000007fff56053298        2
                   0x000000007fff56053278        1
                   0x000000007fff560532a0        1
                   0x000000007fff56053280        1
                   0x000000007fff560532cc        1
  4.95%        19  _Py_atomic_load_32bit_impl:python312_d.dll
                   0x000000007fff55de8e6c        5
                   0x000000007fff55de8e74        4
                   0x000000007fff55de8e30        2
                   0x000000007fff55de8e48        2
                   0x000000007fff55de8d48        2
                   0x000000007fff55de8e50        1
                   0x000000007fff55de8e88        1
                   0x000000007fff55de8e2c        1
                   0x000000007fff55de8e64        1
  4.43%        17  unknown
                   0x000000007fff946d1f10        5
                   0x0000fffff801021c31dc        1
                   0x000000007fff5a0419a4        1
                   0x000000007fff947810f4        1
                   0x000000007fff55b2b7a8        1
                   0x000000007fff55cb3c94        1
                   0x000000007fff946d1250        1
                   0x000000007fff5a04199c        1
                   0x000000007fff558eb7c4        1
                   0x000000007fff5a041660        1
  4.17%        16  v_iadd:python312_d.dll
                   0x000000007fff56053054        4
                   0x000000007fff5605306c        3
                   0x000000007fff56053098        2
                   0x000000007fff5605308c        2
                   0x000000007fff56053028        1
                   0x000000007fff56053058        1
                   0x000000007fff56053048        1
                   0x000000007fff560530c8        1
                   0x000000007fff560530a0        1
  2.34%         9  x_add:python312_d.dll
                   0x000000007fff56053858        3
                   0x000000007fff5605385c        3
                   0x000000007fff56053870        3
  1.82%         7  PyErr_CheckSignals:python312_d.dll
                   0x000000007fff55e9acac        3
                   0x000000007fff55e9ade0        1
                   0x000000007fff55e9ac98        1
                   0x000000007fff55e9aca0        1
                   0x000000007fff55e9ada8        1
  1.30%         5  _Py_atomic_load_64bit_impl:python312_d.dll
                   0x000000007fff55d04240        2
                   0x000000007fff55d04138        2
                   0x000000007fff55d041f0        1
  0.78%         3  read_size_t:python312_d.dll
                   0x000000007fff56081944        3
  0.78%         3  _PyMem_DebugCheckAddress:python312_d.dll
                   0x000000007fff5607b08c        2
                   0x000000007fff5607b028        1
  0.78%         3  _Py_ThreadCanHandleSignals:python312_d.dll
                   0x000000007fff55e9cc8c        1
                   0x000000007fff55e9ccd8        1
                   0x000000007fff55e9ccb0        1
  0.26%         1  pymalloc_alloc:python312_d.dll
                   0x000000007fff560810ec        1
  0.26%         1  long_normalize:python312_d.dll
                   0x000000007fff5604edbc        1
  0.26%         1  pymalloc_pool_extend:python312_d.dll
                   0x000000007fff56081400        1
  0.26%         1  k_mul:python312_d.dll
                   0x000000007fff56047c14        1
  0.26%         1  PyThread_get_thread_ident:python312_d.dll
                   0x000000007fff564a1ac4        1
  0.26%         1  PyGILState_Check:python312_d.dll
                   0x000000007fff563f9870        1
  0.26%         1  _PyLong_New:python312_d.dll
                   0x000000007fff56042adc        1
100.00%       384  top 18 in total
```

IN above example:

```
 68.49%       263  x_mul:python312_d.dll
                   0x000000007fff56054b8c       82
                   0x000000007fff56054bbc       63
                   0x000000007fff56054be4       34
                   0x000000007fff56054b54       19
                   0x000000007fff56054bac       19
                   0x000000007fff56054b78       10
                   0x000000007fff56054b58       10
                   0x000000007fff56054b60        5
                   0x000000007fff56054bb8        3
                   0x000000007fff56054bec        2
```

represents a set of samples which were coming from the single symbol `x_mul` originated in `python312_d.dll` DLL.
Below hexadecimal values represent PC values which were sampled with corresponding sample count.

## Using the `record` command

The `record` command spawns the process and pins it to the core specified by the `-c` option. You can either use
`--pe_file` to let WindowsPerf know which process to spawn or after all the options to WindowsPerf just type the command
you would like to execute. For example:

```
>wperf record -e vfp_spec -c 1 --pe_file main.exe --timeout 1
```

or:

```
>wperf record -e vfp_spec -c 1 --timeout 1 -- main.exe
```

If you want to pass command line arguments to your application you can just call it after all WindowsPerf options, all command line arguments are going to be passed
verbatim to the program that is being spawned. If you want to execute the CPython example above using this approach you could just type:

```
>wperf record -e ld_spec:100000 -c 1 --timeout 30 -- python_d.exe -c 10**10**1000
```

### wperf "--" (double-dash) support

A double-dash (`--`) is a syntax used in shell commands to signify end of command options and beginning of positional arguments. In other words, it separates `wperf` CLI options from arguments that command operates on. Use `--` to separate `wperf.exe` command line options from process you want to spawn followed by its verbatim arguments.

```
>wperf [OPTIONS] -- PROCESS_NAME [ARGS]
```
