# Build wperf CLI

You can build `wperf` project from command line:

```
> devenv windowsperf.sln /Rebuild "Debug|ARM64" /Project wperf\wperf.vcxproj
```

# Usage of wperf
```
  > wperf [options]

Options:
  list                   List supported events and metrics
  stat                   Count events. If -e is not specified, then count default events
  -e e1,e2...            Specify events to count. Event eN could be a symbolic name or in raw number.
                         Symbolic name should be what's listed by 'perf list', raw number should be rXXXX,
                         XXXX is hex value of the number without '0x' prefix
  -m m1,m2...            Specify metrics to count. "imix", "icache", "dcache", "itlb", "dtlb" supported
  -d N                   Specify counting duration (in s). The accuracy is 0.1s
  sleep N                Like -d, for compatability with Linux perf
  -i N                   Specify counting interval (in s). To be used with -t
  -t                     Enable timeline mode. It specifies -i 60 -d 1 implicitly.
                         Means counting 1 second after every 60 second, and the result
                         is in .csv file in the same folder where wperf is invoked.
                         You can use -i and -d to change counting duration and interval.
  -C config_file         Provide customized config file which describes metrics etc.
  -c core_idx            Profile on the specified core
                         CORE_INDEX could be a number or 'ALL' (default if -c not specified)
  -k                     Count kernel model as well (disabled at default)
  -h                     Show tool help
  -l                     Alias of 'list'
  -verbose               Enable verbose output
  -v                     Alias of '-verbose'
  -version               Show tool version
```

# Examples

## List available PMU events
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
