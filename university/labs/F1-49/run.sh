#!/usr/bin/env bash
# F1-49 run.sh: look at the block devices of the machine that runs the lab (here: the
# cloud build container, which is itself a virtual machine) and record what it reports.
set -u -o pipefail
cd "$(dirname "$0")"
rec() {
    {
        echo "listing:   run.sh (step $1)"
        echo "toolchain: $2"
        echo "command:   $3"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
        echo "exit code: $4"
        if [ $# -ge 5 ]; then echo "$5"; fi
    } > "$1.log"
}
C1="lsblk -d -o NAME,TYPE,ROTA,LOG-SEC,PHY-SEC,SIZE"
C2="lspci -nn | grep -i -E 'storage|nvm|sata'"
{
    echo "\$ $C1"; lsblk -d -o NAME,TYPE,ROTA,LOG-SEC,PHY-SEC,SIZE; rc1=$?
    echo; echo "\$ $C2"; lspci -nn | grep -i -E 'storage|nvm|sata'; rc2=$?
} > my_disks.out 2>&1
rc=$(( ${rc1:-0} + ${rc2:-0} ))
rec my_disks "$(lsblk --version 2>&1 | head -n 1); $(lspci --version 2>&1 | head -n 1)" "$C1; $C2" "$rc" \
    "hardware:  the build container is a virtual machine; its disks are virtual devices, not physical drives"
exit "$rc"
