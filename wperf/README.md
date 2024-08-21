# wperf

[[_TOC_]]

# Build wperf CLI

You can build `wperf` project from the command line:

```
> devenv windowsperf.sln /Rebuild "Debug|ARM64" /Project wperf\wperf.vcxproj
```

## Inject ETW trace from wperf

New macro, `ENABLE_ETW_TRACING_APP` is used to control ETW output inside the `wperf`.

# Usage of wperf

```
WindowsPerf ver. 3.7.2 (489c1443/Debug+etw-app) WOA profiling with performance counters.
Report bugs to: https://gitlab.com/Linaro/WindowsPerf/windowsperf/-/issues

NAME:
    wperf - Performance analysis tools for Windows on Arm

SYNOPSIS:
    wperf [--version] [--help] [OPTIONS]

    wperf stat [-e] [-m] [-t] [-i] [-n] [-c] [-C] [-E] [-k] [--dmc] [-q] [--json]
               [--output] [--config] [--force-lock]
    wperf stat [-e] [-m] [-t] [-i] [-n] [-c] [-C] [-E] [-k] [--dmc] [-q] [--json]
               [--output] [--config] -- COMMAND [ARGS]
        Counting mode, for obtaining aggregate counts of occurrences of special
        events.

    wperf sample [-e] [--timeout] [-c] [-C] [-E] [-q] [--json] [--output] [--config]
                 [--image_name] [--pe_file] [--pdb_file] [--sample-display-long] [--force-lock]
                 [--sample-display-row] [--record_spawn_delay] [--annotate] [--disassemble]
        Sampling mode, for determining the frequencies of event occurrences
        produced by program locations at the function, basic block, and/or
        instruction levels.

    wperf record [-e] [--timeout] [-c] [-C] [-E] [-q] [--json] [--output] [--config]
                 [--image_name] [--pe_file] [--pdb_file] [--sample-display-long] [--force-lock]
                 [--sample-display-row] [--record_spawn_delay] [--annotate] [--disassemble] -- COMMAND [ARGS]
        Same as sample but also automatically spawns the process and pins it to
        the core specified by `-c`. Process name is defined by COMMAND. User can
        pass verbatim arguments to the process with [ARGS].

    wperf list [-v] [--json] [--force-lock]
        List supported events and metrics. Enable verbose mode for more details.

    wperf test [--json] [OPTIONS]
        Configuration information about driver and application.

    wperf detect [--json] [OPTIONS]
        List installed WindowsPerf-like Kernel Drivers (match GUID).

    wperf man [--json]
        Plain text information about one or more specified event(s), metric(s), and or group metric(s).

OPTIONS:
    -h, --help
        Run wperf help command.

    --version
        Display version.

    -v, --verbose
        Enable verbose output also in JSON output.

    -q
        Quiet mode, no output is produced.

    -e
        Specify comma separated list of event names (or raw events) to count, for
        example `ld_spec,vfp_spec,r10`. Use curly braces to group events.
        Specify comma separated list of event names with sampling frequency to
        sample, for example `ld_spec:100000`.

        Raw events: specify raw evens with `r<VALUE>` where `<VALUE>` is a 16-bit
        hexadecimal event index value without leading `0x`. For example `r10` is
        event with index `0x10`.

        Note: see list of available event names using `list` command.

    -m
        Specify comma separated list of metrics to count.

        Note: see list of available metric names using `list` command.

    --timeout
        Specify counting or sampling duration. If not specified, press
        Ctrl+C to interrupt counting or sampling. Input may be suffixed by
        one (or none) of the following units, with up to 2 decimal
        points: "ms", "s", "m", "h", "d" (i.e. milliseconds, seconds,
        minutes, hours, days). If no unit is provided, the default unit
        is seconds. Accuracy is 0.1 sec.

    -t
        Enable timeline mode (count multiple times with specified interval).
        Use `-i` to specify timeline interval, and `-n` to specify number of
        counts.

    -i
        Specify counting interval. `0` seconds is allowed. Input may be
        suffixed with one (or none) of the following units, with up to
        2 decimal points: "ms", "s", "m", "h", "d" (i.e. milliseconds,
        seconds, minutes, hours, days). If no unit is provided, the default
        unit is seconds (60s by default).

    -n
        Number of consecutive counts in timeline mode (disabled by default).

    --annotate
        Enable translating addresses taken from samples in sample/record mode into source code line numbers.

    --disassemble
        Enable disassemble output on sampling mode. Implies 'annotate'.

    --image_name
        Specify the image name you want to sample.

    --pe_file
        Specify the PE filename (and path).

    --pdb_file
        Specify the PDB filename (and path), PDB file should directly
        corresponds to a PE file set with `--pe_file`.

    --sample-display-long
        Display decorated symbol names.

    --sample-display-row
        Set how many samples you want to see in the summary (50 by default).

    --record_spawn_delay
        Set the waiting time, in milliseconds, before reading process data after
        spawning it with `record`.

    --force-lock
        Force driver to give lock to current `wperf` process, use when you want
        to interrupt currently executing `wperf` session or to recover from the lock.

    -c, --cpu
        Specify comma separated list of CPU cores, and or ranges of CPU cores, to count
        on, or one CPU to sample on.

    -k
        Count kernel mode as well (disabled by default).

    --dmc
        Profile on the specified DDR controller. Skip `--dmc` to count on all
        DMCs.

    -C
        Provide customized config file which describes metrics.

    -E
        Provide customized config file which describes custom events or
        provide custom events from the command line.

    --json
        Define output type as JSON.

    --output, -o
        Specify JSON output filename.

    --output-csv
        Specify CSV output filename. Only with timeline `-t`.

    --output-prefix, --cwd
         Set current working dir for storing output JSON and CSV file.

    --config
        Specify configuration parameters.

OPTIONS aliases:
    -l
        Alias of 'list'.

    sleep
        Alias of `--timeout`.

EXAMPLES:

    > wperf list -v
    List all events and metrics available on your host with extended
    information.

    > wperf stat -e inst_spec,vfp_spec,ase_spec,ld_spec -c 0 --timeout 3
    Count events `inst_spec`, `vfp_spec`, `ase_spec` and `ld_spec` on core #0
    for 3 seconds.

    > wperf stat -m imix -e l1i_cache -c 7 --timeout 10.5
    Count metric `imix` (metric events will be grouped) and additional event
    `l1i_cache` on core #7 for 10.5 seconds.

    > wperf stat -m imix -c 1 -t -i 2 -n 3 --timeout 5
    Count in timeline mode (output counting to CSV file) metric `imix` 3 times
    on core #1 with 2 second intervals (delays between counts). Each count
    will last 5 seconds.

    > wperf sample -e ld_spec:100000 --pe_file python_d.exe -c 1
    Sample event `ld_spec` with frequency `100000` already running process
    `python_d.exe` on core #1. Press Ctrl+C to stop sampling and see the results.

    > wperf record -e ld_spec:100000 -c 1 --timeout 30 -- python_d.exe -c 10**10**100
    Launch `python_d.exe -c 10**10**100` process and start sampling event `ld_spec`
    with frequency `100000` on core #1 for 30 seconds.
    Hint: add `--annotate` or `--disassemble` to `wperf record` command line
    parameters to increase sampling "resolution".
```

# WindowsPerf Driver lock/unlock feature

When `wperf` communicates with WindowsPerf Kernel Driver, driver acquires lock and will deny access to other instances of `wperf` to access the driver and its resources.
This prevents others from interfering with current `wperf` run and protects you from interference with your count.

When other `wperf` process "locked" access to the driver you will see below warning:

```
>wperf --version
warning: other WindowsPerf process acquired the wperf-driver.
Operation cancelled!
```

You can force the lock (and kick-out other `wperf` process from accessing Kernel Driver) with `--force-lock` command line option:

```
>wperf --version --force-lock
        Component     Version  GitVer
        =========     =======  ======
        wperf         3.3.0    fb7f8c66
        wperf-driver  3.3.0    fb8d521c
```

Process that was forced out and lost the lock will fail with below warning:

```
>wperf stat -m imix -c 1
...
warning: other WindowsPerf process hijacked (forced lock) the wperf-driver, see --force-lock.
Operation terminated, your data was lost!
```

> :warning: Use `--force-lock` to recover from `wperf` crash that could cause lock. You can issue simple `wperf --version --force-lock` command to recover.

# WindowsPerf auxiliary command line options

## List available PMU events, metrics and groups of metrics with `list`

```
>wperf list
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

List of supported metrics (to be used in -m)

        Metric                      Events
        ======                      ======
        backend_stalled_cycles      {cpu_cycles,stall_backend}
        branch_misprediction_ratio  {br_mis_pred_retired,br_retired}
        branch_mpki                 {br_mis_pred_retired,inst_retired}
        branch_percentage           {br_immed_spec,br_indirect_spec,inst_spec}
        crypto_percentage           {crypto_spec,inst_spec}
        dcache                      {l1d_cache,l1d_cache_refill,l2d_cache,l2d_cache_refill,inst_retired}
        ddr_bw                      /dmc_clkdiv2/rdwr
        dtlb                        {l1d_tlb,l1d_tlb_refill,l2d_tlb,l2d_tlb_refill,inst_retired}
        dtlb_mpki                   {dtlb_walk,inst_retired}
        dtlb_walk_ratio             {dtlb_walk,l1d_tlb}
        frontend_stalled_cycles     {cpu_cycles,stall_frontend}
...
        l1i_cache_mpki              {inst_retired,l1i_cache_refill}
        l1i_tlb_miss_ratio          {l1i_tlb,l1i_tlb_refill}
        l1i_tlb_mpki                {inst_retired,l1i_tlb_refill}
        l2_cache_miss_ratio         {l2d_cache,l2d_cache_refill}
        l2_cache_mpki               {inst_retired,l2d_cache_refill}
        l2_tlb_miss_ratio           {l2d_tlb,l2d_tlb_refill}
        l2_tlb_mpki                 {inst_retired,l2d_tlb_refill}
        l3_cache                    /dsu/l3d_cache,/dsu/l3d_cache_refill
        ll_cache_read_hit_ratio     {ll_cache_miss_rd,ll_cache_rd}
        ll_cache_read_miss_ratio    {ll_cache_miss_rd,ll_cache_rd}
        ll_cache_read_mpki          {inst_retired,ll_cache_miss_rd}
        load_percentage             {inst_spec,ld_spec}
        scalar_fp_percentage        {inst_spec,vfp_spec}
        simd_percentage             {ase_spec,inst_spec}
        store_percentage            {inst_spec,st_spec}
...

List of supported groups of metrics (to be used in -m)

        Group                    Metrics
        =====                    =======
        Branch_Effectiveness     branch_mpki,branch_misprediction_ratio
        Cycle_Accounting         frontend_stalled_cycles,backend_stalled_cycles
        DTLB_Effectiveness       dtlb_mpki,l1d_tlb_mpki,l2_tlb_mpki,dtlb_walk_ratio,l1d_tlb_miss_ratio,l2_tlb_miss_ratio
        General                  ipc
        ITLB_Effectiveness       itlb_mpki,l1i_tlb_mpki,l2_tlb_mpki,itlb_walk_ratio,l1i_tlb_miss_ratio,l2_tlb_miss_ratio
        L1D_Cache_Effectiveness  l1d_cache_mpki,l1d_cache_miss_ratio
        L1I_Cache_Effectiveness  l1i_cache_mpki,l1i_cache_miss_ratio
        L2_Cache_Effectiveness   l2_cache_mpki,l2_cache_miss_ratio
        LL_Cache_Effectiveness   ll_cache_read_mpki,ll_cache_read_miss_ratio,ll_cache_read_hit_ratio
        MPKI                     branch_mpki,itlb_mpki,l1i_tlb_mpki,dtlb_mpki,l1d_tlb_mpki,l2_tlb_mpki,l1i_cache_mpki,l1d_cache_mpki,l2_cache_mpki,ll_cache_read_mpki
        Miss_Ratio               branch_misprediction_ratio,itlb_walk_ratio,dtlb_walk_ratio,l1i_tlb_miss_ratio,l1d_tlb_miss_ratio,l2_tlb_miss_ratio,l1i_cache_miss_ratio,l1d_cache_miss_ratio,l2_cache_miss_ratio,ll_cache_read_miss_ratio
        Operation_Mix            load_percentage,store_percentage,integer_dp_percentage,simd_percentage,scalar_fp_percentage,branch_percentage,crypto_percentage

```

## List PMU events, metrics and groups of metrics detailed information with `man`

Obtain information about a specific set of PMU events, metrics and groups of metrics with `man` (in manual style).
Use `wperf list [-v]` command to list all available events, metrics or groups of metrics first.

```
>wperf man ld_spec

NAME
    ld_spec
DESCRIPTION
    Operation speculatively executed
```

Note: To learn about events, metrics or groups of metrics available on other CPUs, prefix the option with the CPU name (default is detected CPU) followed by `/` and name.

```
>wperf man neoverse-v1/fp_fixed_ops_spec,neoverse-n1/ld_spec

NAME
    fp_fixed_ops_spec
DESCRIPTION
    Non-scalable floating-point element ALU operations speculatively execute
NAME
    ld_spec
DESCRIPTION
    Operation speculatively executed
```

## Obtain information about `WindowsPerf` configuration with `test`

```
>wperf test
        Test Name                                           Result
        =========                                           ======
        request.ioctl_events [EVT_CORE]                     False
        request.ioctl_events [EVT_DSU]                      False
        request.ioctl_events [EVT_DMC_CLK/EVT_DMC_CLKDIV2]  False
        pmu_device.vendor_name                              Arm Limited
        pmu_device.product_name                             neoverse-n1
        pmu_device.product_name(extended)                   Neoverse N1 (neoverse-n1), armv8.1, pmu_v3
        pmu_device.product []                               armv8-a,armv9-a,neoverse-n1,neoverse-n2,neoverse-n2-r0p0,neoverse-n2-r0p1,neoverse-n2-r0p3,neoverse-v1
        pmu_device.m_product_alias                          (neoverse-n2-r0p0:neoverse-n2),(neoverse-n2-r0p1:neoverse-n2)
        pmu_device.events_query(events) [EVT_CORE]          110
        pmu_device.events_query(events) [EVT_DSU]           9
        pmu_device.events_query(events) [EVT_DMC_CLK]       3
        pmu_device.events_query(events) [EVT_DMC_CLKDIV2]   26
        pmu_device.sampling.INTERVAL_DEFAULT                0x4000000
        pmu_device.version_name                             FEAT_PMUv3p1
        PMU_CTL_QUERY_HW_CFG [arch_id]                      0x000f
        PMU_CTL_QUERY_HW_CFG [core_num]                     0x0050
        PMU_CTL_QUERY_HW_CFG [fpc_num]                      0x0001
        PMU_CTL_QUERY_HW_CFG [gpc_num]                      0x0006
        PMU_CTL_QUERY_HW_CFG [total_gpc_num]                0x0006
        PMU_CTL_QUERY_HW_CFG [part_id]                      0x0d0c
        PMU_CTL_QUERY_HW_CFG [pmu_ver]                      0x0004
        PMU_CTL_QUERY_HW_CFG [rev_id]                       0x0001
        PMU_CTL_QUERY_HW_CFG [variant_id]                   0x0003
        PMU_CTL_QUERY_HW_CFG [vendor_id]                    0x0041
        PMU_CTL_QUERY_HW_CFG [midr_value]                   0x000000000000413fd0c1
        PMU_CTL_QUERY_HW_CFG [id_aa64dfr0_value]            0x00000000000110305408
        PMU_CTL_QUERY_HW_CFG [counter_idx_map]              0,1,2,3,4,5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,31
        PMU_CTL_QUERY_HW_CFG [device_id_str]                core.stat=core;core.sample=core;dsu.stat=dsu;dmc.stat=dmc_clk,dmc_clkdiv2
        gpc_nums[EVT_CORE]                                  6
        gpc_nums[EVT_DSU]                                   6
        gpc_nums[EVT_DMC_CLK]                               2
        gpc_nums[EVT_DMC_CLKDIV2]                           8
        fpc_nums[EVT_CORE]                                  1
        fpc_nums[EVT_DSU]                                   1
        fpc_nums[EVT_DMC_CLK]                               0
        fpc_nums[EVT_DMC_CLKDIV2]                           0
        ioctl_events[EVT_CORE].index
        ioctl_events[EVT_CORE].note
        ioctl_events[EVT_DSU].index
        ioctl_events[EVT_DSU].note
        ioctl_events[EVT_DMC_CLK].index
        ioctl_events[EVT_DMC_CLK].note
        ioctl_events[EVT_DMC_CLKDIV2].index
        ioctl_events[EVT_DMC_CLKDIV2].note
        config.count.period                                 100
        config.count.period_max                             100
        config.count.period_min                             10
        spe_device.version_name                             FEAT_SPE
```

## Enumerate devices with WindowsPerf Kernel Driver GUID

```
>wperf detect
        Device Instance ID                                           Hardware IDs
        ==================                                           ============
        \\?\ROOT#SYSTEM#0001#{f8047fdd-7083-4c2e-90ef-c0c73f1045fd}  Root\WPERFDRIVER
```

# Counting model

## Counting core 0 (Ctrl-C to stop counting)
```
>wperf stat -e inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec -c 0
counting ...

Performance counter stats for core 0, no multiplexing, kernel mode excluded, on Arm Limited core implementation:
note: 'e' - normal event, 'gN' - grouped event with group number N, metric name will be appended if 'e' or 'g' comes from it

        counter value  event name  event idx  event note
        =============  ==========  =========  ==========
           46,009,833  cycle       fixed      e
           67,113,005  inst_spec   0x1b       e
              167,775  vfp_spec    0x75       e
            1,944,625  ase_spec    0x74       e
           31,513,052  dp_spec     0x73       e
           12,580,829  ld_spec     0x70       e
            8,639,941  st_spec     0x71       e

               3.823 seconds time elapsed
```

## Counting core 0 for 1 second
```
>wperf stat -e inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec -c 0 sleep 1
counting ... done

Performance counter stats for core 0, no multiplexing, kernel mode excluded, on Arm Limited core implementation:
note: 'e' - normal event, 'gN' - grouped event with group number N, metric name will be appended if 'e' or 'g' comes from it

        counter value  event name  event idx  event note
        =============  ==========  =========  ==========
           13,612,036  cycle       fixed      e
           16,652,937  inst_spec   0x1b       e
               80,106  vfp_spec    0x75       e
              774,828  ase_spec    0x74       e
            7,632,994  dp_spec     0x73       e
            3,200,780  ld_spec     0x70       e
            1,792,766  st_spec     0x71       e

               1.122 seconds time elapsed
```

## Specify up to 127 events, they will get multiplexed automatically, for example:
```
>wperf stat -e inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec,br_immed_spec,crypto_spec -c 0 sleep 1
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
>wperf stat -e {inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec},br_immed_spec,crypto_spec -c 0 sleep 1
counting ... done

Performance counter stats for core 0, multiplexed, kernel mode excluded, on Arm Limited core implementation:
note: 'e' - normal event, 'gN' - grouped event with group number N, metric name will be appended if 'e' or 'g' comes from it

        counter value  event name     event idx  event note  multiplexed  scaled value
        =============  ==========     =========  ==========  ===========  ============
           19,724,652  cycle          fixed      e                 10/10    19,724,652
           26,213,576  inst_spec      0x1b       g0                 5/10    52,427,152
               46,559  vfp_spec       0x75       g0                 5/10        93,118
              816,117  ase_spec       0x74       g0                 5/10     1,632,234
           12,511,546  dp_spec        0x73       g0                 5/10    25,023,092
            4,697,973  ld_spec        0x70       g0                 5/10     9,395,946
            3,492,198  st_spec        0x71       g0                 5/10     6,984,396
              439,463  br_immed_spec  0x78       e                  5/10       878,926
                    0  crypto_spec    0x77       e                  5/10             0

               1.091 seconds time elapsed
```

## Count using pre-defined metrics, metric could be used together with -e, no restriction
```
>wperf stat -m imix -e l1i_cache -c 0 sleep 1
counting ... done

Performance counter stats for core 0, multiplexed, kernel mode excluded, on Arm Limited core implementation:
note: 'e' - normal event, 'gN' - grouped event with group number N, metric name will be appended if 'e' or 'g' comes from it

        counter value  event name  event idx  event note  multiplexed  scaled value
        =============  ==========  =========  ==========  ===========  ============
           11,851,602  cycle       fixed      e                 10/10    11,851,602
           10,467,162  inst_spec   0x1b       g0,imix            5/10    20,934,324
            4,740,558  dp_spec     0x73       g0,imix            5/10     9,481,116
               33,819  vfp_spec    0x75       g0,imix            5/10        67,638
              739,099  ase_spec    0x74       g0,imix            5/10     1,478,198
            1,973,970  ld_spec     0x70       g0,imix            5/10     3,947,940
            1,077,824  st_spec     0x71       g0,imix            5/10     2,155,648
            1,408,408  l1i_cache   0x14       e                  5/10     2,816,816

               1.097 seconds time elapsed
```

You can create your own metrics and enable them via custom configuration file. Provide customized config file which describes metrics with `-C <filename>` command line option.

Create a config filenamed `customized_config`, and add metric(s):
```
> cat customized_config
customizedmetric:{inst_spec,dp_spec,vfp_spec,ase_spec,ldst_spec}
```

Use command line options `-C <filename>` to select metrics configuration file and option `-m` to use new metric, see:
```
>wperf stat -C customized_config -m customizedmetric -c 0 sleep 1
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

## Count using pre-defined groups of metrics

For some CPUs (e.g. `neoverse-n1`) Arm Telemetry Solution team defined metrics, and groups of metrics you can use to simplify your analysis.

```
>wperf stat -m Operation_Mix -c 7 --timeout 3 -- cpython\PCbuild\arm64\python_d.exe -c 10**10**100
counting ... done

Performance counter stats for core 7, multiplexed, kernel mode excluded, on Arm Limited core implementation:
note: 'e' - normal event, 'gN' - grouped event with group number N, metric name will be appended if 'e' or 'g' comes from it

        counter value  event name        event idx  event note                multiplexed    scaled value
        =============  ==========        =========  ==========                ===========    ============
        6,592,321,176  cycle             fixed      e                               33/33   6,592,321,176
        4,734,051,866  inst_spec         0x1b       g0,load_percentage              11/33  14,202,155,598
        1,787,545,156  ld_spec           0x70       g0,load_percentage              11/33   5,362,635,468
        4,734,051,866  inst_spec         0x1b       g1,store_percentage             11/33  14,202,155,598
          665,908,289  st_spec           0x71       g1,store_percentage             11/33   1,997,724,867
        1,809,734,797  dp_spec           0x73       g2,integer_dp_percentage        11/33   5,429,204,391
        4,734,051,866  inst_spec         0x1b       g2,integer_dp_percentage        11/33  14,202,155,598
              872,144  ase_spec          0x74       g3,simd_percentage              11/33       2,616,432
        4,173,505,464  inst_spec         0x1b       g3,simd_percentage              11/33  12,520,516,392
        4,173,505,464  inst_spec         0x1b       g4,scalar_fp_percentage         11/33  12,520,516,392
                    0  vfp_spec          0x75       g4,scalar_fp_percentage         11/33               0
          373,516,721  br_immed_spec     0x78       g5,branch_percentage            11/33   1,120,550,163
           28,900,861  br_indirect_spec  0x7a       g5,branch_percentage            11/33      86,702,583
        4,141,998,024  inst_spec         0x1b       g5,branch_percentage            11/33  12,425,994,072
                    0  crypto_spec       0x77       g6,crypto_percentage            11/33               0
        4,141,998,024  inst_spec         0x1b       g6,crypto_percentage            11/33  12,425,994,072

Telemetry Solution Metrics:
        core  product_name  metric_name             value  unit
        ====  ============  ===========             =====  ====
           7  neoverse-n1   branch_percentage       9.716  percent of operations
           7  neoverse-n1   crypto_percentage       0.000  percent of operations
           7  neoverse-n1   integer_dp_percentage  38.228  percent of operations
           7  neoverse-n1   load_percentage        37.759  percent of operations
           7  neoverse-n1   scalar_fp_percentage    0.000  percent of operations
           7  neoverse-n1   simd_percentage         0.021  percent of operations
           7  neoverse-n1   store_percentage       14.066  percent of operations

               3.303 seconds time elapsed
```

See how CPython computation of `10^10^100` is `integer_dp_percentage` and `load_percentage` bound.

## Available units for --timeout, sleep & -i

Options `--timeout`, `sleep` and `-i` can be used with a number along with one, or none, of the supported units: `ms` (milliseconds), `s` (seconds), `m` (minutes), `h` (hours), `d` (days).

The following restrictions apply:
* The default unit `seconds` is used if no unit is provided as input.
* Units may not be used in conjunction with one another: `--timeout 1m30s` is not accepted.
* Decimals may be up to 2 decimal places, and must be preceded by a single `0`: `.001h` is not accepted.
* Padded `0`s are not permitted: `01h` is not accepted.

See the following for examples of correct usage:

```
>wperf stat -c 0 -e ld_spec --timeout 10

>wperf stat -c 0 -e ld_spec --timeout 1.00h

>wperf stat -c 0 -e ld_spec sleep 750ms

>wperf stat -c 0 -e ld_spec sleep 5m
```

represent a duration of `10`, `3600`, `0.75` & `300` seconds respectively.

Similarly:
```
> wperf stat -m imix -c 0 -t -i 36 --timeout 1 -n 1

> wperf stat -m imix -c 0 -t -i 3600.00ms --timeout 1 -n 1

> wperf stat -m imix -c 0 -t -i 0.01h --timeout 1 -n 1
```

all represent a counting interval of `36` seconds.

## Count on multiple cores simultaneously with -c

In below example we specify events with `-e` and schedule counting on cores 0, 1, 6 and 7. This is done with `-c 0,1,6,7 ` command line option.
Skip `-c` to count on all cores.

Note: when you specify more than one core overall summary will be also printed. See last table with counted events in below listing.

```
>wperf stat -e inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec,br_immed_spec,crypto_spec -c 0,1,6,7 sleep 1
counting ... done

Performance counter stats for core 0, multiplexed, kernel mode excluded, on Arm Limited core implementation:
note: 'e' - normal event, 'gN' - grouped event with group number N, metric name will be appended if 'e' or 'g' comes from it

        counter value  event name     event idx  event note  multiplexed  scaled value
        =============  ==========     =========  ==========  ===========  ============
            9,570,473  cycle          fixed      e                 10/10     9,570,473
           10,267,263  inst_spec      0x1b       e                  8/10    12,834,078
               44,365  vfp_spec       0x75       e                  8/10        55,456
              599,660  ase_spec       0x74       e                  8/10       749,575
            5,084,557  dp_spec        0x73       e                  8/10     6,355,696
            1,800,692  ld_spec        0x70       e                  7/10     2,572,417
              960,444  st_spec        0x71       e                  7/10     1,372,062
              707,208  br_immed_spec  0x78       e                  7/10     1,010,297
                    0  crypto_spec    0x77       e                  7/10             0

Performance counter stats for core 1, multiplexed, kernel mode excluded, on Arm Limited core implementation:
note: 'e' - normal event, 'gN' - grouped event with group number N, metric name will be appended if 'e' or 'g' comes from it

        counter value  event name     event idx  event note  multiplexed  scaled value
        =============  ==========     =========  ==========  ===========  ============
           10,094,676  cycle          fixed      e                 10/10    10,094,676
           16,079,022  inst_spec      0x1b       e                  8/10    20,098,777
               41,049  vfp_spec       0x75       e                  8/10        51,311
              760,031  ase_spec       0x74       e                  8/10       950,038
            7,681,186  dp_spec        0x73       e                  8/10     9,601,482
            2,609,380  ld_spec        0x70       e                  7/10     3,727,685
            2,218,466  st_spec        0x71       e                  7/10     3,169,237
              312,946  br_immed_spec  0x78       e                  7/10       447,065
                    0  crypto_spec    0x77       e                  7/10             0

Performance counter stats for core 6, multiplexed, kernel mode excluded, on Arm Limited core implementation:
note: 'e' - normal event, 'gN' - grouped event with group number N, metric name will be appended if 'e' or 'g' comes from it

        counter value  event name     event idx  event note  multiplexed  scaled value
        =============  ==========     =========  ==========  ===========  ============
                    0  cycle          fixed      e                 10/10             0
                    0  inst_spec      0x1b       e                  8/10             0
                    0  vfp_spec       0x75       e                  8/10             0
                    0  ase_spec       0x74       e                  8/10             0
                    0  dp_spec        0x73       e                  8/10             0
                    0  ld_spec        0x70       e                  7/10             0
                    0  st_spec        0x71       e                  7/10             0
                    0  br_immed_spec  0x78       e                  7/10             0
                    0  crypto_spec    0x77       e                  7/10             0

Performance counter stats for core 7, multiplexed, kernel mode excluded, on Arm Limited core implementation:
note: 'e' - normal event, 'gN' - grouped event with group number N, metric name will be appended if 'e' or 'g' comes from it

        counter value  event name     event idx  event note  multiplexed  scaled value
        =============  ==========     =========  ==========  ===========  ============
                    0  cycle          fixed      e                 10/10             0
                    0  inst_spec      0x1b       e                  8/10             0
                    0  vfp_spec       0x75       e                  8/10             0
                    0  ase_spec       0x74       e                  8/10             0
                    0  dp_spec        0x73       e                  8/10             0
                    0  ld_spec        0x70       e                  7/10             0
                    0  st_spec        0x71       e                  7/10             0
                    0  br_immed_spec  0x78       e                  7/10             0
                    0  crypto_spec    0x77       e                  7/10             0

System-wide Overall:
        counter value  event name     event idx  event note  scaled value
        =============  ==========     =========  ==========  ============
           19,665,149  cycle          fixed      e             19,665,149
           26,346,285  inst_spec      0x001b     e             32,932,855
               85,414  vfp_spec       0x0075     e                106,767
            1,359,691  ase_spec       0x0074     e              1,699,613
           12,765,743  dp_spec        0x0073     e             15,957,178
            4,410,072  ld_spec        0x0070     e              6,300,102
            3,178,910  st_spec        0x0071     e              4,541,299
            1,020,154  br_immed_spec  0x0078     e              1,457,362
                    0  crypto_spec    0x0077     e                      0

                1.09 seconds time elapsed
```

### Specify a range of cores

Ranges are specified as `[core_number]-[core_number]`, and can be specified alongside singular cores. For example,

```
> wperf stat -m imix -c 0,3-5 --timeout 1 --json
counting ... done

Performance counter stats for core 0, no multiplexing, kernel mode excluded, on Arm Limited core implementation:
note: 'e' - normal event, 'gN' - grouped event with group number N, metric name will be appended if 'e' or 'g' comes from it

        counter value  event name  event idx  event note
        =============  ==========  =========  ==========
          237,651,684  cycle       fixed      e
          166,415,456  inst_spec   0x1b       g0,imix
           74,655,597  dp_spec     0x73       g0,imix
              657,789  vfp_spec    0x75       g0,imix
              149,333  ase_spec    0x74       g0,imix
           38,158,577  ld_spec     0x70       g0,imix

Performance counter stats for core 3, no multiplexing, kernel mode excluded, on Arm Limited core implementation:
note: 'e' - normal event, 'gN' - grouped event with group number N, metric name will be appended if 'e' or 'g' comes from it

        counter value  event name  event idx  event note
        =============  ==========  =========  ==========
           81,321,835  cycle       fixed      e
           83,260,473  inst_spec   0x1b       g0,imix
           38,963,288  dp_spec     0x73       g0,imix
              106,695  vfp_spec    0x75       g0,imix
              114,980  ase_spec    0x74       g0,imix
           18,559,855  ld_spec     0x70       g0,imix

Performance counter stats for core 4, no multiplexing, kernel mode excluded, on Arm Limited core implementation:
note: 'e' - normal event, 'gN' - grouped event with group number N, metric name will be appended if 'e' or 'g' comes from it

        counter value  event name  event idx  event note
        =============  ==========  =========  ==========
            1,181,079  cycle       fixed      e
              355,685  inst_spec   0x1b       g0,imix
              146,025  dp_spec     0x73       g0,imix
                  702  vfp_spec    0x75       g0,imix
                1,637  ase_spec    0x74       g0,imix
               85,716  ld_spec     0x70       g0,imix

Performance counter stats for core 5, no multiplexing, kernel mode excluded, on Arm Limited core implementation:
note: 'e' - normal event, 'gN' - grouped event with group number N, metric name will be appended if 'e' or 'g' comes from it

        counter value  event name  event idx  event note
        =============  ==========  =========  ==========
            1,595,874  cycle       fixed      e
            1,560,082  inst_spec   0x1b       g0,imix
              725,657  dp_spec     0x73       g0,imix
                7,114  vfp_spec    0x75       g0,imix
                  386  ase_spec    0x74       g0,imix
              383,499  ld_spec     0x70       g0,imix

System-wide Overall:
        counter value  event name  event idx  event note
        =============  ==========  =========  ==========
          321,750,472  cycle       fixed      e
          251,591,696  inst_spec   0x001b     g0,imix
          114,490,567  dp_spec     0x0073     g0,imix
              772,300  vfp_spec    0x0075     g0,imix
              266,336  ase_spec    0x0074     g0,imix
           57,187,647  ld_spec     0x0070     g0,imix

               1.087 seconds time elapsed
```

### Using alias `--cpu`

The alias for `-c`, `--cpu` can be used in the same way. For example,

```
> wperf stat -e inst_spec --cpu 0 sleep 1
counting ... done

Performance counter stats for core 0, no multiplexing, kernel mode excluded, on Arm Limited core implementation:
note: 'e' - normal event, 'gN' - grouped event with group number N, metric name will be appended if 'e' or 'g' comes from it

        counter value  event name  event idx  event note
        =============  ==========  =========  ==========
        1,226,336,052  cycle       fixed      e
        1,896,822,215  inst_spec   0x1b       e

               1.119 seconds time elapsed
```

counts on core 0.

## Timeline (count multiple times between intervals)

Timeline feature allow users to perform continuous counting:
- defined with `--timeout <DURATION>` command line option
- between intervals, defined with `-i <DURATION>`
- for `N` times, defined with `-n <N>`.

For example command:

```
>wperf stat -m imix -c 1 -t -i 2 -n 3 --timeout 5
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
3) Repeat the above count and sleep 3 times (`-n 3`).

Note: use `-v` (verbose) command line option together with timeline to get access to CSV output filename:

```
>wperf stat -m imix -c 1 -t -i 2 -n 3 --timeout 5  -v
timeline file: 'wperf_core_1_2023_09_13_13_31_59.core.csv'
events to be counted:
     5              core events: 0x001b 0x0073 0x0075 0x0074 0x0070
...
```

Hint:
- use `-m <metric>` to capture metric events, and/or `-e <events>` to count additional events.
- use `--json` to additionally return timeline output data in the JSON format, add `--output <FILENAME>` to capture JSON output to a given file.
  - use `--output-csv <CSV_FILENAME>` with `--json` and `--output <JSON_FILENAME>` to capture JSON timeline output to `<JSON_FILENAME>` and CSV timeline output to `<CSV_FILENAME>` file.
- use `--cwd <PATH>` command line to specify the directory in which WindowsPerf will store JSON and CSV files.

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

Timeline command (`-t`) produces [CSV file](https://en.wikipedia.org/wiki/Comma-separated_values). Its format uses comma separated values to distinguish between columns. CSV filename contains core number, current timestamp, name of event counted.

Timeline stores results in a form of a CSV file. Below is an output from the above timeline example. Please note that we

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

#### Specify timeline CSV output filename with --output-csv command line option

Support for `--output-csv` command line in timeline (`-t`) is as follows:

Previously users had to specify `-v` (verbose mode on) with `-t` (timeline command line option) to retrieve from the console name of the timeline CSV file. Now users can also specify timeline output filename with `--output-csv <FILENAME>` command line option, where `<FILENAME>` is template string for timeline CSV file.

User can specify in `<FILENAME>` few placeholders which can improve timeline filename:
* `{timestamp}` to add the current timestamp to the output filename. E.g. `2023_09_21_09_42_59` for 21st of September 2023, time: 09:42:59.
* `{class}` to add event class name (e.g. `core`, `dsu`, `dmc_clk`, `dmc_clkdiv2`). Multiple timeline files will be created if the user specifies `-e` events with different classes.
* `{core}` to add `<N>` from `-c <N>` command line option. Note: when more than one core is specified `{core}` will be replaced with the first core specified.

Examples:

```
>wperf stat -e l1d_cache_rd -t -i 0 --timeout 1 -n 3 -c 1,2,3 -v --output-csv timeline_{core}_{timestamp}_{class}.csv
timeline file: 'timeline_1_2023_09_21_12_21_46_core.csv'
```

```
>wperf stat -e l1d_cache_rd -t -i 0 --timeout 1 -n 3 -c 7 -v --output-csv timeline--{core}--{class}.csv
timeline file: 'timeline--7--core.csv'
```

```
>wperf stat -e l1d_cache_rd -t -i 0 --timeout 1 -n 3 -c 7 -v -o {timestamp}.{core}.{class}.csv
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

In case of targets supporting Telemetry Solution metrics users can specify those with `-m` command line option. Because TS metrics contain formulas, `wperf` can calculate those based on event occurrences and present metric value in last columns. Metrics are available in CSV file and marked with leading `M@`, e.g. `M@l1d_cache_miss_ratio` or `M@l1d_tlb_mpki` in order to distinguish metric name from event name.

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
You can also emit JSON to file directly with `--output <FILENAME>`. Optionally add `--cwd` command line option to specify the directory where `<FILENAME>` will be created.

Currently we support `--json` with `stat`, `sample`, `record`, `list`, `man` and `test` commands.

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

### Store counting results in the JSON file

Print on standard output results of counting and at the same time store these results in the JSON file `count.json`.
```
> wperf stat -e inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec -c 0 --output count.json sleep 1
```

### Only store counting results in the JSON file

Store counting results in the JSON file `count.json` and does not print anything on the screen. Printouts are suppressed with the `-q` command line flag.
```
> wperf stat -e inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec -c 0 --output count.json -q sleep 1
```

# Sampling model

## CPython sampling example

In this example we will build CPython from sources and execute simple instructions in Python interactive mode to obtain sampling information from CPython runtime image.
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
           41,806,794  cycle       fixed      e
           46,965,926  inst_spec   0x1b       g0,imix
           21,251,194  dp_spec     0x73       g0,imix
               76,479  vfp_spec    0x75       g0,imix
            6,078,472  ase_spec    0x74       g0,imix
            7,450,823  ld_spec     0x70       g0,imix
            4,309,117  st_spec     0x71       g0,imix

               3.302 seconds time elapsed
```

#### Sampling for `ld_spec` event which, by looking at counting is dominant (at least for `imix` metrics)

Let's sample the `ld_spec` event. Please note that you can specify the process image name and PDB filename with `--pdb_file python_d.pdb` and `--image_name python_d.exe`. In our case `wperf` is able to deduce image name (same as PE filename) and PDB file from PR filename.

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

Note that in `sampling ....e.e.e.e.e.` progressing printout `.` represents sample payload (of 128 samples)  received from the driver. 'e' represents an unsuccessful attempt to fetch a whole sample payload. `wperf` is polling `wperf-driver` awaiting sample payload.

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

A double-dash (`--`) is a syntax used in shell commands to signify end of command options and beginning of positional arguments. In other words, it separates `wperf` CLI options from arguments that the command operates on. Use `--` to separate `wperf.exe` command line options from the process you want to spawn followed by its verbatim arguments.

```
>wperf [OPTIONS] -- PROCESS_NAME [ARGS]
```

## Using the `annotate` option

A normal output of the following command

```
>wperf record -c 0 -e vfp_spec:1000 --timeout 5 -- .\WindowsPerfSample1.exe
```

could be

```
base address of '.\WindowsPerfSample1.exe': 0x7ff7526b15c4, runtime delta: 0x7ff6126a0000
sampling ....eeee done!
======================== sample source: vfp_spec, top 50 hot functions ========================
        overhead  count  symbol
        ========  =====  ======
           91.41    117  simd_hot
            7.81     10  df_hot
            0.78      1  __CheckForDebuggerJustMyCode
100.00%       128  top 3 in total
```

If you want to have more information about the exact place in the source code where those samples were acquired you can use the `--annotate` option, like so

```
>wperf record -c 0 -e vfp_spec:1000 --timeout 5 --annotate -- .\WindowsPerfSample1.exe
```

resulting in

```
base address of '.\WindowsPerfSample1.exe': 0x7ff7526b15c4, runtime delta: 0x7ff6126a0000
sampling ....eeee done!
======================== sample source: vfp_spec, top 50 hot functions ========================

simd_hot
        line_number  hits  filename
        ===========  ====  ========
        53           49    C:\Users\evert\source\repos\WindowsPerfSample\lib.c
        52           8     C:\Users\evert\source\repos\WindowsPerfSample\lib.c

df_hot
        line_number  hits  filename
        ===========  ====  ========
        33           1     C:\Users\evert\source\repos\WindowsPerfSample\lib.c
        15,732,480   1     C:\Users\evert\source\repos\WindowsPerfSample\lib.c

        overhead  count  symbol
        ========  =====  ======
           53.91     69  unknown
           44.53     57  simd_hot
            1.56      2  df_hot
100.00%       128  top 3 in total
```

You will now see the list of top functions followed by a table with line numbers, hits and filename.
The filename and line number shows information extracted from the PDB files matching the sample address to a particular position on the source code. The hits column shows the number of samples for that filename/line number pair. Notice that due to address skid this can be a bit off.

### Using the `disassemble` option

In case you need even more information than the one given by `--annotate` you can use the `--disassemble` option to give the particular surroundings of the instruction that generated the sample. Notice that `--disassemble` implies `--annotate`. Use the following command

```
>wperf record -c 0 -e vfp_spec:1000 --timeout 5 --disassemble -- .\WindowsPerfSample1.exe
```

to get

```
base address of '.\WindowsPerfSample1.exe': 0x7ff7526b15c4, runtime delta: 0x7ff6126a0000
sampling ....eeee done!
======================== sample source: vfp_spec, top 50 hot functions ========================
simd_hot
        line_number  hits  filename                 instruction_address    disassembled_line
        ===========  ====  ========                 ===================    =================
        53           96    WindowsPerfSample\lib.c  13a30                  address  instruction
                                                                           =======  ===========
                                                                           13a30    ldr     q18, [x19], #0x40
                                                                           13a34    add     v16.4s, v16.4s, v18.4s
                                                                           13a38    stur    q16, [x9, #-0x20]
                                                                           13a3c    ldr     q16, [x13, x8]
                                                                           13a40    add     v16.4s, v16.4s, v17.4s
                                                                           13a44    ldr     q17, [x11, x9]
                                                                           13a48    str     q16, [x12, x8]
                                                                           13a4c    ldp     q16, q18, [x8, #0x10]
                                                                           13a50    add     x8, x8, #0x40
                                                                           13a54    add     v17.4s, v17.4s, v16.4s
                                                                           13a58    ldur    q16, [x19, #-0x10]
                                                                           13a5c    add     v16.4s, v18.4s, v16.4s
                                                                           13a60    stp     q17, q16, [x9], #0x40
                                                                           13a64    cbnz    w10, 0x140013a28 <.text+0x2a28>
                                                                           13a68    ldp     x29, x30, [sp], #0x10
                                                                           13a6c    ldr     x21, [sp, #0x10]
                                                                           13a70    ldp     x19, x20, [sp], #0x20
                                                                           13a74    ret
        52           17    WindowsPerfSample\lib.c  13a2c                  address  instruction
                                                                           =======  ===========
                                                                           13a28    ldp     q16, q17, [x8, #-0x10]
                                                                           13a2c    sub     w10, w10, #0x1


df_hot
        line_number  hits  filename                 instruction_address    disassembled_line
        ===========  ====  ========                 ===================    =================
        15,732,480   5     WindowsPerfSample\lib.c  137bc                  address  instruction
                                                                           =======  ===========
                                                                           137a8    adrp    x8, 0x140023000
                                                                           137ac    add     x0, x8, #0x0
                                                                           137b0    bl      0x1400117e0 <.text+0x7e0>
                                                                           137b4    adrp    x9, 0x14001f000
                                                                           137b8    ldr     d16, [x9]
                                                                           137bc    ldr     d18, 0x140013828 <.text+0x2828>
                                                                           137c0    ldr     d17, 0x140013830 <.text+0x2830>
                                                                           137c4    fdiv    d16, d16, d18
                                                                           137c8    fadd    d19, d16, d17
                                                                           137cc    ldr     d16, 0x140013838 <.text+0x2838>
                                                                           137d0    fmul    d0, d19, d16
                                                                           137d4    str     d0, [x9]
                                                                           137d8    cbnz    w19, 0x14001381c <.text+0x281c>
        33           1     WindowsPerfSample\lib.c  1379c                  address  instruction
                                                                           =======  ===========
                                                                           13798    str     x19, [sp, #-0x10]!
                                                                           1379c    stp     x29, x30, [sp, #-0x10]!
                                                                           137a0    mov     x29, sp
                                                                           137a4    mov     w19, w0

__CheckForDebuggerJustMyCode
        line_number  hits  filename                                                          instruction_address  disassembled_line
        ===========  ====  ========                                                          ===================  =================
        25           1     D:\a\_work\1\s\src\vctools\crt\vcstartup\src\misc\debugger_jmc.c  13fe0                  address  instruction
                                                                                                                    =======  ===========
                                                                                                                    13fd8    ldr        x8, [sp, #0x10]
                                                                                                                    13fdc    ldrb       w8, [x8]
                                                                                                                    13fe0    mov        w8, w8
                                                                                                                    13fe4    cmp        w8, #0x0
                                                                                                                    13fe8    b.eq       0x140014020 <.text+0x3020>
                                                                                                                    13fec    adrp       x8, 0x14001f000
                                                                                                                    13ff0    ldr        w8, [x8, #0x754]
                                                                                                                    13ff4    cmp        w8, #0x0
                                                                                                                    13ff8    b.eq       0x140014020 <.text+0x3020>
                                                                                                                    13ffc    adrp       x8, 0x140022000
                                                                                                                    14000    ldr        x8, [x8, #0x70]
                                                                                                                    14004    blr        x8
                                                                                                                    14008    mov        w9, w0
                                                                                                                    1400c    adrp       x8, 0x14001f000
                                                                                                                    14010    ldr        w8, [x8, #0x754]
                                                                                                                    14014    cmp        w8, w9
                                                                                                                    14018    b.ne       0x140014020 <.text+0x3020>

        overhead  count  symbol
        ========  =====  ======
           88.28    113  simd_hot
            6.25      8  unknown
            4.69      6  df_hot
            0.78      1  __CheckForDebuggerJustMyCode
100.00%       128  top 4 in total
```

The columns are pretty similar to what you would get from `--annotate` except that now you have an entry for each instruction address along with the pair filename/line number's disassembled code. Notice that
WindowsPerf uses LLVM's [objdump](https://llvm.org/docs/CommandGuide/llvm-objdump.html) and it needs to be available on PATH or else you will get the following message

```
Error executing disassembler `llvm-objdump`. Is it on PATH?
note: wperf uses LLVM's objdump. You can install Visual Studio 'C++ Clang Compiler...' and 'MSBuild support for LLVM'
Failed to call disassembler!
```

You can either:
- Download LLVM from its [releases page](https://releases.llvm.org/) or
- Follow the instructions [here](https://gitlab.com/Linaro/WindowsPerf/windowsperf/-/blob/main/wperf-scripts/tests/README.md?ref_type=heads#build-dependencies) to get it installed within Visual Studio.
  - Shortcut: You need clang targeting `aarch64-pc-windows-msvc`:
    - Go to Visual Studio Installer and install: Modify -> Individual Components -> search "clang".
    - Install: "C++ Clang Compiler..." and "MSBuild support for LLVM..."

## Sampling with Arm Statistical Profiling Extension (SPE)

WindowsPerf added support (in `record` command) for the Arm Statistical Profiling Extension (SPE). SPE is an optional feature in ARMv8.2 hardware that allows CPU instructions to be sampled and associated with the source code location where that instruction occurred.

> :warning: Currently SPE is available on Windows On Arm Test Mode only!

You can use the same `--annotate` and `--disassemble` interface of WindowsPerf with one exception. To reach out to SPE resources please use `-e` command with `arm_spe_0//` options. For example:

```
>wperf record -e arm_spe_0// -c 0 --timeout 10 -- cpython\PCbuild\arm64\python_d.exe -c 10**10**100
```

### SPE detection

> :warning: Currently WindowsPerf support of SPE is in development, you can enable beta code of SPE support with `ENABLE_SPE` macro or just rebuild project with `Debug+SPE` configuration.

#### SPE hardware support detection

You can check if your system supports SPE or if WindowsPerf can detect SPE with `wperf test` command. See below an example of `spe_device.version_name` property value on system with SPE:

```
>wperf test
        Test Name                                           Result
        =========                                           ======
...
        spe_device.version_name                             FEAT_SPE
```

#### How do I know if you WindowsPerf binaries and driver support optional SPE?

You can check feature string (`FeatureString`) of both `wperf` and `wperf-driver` with `wperf --version` command:

```
>wperf --version
        Component     Version  GitVer          FeatureString
        =========     =======  ======          =============
        wperf         3.7.2    4338371a        +etw-app+spe
        wperf-driver  3.7.2    4338371a        +trace+spe
```

If `FeatureString` for both components (`wperf` and `wperf-driver`) contains `+spe` (and `spe_device.version_name` contains `FEAT_SPE`) you are good to go!

### arm_spe_0// format

Users can specify SPE filters with `arm_spe_0//`. We added CLI parser function for `-e arm_spe_0/*/` notation for `record` command. Where `*` is a comma separated list of supported filters. Currently we support filters. Users can define filters such as `store_filter=`, `load_filter=`, `branch_filter=` or short equivalents like `st=`, `ld=` and `b=`. Use `0` or `1` to disabled or enable a given filter. For example:

```
>wperf record -c 0 -e arm_spe_0/branch_filter=1/
>wperf record -c 0 -e arm_spe_0/load_filter=1,branch_filter=0/
>wperf record -c 0 -e arm_spe_0/st=0,ld=0,b=1/
```

#### Filtering sample records

SPE register `PMSFCR_EL1.FT` enables filtering by operation type. When enabled `PMSFCR_EL1.{ST, LD, B}` define the collected types:
- `ST` enables collection of store sampled operations, including all atomic operations.
- `LD` enables collection of load sampled operations, including atomic operations that return a value to a register.
- `B` enables collection of branch sampled operations, including direct and indirect branches and exception returns.

### Sampling with SPE CPython example

### SPE with annotate

Annotate example with `ld=1` filter enabled: enables collection of load sampled operations, including atomic operations that return a value to a register.

```
>wperf record -e arm_spe_0/ld=1/ -c 8 -- cpython\PCbuild\arm64\python_d.exe -c 10**10**100
base address of 'cpython\PCbuild\arm64\python_d.exe': 0x7ff69e251288, runtime delta: 0x7ff55e250000
sampling ...e..........e... done!
======================== sample source: LOAD_STORE_ATOMIC-LOAD-GP/retired+level1-data-cache-access+tlb_access, top 50 hot functions ========================
        overhead  count  symbol
        ========  =====  ======
           93.75     15  x_mul:python312_d.dll
            6.25      1  _Py_ThreadCanHandleSignals:python312_d.dll
100.00%        16  top 2 in total
```

#### SPE with disassemble

Disassemble example with `ld=1` filter enabled: enables collection of load sampled operations, including atomic operations that return a value to a register.

```
>wperf record -e arm_spe_0/ld=1/ -c 8 --disassemble -- cpython\PCbuild\arm64\python_d.exe -c 10**10**100
base address of 'cpython\PCbuild\arm64\python_d.exe': 0x7ff69e251288, runtime delta: 0x7ff55e250000
sampling ...ee..eee..e. done!
======================== sample source: LOAD_STORE_ATOMIC-LOAD-GP/retired+level1-data-cache-access+tlb_access, top 50 hot functions ========================
x_mul:python312_d.dll
        line_number  hits  filename                      instruction_address    disassembled_line
        ===========  ====  ========                      ===================    =================
        3,590        1     cpython\Objects\longobject.c  404384                 address  instruction
                                                                                =======  ===========
                                                                                40436c   ldr   x8, [sp, #0x20]
                                                                                404370   ldr   w8, [x8]
                                                                                404374   ubfx  x8, x8, #0, #32
                                                                                404378   ldr   x9, [sp, #0x58]
                                                                                40437c   ldr   w9, [x9]
                                                                                404380   ubfx  x9, x9, #0, #32
                                                                                404384   ldr   x10, [sp, #0x50]
                                                                                404388   mul   x9, x9, x10
                                                                                40438c   add   x9, x8, x9
                                                                                404390   ldr   x8, [sp, #0x10]
                                                                                404394   add   x8, x8, x9
                                                                                404398   str   x8, [sp, #0x10]
                                                                                40439c   ldr   x8, [sp, #0x58]
                                                                                4043a0   add   x8, x8, #0x4
                                                                                4043a4   str   x8, [sp, #0x58]
        3,591        1     cpython\Objects\longobject.c  4043bc                 address  instruction
                                                                                =======  ===========
                                                                                4043a8   ldr   x8, [sp, #0x10]
                                                                                4043ac   and   x8, x8, #0x3fffffff
                                                                                4043b0   mov   w8, w8
                                                                                4043b4   ldr   x9, [sp, #0x20]
                                                                                4043b8   str   w8, [x9]
                                                                                4043bc   ldr   x8, [sp, #0x20]
                                                                                4043c0   add   x8, x8, #0x4
                                                                                4043c4   str   x8, [sp, #0x20]
        3,592        1     cpython\Objects\longobject.c  4043c8                 address  instruction
                                                                                =======  ===========
                                                                                4043c8   ldr   x8, [sp, #0x10]
                                                                                4043cc   lsr   x8, x8, #30
                                                                                4043d0   str   x8, [sp, #0x10]
        3,595        1     cpython\Objects\longobject.c  40440c                 address  instruction
                                                                                =======  ===========
                                                                                40440c   ldr   x8, [sp, #0x10]
                                                                                404410   cmp   x8, #0x0
                                                                                404414   b.eq  0x180404524 <_PyCrossInterpreterData_UnregisterClass+0x3fc798>

x_add:python312_d.dll
        line_number  hits  filename                      instruction_address    disassembled_line
        ===========  ====  ========                      ===================    =================
        3,401        1     cpython\Objects\longobject.c  402fb8                 address  instruction
                                                                                =======  ===========
                                                                                402fb0   ldr   x8, [sp, #0x20]
                                                                                402fb4   add   x8, x8, #0x18
                                                                                402fb8   ldr   x9, [sp, #0x18]
                                                                                402fbc   mov   x10, #0x4               // =4
                                                                                402fc0   mul   x9, x9, x10
                                                                                402fc4   add   x8, x8, x9
                                                                                402fc8   ldr   x9, [sp, #0x28]
                                                                                402fcc   add   x9, x9, #0x18
                                                                                402fd0   ldr   x10, [sp, #0x18]
                                                                                402fd4   mov   x11, #0x4               // =4
                                                                                402fd8   mul   x10, x10, x11
                                                                                402fdc   add   x9, x9, x10
                                                                                402fe0   ldr   w8, [x8]
                                                                                402fe4   ldr   w9, [x9]
                                                                                402fe8   add   w9, w8, w9
                                                                                402fec   ldr   w8, [sp, #0x10]
                                                                                402ff0   add   w8, w8, w9
                                                                                402ff4   str   w8, [sp, #0x10]

        overhead  count  symbol
        ========  =====  ======
           80.00      4  x_mul:python312_d.dll
           20.00      1  x_add:python312_d.dll
100.00%         5  top 2 in total
```
