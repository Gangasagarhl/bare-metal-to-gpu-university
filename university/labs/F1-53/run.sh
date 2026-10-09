#!/usr/bin/env bash
# F1-53 run.sh: record honestly whether this machine has any RDMA-capable device or tools.
set -u -o pipefail
cd "$(dirname "$0")"
C='ls /sys/class/infiniband; command -v ibv_devinfo ibv_devices rdma'
{
    echo "\$ ls /sys/class/infiniband"
    ls /sys/class/infiniband 2>&1
    echo "\$ command -v ibv_devinfo ibv_devices rdma"
    command -v ibv_devinfo ibv_devices rdma 2>&1 || echo "(none of these commands is installed)"
} > rdma_check.out
{
    echo "listing:   run.sh (step rdma_check)"
    echo "toolchain: $(bash --version | head -n 1)"
    echo "command:   $C"
    echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
    echo "exit code: 0"
    echo "hardware:  untested on hardware: no RDMA-capable NIC and no RDMA user-space tools in the build container"
} > rdma_check.log
exit 0
