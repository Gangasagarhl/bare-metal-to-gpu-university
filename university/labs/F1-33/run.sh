#!/usr/bin/env bash
# F1-33: (1) the cache geometry the OS reports, (2) the simulator on the 2-way and fully
# associative versions of the hand trace and on a generated "same set" trace,
# (3) the measured conflict-miss program (-O2, no sanitizers: a timing).
set -u -o pipefail
cd "$(dirname "$0")"
TC="$(g++ --version | head -n 1)"
MACH="$(uname -s) $(uname -m) (cloud build container, $(nproc) CPUs); CPU model: $(grep -m1 'model name' /proc/cpuinfo | cut -d: -f2 | sed 's/^ //')"
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
./cache_info.sh > cache_info.out 2>&1; rc=$?; [ $rc = 0 ] || status=1
rec cache_info cache_info.sh "$(bash --version | head -n 1)" "./cache_info.sh" $rc \
    "note:      values reported by the kernel of the build container (a virtual machine); not checked against any CPU datasheet"
SAN="g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
$SAN cachesim.cpp -o .bin_sim || exit 1
# same trace as cachesim.in, other geometries (same total size: 4 blocks of 16 B)
{ echo "2 2 16"; tail -n +2 cachesim.in; } > cachesim_2way.in
{ echo "1 4 16"; tail -n +2 cachesim.in; } > cachesim_full.in
# 16 blocks 4096 B apart, visited 3 times, in a 64-set 12-way 64 B-line cache
{ echo "64 12 64"; for r in 1 2 3; do for j in $(seq 0 15); do printf '%x\n' $((j * 4096)); done; done; } > sameset.in
for t in cachesim_2way cachesim_full sameset; do
    ./.bin_sim < $t.in > $t.out 2>&1; rc=$?; [ $rc = 0 ] || status=1
    rec $t cachesim.cpp "$TC" "$SAN cachesim.cpp -o cachesim && ./cachesim < $t.in" $rc "stdin:     $t.in"
done
OPT="g++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror"
$OPT conflict.cc -o .bin_conflict || exit 1
timeout 120 ./.bin_conflict > conflict.out 2>&1; rc=$?; [ $rc = 0 ] || status=1
rec conflict conflict.cc "$TC" "$OPT conflict.cc -o conflict && ./conflict" $rc \
    "note:      measured on the build container (a shared cloud virtual machine), one session; not a specification (AH-23)"
rm -f .bin_sim .bin_conflict
exit $status
