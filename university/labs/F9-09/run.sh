#!/usr/bin/env bash
# F9-09: check that the recorded log fed to odometry.cpp (odometry.in) is exactly what
# the simulator printed in this run (diffdrive_sim.out), so the input is not hand-made.
set -u
name=log_matches_sim
{
    echo "listing:   run.sh (cmp diffdrive_sim.out odometry.in)"
    echo "toolchain: $(cmp --version | head -n 1)"
    echo "command:   cmp diffdrive_sim.out odometry.in"
    echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
} > "${name}.log"
if cmp diffdrive_sim.out odometry.in > "${name}.out" 2>&1; then
    echo "identical: odometry.in is the simulator's recorded log" >> "${name}.out"
    echo "exit code: 0" >> "${name}.log"
else
    echo "exit code: 1" >> "${name}.log"; exit 1
fi
