#!/usr/bin/env bash
# F1-42: which interrupts does the build machine take in about one second?
# Two snapshots of /proc/interrupts, compared by irq_delta.py.
set -u -o pipefail
cd "$(dirname "$0")"
cat /proc/interrupts > .a.txt; t0=$(date +%s.%N)
sleep 1
cat /proc/interrupts > .b.txt; t1=$(date +%s.%N)
dt=$(python3 -I -c "import sys; print('%.3f' % (float(sys.argv[2]) - float(sys.argv[1])))" "$t0" "$t1")
{ echo "interval: $dt s, all CPUs added together"; python3 -I irq_delta.py .a.txt .b.txt "$dt"; } > irq_delta.out 2>&1; rc=$?
{
    echo "listing:   irq_delta.py"
    echo "toolchain: Linux $(uname -r); Python $(python3 -c 'import sys; print(sys.version.split()[0])')"
    echo "command:   cat /proc/interrupts; sleep 1; cat /proc/interrupts; python3 -I irq_delta.py before after $dt"
    echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "machine:   $(uname -s) $(uname -m) (cloud build container), $(nproc) CPUs visible"
    echo "exit code: $rc"
    echo "note:      measured once on a shared virtual machine (AH-23); counts change from second to second"
} > irq_delta.log
rm -f .a.txt .b.txt
exit $rc
