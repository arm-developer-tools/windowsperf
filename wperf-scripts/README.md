# Scripts

[[_TOC_]]

# How to install all dependencies for Python scripts in this directory

Use the command below to install the packages according to the configuration file `requirements.txt`.

```
>pip install -r requirements.txt
```

# Script library

## Tests

`wperf` regression tests written with Python (and `pytest). See [README.md](tests/README.md) for more details.

## wperf-devcon PS1 script

This script is wrapper for `devcon` command line tool. See [README.md](devcon/README.md) for more details.

## arch_events_update.py

`arch_events_update.py` script can fetch PMU events stored in [ARM-software/data/pmu](https://github.com/ARM-software/data/blob/master/pmu/) and output in format compatible with [armv8-arch-events.def](https://gitlab.com/Linaro/WindowsPerf/windowsperf/-/blob/main/wperf-common/armv8-arch-events.def).

```
>python3 arch_events_update.py
usage: arch_events_update.py [-h] [-l] [-c CPU] [-o OUTPUT] [--license LICENSE]

update the cpu's pmu events file!

options:
  -h, --help            show this help message and exit
  -l, --list_cpu        list all cpus in https://github.com/ARM-software/data/blob/master/pmu/
  -c CPU, --cpu CPU     cpu type that to update
  -o OUTPUT, --output OUTPUT
                        pmu events file for wperf
  --license LICENSE     license file added to the script header
```

## telemetry_events_update.py

Script fetches Telemetry Solution CPU's PMU related information from [Telemetry Solution](https://gitlab.arm.com/telemetry-solution/telemetry-solution/-/tree/main/data/pmu/cpu).

Note: for simplicity we now hard-code CPUs available in above repository. In the future we will add enumeration functionality!

```
>python telemetry_events_update.py > ..\wperf-common\telemetry-solution-data.def
```
