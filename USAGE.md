# WindowsPerf Command Line Example Usage

[[_TOC_]]

## WindowsPerf CLI Cheatsheet

| `wperf` Command     | Description |
| ---                 | ---         |
| `wperf -–help`      | Run wperf help command, see detailed command line syntax. |
| `wperf --version`   | Display version of the driver and CLI application. |
| `wperf list`        | List supported events and metrics. Enable verbose mode for more details. |
| `wperf test`        | Configuration information about driver and application. |
| `wperf man`         | Plain text information about one or more specified event(s), metric(s), and or group metric(s). |
| `wperf stat`        | Counting mode, for obtaining aggregate counts of occurrences of special events. |
| `wperf sample`      | Sampling mode, for determining the frequencies of event occurrences produced by program locations at the function, basic block, and/or instruction levels.  |
| `wperf record`      | Same as sample but also automatically spawns the process and pins it to the core specified by `-c`. Process name is defined by COMMAND. User can pass verbatim arguments to the process with ARGS. |

## WindowsPerf CLI Example Commands

- List all events and metrics available on your host with extended information:
```
> wperf list -v
```

- Get information about WindowsPerf configuration:

```
> wperf test
```

- List PMU events, metrics and groups of metrics detailed information:
```
> wperf man ld_spec
```

### CPU PMU Counting

- Count events `inst_spec`, `vfp_spec`, `ase_spec` and `ld_spec` on core no. 0 for 3 seconds:
```
> wperf stat -e inst_spec,vfp_spec,ase_spec,ld_spec -c 0 --timeout 3
```

- Count metric `imix` (metric events will be grouped) and additional event `l1i_cache` on core no. 7 for 10.5 seconds:
```
> wperf stat -m imix -e l1i_cache -c 7 --timeout 10.5
```

- Count in timeline mode (output counting to CSV file) metric `imix` 3 times on core no. 1 with 2 second intervals (delays between counts). Each count will last 5 seconds:
```
> wperf stat -m imix -c 1 -t -i 2 -n 3 --timeout 5
```

### CPU PMU Sampling

- Launch `python_d.exe -c 10**10**100` process and start software sampling event `ld_spec` with frequency `100000` on core no. 1 for 30 seconds.
Hint: add `--annotate` or `--disassemble` to `wperf record` command line parameters to increase sampling "resolution":
```
> wperf record -e ld_spec:100000 -c 1 --timeout 30 -- python_d.exe -c 10**10**100
```

### Arm SPE Sampling

- Launch `python_d.exe -c 10**10**100` process and start hardware sampling with Arm SPE (filter samples with `load_filter` filter) on core no. 7 for 10 seconds.

Hint: add `--annotate` or `--disassemble` to `wperf record` command line parameters to increase sampling "resolution":
```
> wperf record -c 7 -e arm_spe_0/load_filter=1/ --timeout 10 -- python_d.exe -c 10**10**100
```
