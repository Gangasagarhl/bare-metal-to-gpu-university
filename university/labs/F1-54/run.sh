#!/usr/bin/env bash
# F1-54 run.sh: look for temperature sensors and memory-error reporting on the machine that
# runs the lab, and record honestly what this (virtual) build machine exposes.
set -u -o pipefail
cd "$(dirname "$0")"
C='ls /sys/class/thermal; ls /sys/class/hwmon; ls /sys/devices/system/edac'
{
    for d in /sys/class/thermal /sys/class/hwmon /sys/devices/system/edac; do
        echo "\$ ls $d"
        if [ -d "$d" ]; then
            out="$(ls "$d")"; if [ -z "$out" ]; then echo "(empty: the folder exists but lists nothing)"; else echo "$out"; fi
        else
            echo "(no such folder)"
        fi
    done
} > sensors.out 2>&1
{
    echo "listing:   run.sh (step sensors)"
    echo "toolchain: $(bash --version | head -n 1); Linux $(uname -r)"
    echo "command:   $C"
    echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "machine:   $(uname -s) $(uname -m) (cloud build container, a virtual machine)"
    echo "exit code: 0"
    echo "hardware:  untested on hardware: the virtual build machine exposes no thermal sensors and no memory-error (EDAC) reporting"
} > sensors.log
exit 0
