#!/usr/bin/env bash
# F1-32: build the two timing programs with optimisation (-O2, no sanitizers, so the timing is
# not distorted), run them on this machine, then feed the measured large-block numbers into
# littles.cpp. Every number printed is a measurement on the build container (AH-23).
set -u -o pipefail
cd "$(dirname "$0")"
TC="$(g++ --version | head -n 1)"
rec() {
    {
        echo "listing:   $2"
        echo "toolchain: $TC"
        echo "command:   $3"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container, $(nproc) CPUs); CPU model: $(grep -m1 'model name' /proc/cpuinfo | cut -d: -f2 | sed 's/^ //')"
        echo "exit code: $4"
        if [ $# -ge 5 ]; then echo "$5"; fi
    } > "$1.log"
}
NOTE="note:      measured on the build container (a shared cloud virtual machine), one session; not a specification of any CPU (AH-23)"
OPT="g++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror"
status=0
$OPT chase.cc -o .bin_chase || exit 1
$OPT bandwidth.cc -o .bin_bw || exit 1
timeout 120 ./.bin_chase random > chase_random.out 2>&1; rc=$?; [ $rc = 0 ] || status=1
rec chase_random chase.cc "$OPT chase.cc -o chase && ./chase random" $rc "$NOTE"
timeout 120 ./.bin_chase sequential > chase_sequential.out 2>&1; rc=$?; [ $rc = 0 ] || status=1
rec chase_sequential chase.cc "$OPT chase.cc -o chase && ./chase sequential" $rc "$NOTE"
timeout 120 ./.bin_bw > bandwidth.out 2>&1; rc=$?; [ $rc = 0 ] || status=1
rec bandwidth bandwidth.cc "$OPT bandwidth.cc -o bandwidth && ./bandwidth" $rc "$NOTE"
# Little's law with this run's own numbers: the largest working set of each program.
lat="$(grep ' KiB' chase_random.out | tail -n 1 | awk '{print $3}')"
bw="$(grep ' KiB' bandwidth.out | tail -n 1 | awk '{print $3}')"
echo "measured-largest-block $lat $bw" > littles_measured.in
SAN="g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
$SAN littles.cpp -o .bin_littles || exit 1
./.bin_littles < littles_measured.in > littles_measured.out 2>&1; rc=$?; [ $rc = 0 ] || status=1
rec littles_measured littles.cpp "$SAN littles.cpp -o littles && ./littles < littles_measured.in" $rc \
    "note:      input taken from the last line of chase_random.out and bandwidth.out of this same run (measured on the build container)"
rm -f .bin_chase .bin_bw .bin_littles
exit $status
