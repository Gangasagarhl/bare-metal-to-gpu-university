#!/usr/bin/env bash
# F9-11: check that the recorded scan log fed to view_scan.cpp (view_scan.in) is exactly what
# record_scan.cpp printed in this run (record_scan.out), so the input is not hand-made.
set -u
name=log_matches_record
{
    echo "listing:   run.sh (cmp record_scan.out view_scan.in)"
    echo "toolchain: $(cmp --version | head -n 1)"
    echo "command:   cmp record_scan.out view_scan.in"
    echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
} > "${name}.log"
if cmp record_scan.out view_scan.in > "${name}.out" 2>&1; then
    echo "identical: view_scan.in is the recorded scan log" >> "${name}.out"
    echo "exit code: 0" >> "${name}.log"
else
    echo "exit code: 1" >> "${name}.log"; exit 1
fi
