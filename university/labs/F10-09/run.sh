#!/usr/bin/env bash
# F10-09 extra step (called by run_lab.sh after the .cpp listings): cross-checks
# vib_report.cpp with numpy on the CSV files written by flight_log.cpp.
set -u
cd "$(dirname "$0")" || exit 2
name=spectrum_check
{
    echo "listing:   $name.py (reads flightA.csv and flightB.csv written by flight_log.cpp)"
    echo "toolchain: $(python3 --version 2>&1), numpy $(python3 -I -c 'import numpy; print(numpy.__version__)')"
    echo "command:   python3 -I $name.py flightA.csv flightB.csv"
    echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
} > "$name.log"
timeout 60 python3 -I "$name.py" flightA.csv flightB.csv > "$name.out" 2>&1
rc=$?
echo "exit code: $rc" >> "$name.log"
exit $rc
