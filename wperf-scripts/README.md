# Scripts

* [Scripts](#scripts)
* [arch_events_update.py](#arch_events_updatepy)

# arch_events_update.py

`arch_events_update.py` script can fetch PMU events stored in [ARM-software/data/pmu](https://github.com/ARM-software/data/blob/master/pmu/) and output in format compatible with [armv8-arch-events.def](https://gitlab.com/Linaro/WindowsPerf/windowsperf/-/blob/main/wperf-common/armv8-arch-events.def).

```
> python3 arch_events_update.py
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

