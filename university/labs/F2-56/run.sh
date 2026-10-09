#!/usr/bin/env bash
# F2-56 steps: measure the two roofs (peak.cc, stream.cc), run the SGEMM speed table
# (sgemm.cc), then place everything on the roofline (roofline.cc reads their output).
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
for p in peak stream; do
    g++ $W -O2 -pthread $p.cc -o .bin_$p || exit 1
    timeout 300 ./.bin_$p > $p.out 2>&1; rc=$?; [ $rc = 0 ] || status=1
    rec $p $p.cc "$TC" "g++ $W -O2 -pthread $p.cc -o $p && ./$p" $rc "$NOTE"
done
g++ $W -O3 -march=native -pthread sgemm.cc -o .bin_sgemm || exit 1
timeout 600 ./.bin_sgemm > sgemm.out 2>&1; rc=$?; [ $rc = 0 ] || status=1
rec sgemm sgemm.cc "$TC" "g++ $W -O3 -march=native -pthread sgemm.cc -o sgemm && ./sgemm" $rc "$NOTE"
grep -hE '^(ROOF|KERNEL) ' peak.out stream.out sgemm.out > roofline.in
g++ $W -O2 roofline.cc -o .bin_roofline || exit 1
./.bin_roofline < roofline.in > roofline.out 2>&1; rc=$?; [ $rc = 0 ] || status=1
rec roofline roofline.cc "$TC" "grep -hE '^(ROOF|KERNEL) ' peak.out stream.out sgemm.out > roofline.in && g++ $W -O2 roofline.cc -o roofline && ./roofline < roofline.in" $rc \
    "stdin:     roofline.in (generated in this run from the three measurements above)"
rm -f .bin_peak .bin_stream .bin_sgemm .bin_roofline
exit $status
