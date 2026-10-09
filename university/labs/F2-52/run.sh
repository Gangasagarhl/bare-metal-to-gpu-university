#!/usr/bin/env bash
# F2-52 steps: timings at -O2 without sanitizers (.cc files), the kernel's cache report,
# and cachegrind's simulated cache counts for the two traversals.
set -u -o pipefail
cd "$(dirname "$0")"
TC="$(g++ --version | head -n 1)"
MACH="$(uname -s) $(uname -m) (cloud build container, $(nproc) CPUs); CPU model: $(grep -m1 'model name' /proc/cpuinfo | cut -d: -f2 | sed 's/^ //')"
NOTE="note:      measured on the build container (a shared cloud virtual machine, other jobs may run at the same time), one session; not a specification (AH-23)"
rec() {
    {
        echo "listing:   $2"
        echo "toolchain: $3"
        echo "command:   $4"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $MACH"
        echo "exit code: $5"
        if [ $# -ge 6 ]; then echo "$6"; fi
    } > "$1.log"
}
status=0
W="-std=c++20 -Wall -Wextra -Wpedantic -Werror"
./cacheinfo.sh > cacheinfo.out 2>&1; rc=$?; [ $rc = 0 ] || status=1
rec cacheinfo cacheinfo.sh "$(bash --version | head -n 1)" "./cacheinfo.sh" $rc \
    "note:      values reported by the kernel of a virtual machine; not checked against any CPU datasheet (AH-21)"
for p in traverse chase transpose; do
    g++ $W -O2 $p.cc -o .bin_$p || exit 1
    timeout 300 ./.bin_$p > $p.out 2>&1; rc=$?; [ $rc = 0 ] || status=1
    rec $p $p.cc "$TC" "g++ $W -O2 $p.cc -o $p && ./$p" $rc "$NOTE"
done
g++ $W -O2 -pthread sharing.cc -o .bin_sharing || exit 1
timeout 300 ./.bin_sharing > sharing.out 2>&1; rc=$?; [ $rc = 0 ] || status=1
rec sharing sharing.cc "$TC" "g++ $W -O2 -pthread sharing.cc -o sharing && ./sharing" $rc "$NOTE"
# cachegrind: a simulated cache, so the counts are the same on every run (unlike times).
VG="$(valgrind --version)"
CG="valgrind --tool=cachegrind --cache-sim=yes --cachegrind-out-file=cg.tmp ./traverse 1024 1"
valgrind --tool=cachegrind --cache-sim=yes --cachegrind-out-file=.cg.tmp ./.bin_traverse 1024 1 > .cg.stdout 2> .cg.stderr; rc=$?
{
    echo "--- program output"; cat .cg.stdout
    echo "--- cachegrind summary (stderr, process id replaced by PID)"
    sed -E 's/^(==|--)[0-9]+(==|--)/\1PID\2/' .cg.stderr | grep -E 'I1|D1|LL|D  |refs|misses|miss rate|Cache'
    echo "--- cg_annotate: per-function counts (columns as printed by cg_annotate)"
    cg_annotate .cg.tmp 2>&1 | grep -E '^Events shown|^ +[0-9,]+ .*(sumRows|sumColumns)\(' | sed -E 's/ +/ /g'
} > traverse_cg.out 2>&1
rec traverse_cg traverse.cc "$VG; $TC" "$CG && cg_annotate cg.tmp" $rc \
    "note:      cache behaviour simulated by cachegrind (its own model of the caches), not measured by hardware counters"
rm -f .bin_traverse .bin_chase .bin_transpose .bin_sharing .cg.tmp .cg.stdout .cg.stderr
exit $status
