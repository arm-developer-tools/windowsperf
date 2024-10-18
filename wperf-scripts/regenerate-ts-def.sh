#!/usr/bin/env bash

#
# Script ./regenerate-ts-def.sh
#
# This script simply calls `telemetry_events_update.py` for each Arm Telemetry CPU and generates separate .def file for each CPU.
#

for cpu in `python telemetry_events_update.py --print-family`;
do
    cat ../LICENSE > ../wperf-common/${cpu}.def
    echo "" >> ../wperf-common/${cpu}.def
    python telemetry_events_update.py --family ${cpu}.json >> ../wperf-common/${cpu}.def
done
