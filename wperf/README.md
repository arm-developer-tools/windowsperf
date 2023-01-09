# wperf

* [Build wperf CLI](#build-wperf-cli)
* [Usage of wperf](#usage-of-wperf)
* [Examples](#examples)
  * [List available PMU events](#list-available-pmu-events)
  * [Counting core 0 (Ctrl-C to stop counting)](#counting-core-0-ctrl-c-to-stop-counting)
  * [Counting core 0 for 1 second](#counting-core-0-for-1-second)
  * [Specify up to 127 events, they will get multiplexed automatically, for example:](#specify-up-to-127-events-they-will-get-multiplexed-automatically-for-example)
  * [Count using event group](#count-using-event-group)
  * [Count using pre-defined metrics, metric could be used together with -e, no restriction](#count-using-pre-defined-metrics-metric-could-be-used-together-with--e-no-restriction)
  * [Count on multiple cores simultaneously with -c](#count-on-multiple-cores-simultaneously-with--c)
  * [JSON output](#json-output)
    * [Emit JSON output for simple counting with -json](#emit-json-output-for-simple-counting-with--json)
    * [Store counting results in JSON file](#store-counting-results-in-json-file)
    * [Only store counting results in JSON file](#only-store-counting-results-in-json-file)

# Build wperf CLI

You can build `wperf` project from command line:

```
> devenv windowsperf.sln /Rebuild "Debug|ARM64" /Project wperf\wperf.vcxproj
```

# Usage of wperf
```
> wperf -h

usage: wperf [options]

    Options:
    list              List supported events and metrics.
    stat              Count events.If - e is not specified, then count default events.
    -e e1, e2...      Specify events to count.Event eN could be a symbolic name or in raw number.
                      Symbolic name should be what's listed by 'perf list', raw number should be rXXXX,
                      XXXX is hex value of the number without '0x' prefix.
    -m m1, m2...      Specify metrics to count. \"imix\", \"icache\", \"dcache\", \"itlb\", \"dtlb\" supported.
    -d N              Specify counting duration(in s).The accuracy is 0.1s.
    sleep N           Like -d, for compatibility with Linux perf.
    -i N              Specify counting interval(in s).To be used with -t.
    -t                Enable timeline mode.It specifies -i 60 -d 1 implicitly.
                      Means counting 1 second after every 60 second, and the result
                      is in.csv file in the same folder where wperf is invoked.
                      You can use -i and -d to change counting duration and interval.
    -C config_file    Provide customized config file which describes metrics etc.
    -c core_idx       Profile on the specified core. Skip -c to count on all cores.
    -c cpu_list       Profile on the specified cores, 'cpu_list' is comma separated list e.g. '-c 0,1,2,3'.
    -dmc dmc_idx      Profile on the specified DDR controller. Skip -dmc to count on all DMCs.
    -k                Count kernel model as well (disabled by default).
    -h                Show tool help.
    --output          Enable JSON output to file.
    -q                Quiet mode, no output is produced.
    -json             Define output type as JSON.
    -l                Alias of 'list'.
    -verbose          Enable verbose output.
    -v                Alias of '-verbose'.
    -version          Show tool version.
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

## JSON output

You can output JSON instead of human readable tables with `wperf`. We've introduced three new command line flags which should help you emit JSON.
Flag `-json` will emit JSON for tables with values.
Quiet mode can be selected with `-q`. This will suppress human readable printouts. Please note that `-json` implies `-q`.
You can also emit JSON to file directly with `--output <filename>`.

Currently we support `-json` with `stat`, `list` and `test` commands.

### Emit JSON output for simple counting with -json

```
> wperf stat -e inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec -c 0 -json sleep 1
```

Will print on standard output:

```json
{
"core": {
"0":{"Performance_counter":[{"counter_value":"6062425","event_idx":"fixed","event_name":"cycle","event_note":"e"},{"counter_value":"6864612","event_idx":"0x1b","event_name":"inst_spec","event_note":"e"},{"counter_value":"10884","event_idx":"0x75","event_name":"vfp_spec","event_note":"e"},{"counter_value":"986671","event_idx":"0x74","event_name":"ase_spec","event_note":"e"},{"counter_value":"3081820","event_idx":"0x73","event_name":"dp_spec","event_note":"e"},{"counter_value":"1099973","event_idx":"0x70","event_name":"ld_spec","event_note":"e"},{"counter_value":"603607","event_idx":"0x71","event_name":"st_spec","event_note":"e"}]}
,
"overall": {}
}
,
"dsu": {
"l3metric": {},
"overall": {}
}
,
"dmc": {
"pmu": {},
"ddr": {}
}
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