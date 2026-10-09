#!/usr/bin/env bash
# F1-34: the stride benchmark and the traversal-order programs (-O2, no sanitizers: timings),
# an attempt to use perf, and Cachegrind's simulated cache counts for both traversal orders.
set -u -o pipefail
cd "$(dirname "$0")"
TC="$(g++ --version | head -n 1)"
MACH="$(uname -s) $(uname -m) (cloud build container, $(nproc) CPUs); CPU model: $(grep -m1 'model name' /proc/cpuinfo | cut -d: -f2 | sed 's/^ //')"
NOTE="note:      measured on the build container (a shared cloud virtual machine), one session; not a specification (AH-23)"
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
OPT="g++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror"
$OPT stride.cc -o .bin_stride || exit 1
$OPT traversal.cc -o traversal || exit 1
timeout 300 ./.bin_stride > stride.out 2>&1; rc=$?; [ $rc = 0 ] || status=1
rec stride stride.cc "$TC" "$OPT stride.cc -o stride && ./stride" $rc "$NOTE"
timeout 120 ./traversal 4096 > traversal.out 2>&1; rc=$?; [ $rc = 0 ] || status=1
rec traversal traversal.cc "$TC" "$OPT traversal.cc -o traversal && ./traversal 4096" $rc "$NOTE"
# perf: recorded honestly, whatever happens (a failure here is the expected, documented result)
./perf_try.sh > perf_try.out 2>&1; rc=$?
rec perf_try perf_try.sh "perf wrapper script $(command -v perf) (no perf binary matching the running kernel is installed)" "perf stat -e cache-misses,cache-references ./traversal 2048 cols" $rc \
    "result:    perf could not count on the build container (see perf_try.out); recorded as evidence, not as a failure of the lab"
# Cachegrind: a cache SIMULATOR, configured with the L1d/L2 geometry the kernel reports (F1-33 cache_info.out)
VG="valgrind --tool=cachegrind --cache-sim=yes --cachegrind-out-file=/dev/null --I1=32768,8,64 --D1=49152,12,64 --LL=2097152,16,64"
for o in rows cols; do
    timeout 300 $VG ./traversal 2048 $o > cachegrind_$o.out 2>&1; rc=$?; [ $rc = 0 ] || status=1
    sed -i 's/^==[0-9]*== //' cachegrind_$o.out
    rec cachegrind_$o traversal.cc "$(valgrind --version)" "$VG ./traversal 2048 $o" $rc \
        "note:      Cachegrind simulates a cache; these are simulated counts, not hardware counter readings"
done
# check that the --LL option really replaces the host description printed in the warnings:
# the same column walk with a 256 MiB simulated LL must lose almost all its LL read misses
VG2="valgrind --tool=cachegrind --cache-sim=yes --cachegrind-out-file=/dev/null --I1=32768,8,64 --D1=49152,12,64 --LL=268435456,16,64"
timeout 300 $VG2 ./traversal 2048 cols > cachegrind_cols_bigLL.out 2>&1; rc=$?; [ $rc = 0 ] || status=1
sed -i 's/^==[0-9]*== //' cachegrind_cols_bigLL.out
rec cachegrind_cols_bigLL traversal.cc "$(valgrind --version)" "$VG2 ./traversal 2048 cols" $rc \
    "note:      control run: only the simulated LL size differs from cachegrind_cols"
rm -f .bin_stride traversal
exit $status
