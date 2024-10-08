# WindowsPerf Scripts

The `wperf-scripts` module in WindowsPerf includes both test scripts, which are used to validate the tool, and helper scripts, which assist in various tasks and streamline processes.

# How to install all dependencies for Python scripts in this directory

Use the command below to install the packages according to the configuration file `requirements.txt`.

```
> pip install -r requirements.txt
```

# Script library

## Tests

`wperf` regression tests written with Python (and `pytest`). See [README.md](tests/README.md) for more details.

## wperf-devcon PS1 script

This script is wrapper for `devcon` command line tool. See [README.md](devcon/README.md) for more details.

## Script arch_events_update.py

`arch_events_update.py` script can fetch PMU events stored in [ARM-software/data/pmu](https://github.com/ARM-software/data/blob/master/pmu/) and output in format compatible with [armv8-arch-events.def](https://github.com/arm-developer-tools/windowsperf/blob/main/wperf-common/armv8-arch-events.def).

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

## Script telemetry_events_update.py

Script fetches Telemetry Solution CPU's PMU related information from [Telemetry Solution](https://gitlab.arm.com/telemetry-solution/telemetry-solution/-/tree/main/data/pmu/cpu).

Note: for simplicity we now hard-code CPUs available in above repository. In the future we will add enumeration functionality!

### Usage

Print to terminal:

```
> python telemetry_events_update.py 
```

To manually pipe into a destination file:

```
> python telemetry_events_update.py > ..\wperf-common\telemetry-solution-data.def
```

`-o` flag, path of desired output file (file will be made or overwritten). If not specified, outputs to stdout. `--output` is an alias.
```
> python telemetry_events_update.py -o ..\wperf-common\telemetry-solution-data.def
```

`--url` and `--file` flags, controls input type which can either be: `url` or `file`. If neither are specified, the default input is 'url'. URL path and local file path are both hardcoded within the script itself.

```
> python telemetry_events_update.py --url
> python telemetry_events_update.py --file
> python telemetry_events_update.py --file --output ./test.def
```
