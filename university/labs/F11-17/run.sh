#!/usr/bin/env bash
# F11-17 run.sh: run umask_demo.sh (real Linux behaviour) and record it.
set -u
cd "$(dirname "$0")"
./umask_demo.sh > umask_demo.out 2>&1; rc=$?
printf 'listing:   umask_demo.sh\ntoolchain: %s; %s\ncommand:   ./umask_demo.sh\ndate:      %s\nmachine:   %s\nexit code: %s\n' \
    "$(bash --version | head -n 1)" "$(stat --version | head -n 1)" "$(date -u +%Y-%m-%dT%H:%M:%SZ)" \
    "$(uname -s) $(uname -m) (cloud build container)" "$rc" > umask_demo.log
exit $rc
