#!/usr/bin/env bash
# F1-65 extra step (called by run_lab.sh after the .cpp listings):
# cross-checks the C++ DFT (spectrum.cpp) with numpy's FFT on the same CSV file.
# Writes spectrum_check.out and spectrum_check.log in the run_lab.sh format.
set -u
cd "$(dirname "$0")" || exit 2
name=spectrum_check
{
    echo "listing:   $name.py (reads jumpy_imu.csv written by jumpy_log.cpp)"
    echo "toolchain: $(python3 --version 2>&1)"
    echo "command:   python3 -I $name.py jumpy_imu.csv"
    echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
} > "$name.log"
timeout 60 python3 -I "$name.py" jumpy_imu.csv > "$name.out" 2>&1
rc=$?
echo "exit code: $rc" >> "$name.log"
exit $rc
