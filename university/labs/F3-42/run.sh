#!/usr/bin/env bash
# F3-42 lab step (the C++ models ec_sim, forensic and bmc_sim are run by run_lab.sh):
# list the BMC and EC-class machines that this build's QEMU can emulate. Booting a real
# OpenBMC image on them needs an image this build does not have (untested).
set -u -o pipefail
cd "$(dirname "$0")"
QV="$(qemu-system-arm --version | head -n 1)"
qemu-system-arm -machine help | grep -i -E 'bmc|ast[0-9]' > machines.out 2>&1; rc=$?
{
    echo "listing:   (QEMU machine list)"
    echo "toolchain: $QV"
    echo "command:   qemu-system-arm -machine help | grep -i -E 'bmc|ast[0-9]'"
    echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
    echo "exit code: $rc"
    echo "hardware:  untested on hardware; only the list of emulated machines was produced, no firmware image was booted"
} > machines.log
[ "$rc" = 0 ] && [ -s machines.out ]
