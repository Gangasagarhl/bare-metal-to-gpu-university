#!/usr/bin/env bash
# F1-38: the first-touch and TLB-reach measurements (-O2, no sanitizers: timings on this machine).
set -u -o pipefail
cd "$(dirname "$0")"
TC="$(g++ --version | head -n 1)"
MACH="$(uname -s) $(uname -m) (cloud build container, $(nproc) CPUs); CPU model: $(grep -m1 'model name' /proc/cpuinfo | cut -d: -f2 | sed 's/^ //'); THP mode: $(cat /sys/kernel/mm/transparent_hugepage/enabled 2>/dev/null)"
NOTE="note:      measured on the build container (a shared cloud virtual machine), one session; not a specification (AH-23)"
rec() {
    {
        echo "listing:   $2"
        echo "toolchain: $TC"
        echo "command:   $3"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $MACH"
        echo "exit code: $4"
        echo "$NOTE"
    } > "$1.log"
}
status=0
OPT="g++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror"
for p in first_touch tlb_reach; do
    $OPT $p.cc -o .bin_$p || exit 1
    timeout 120 ./.bin_$p > $p.out 2>&1; rc=$?; [ $rc = 0 ] || status=1
    rec $p $p.cc "$OPT $p.cc -o $p && ./$p" $rc
    rm -f .bin_$p
done
exit $status
