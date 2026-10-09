#!/usr/bin/env bash
# F1-39: the NUMA layout the kernel reports, and the multi-thread read bandwidth
# (-O2, -pthread, no sanitizers: a timing on this machine).
set -u -o pipefail
cd "$(dirname "$0")"
MACH="$(uname -s) $(uname -m) (cloud build container, $(nproc) CPUs); CPU model: $(grep -m1 'model name' /proc/cpuinfo | cut -d: -f2 | sed 's/^ //')"
rec() {
    {
        echo "listing:   $2"
        echo "toolchain: $3"
        echo "command:   $4"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $MACH"
        echo "exit code: $5"
        echo "$6"
    } > "$1.log"
}
status=0
./numa_info.sh > numa_info.out 2>&1; rc=$?; [ $rc = 0 ] || status=1
rec numa_info numa_info.sh "$(bash --version | head -n 1)" "./numa_info.sh" $rc \
    "note:      reported by the kernel of the build container (a virtual machine); a multi-socket server would list more nodes"
OPT="g++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror -pthread"
$OPT threads_bw.cc -o .bin_tbw || exit 1
timeout 120 ./.bin_tbw > threads_bw.out 2>&1; rc=$?; [ $rc = 0 ] || status=1
rec threads_bw threads_bw.cc "$(g++ --version | head -n 1)" "$OPT threads_bw.cc -o threads_bw && ./threads_bw" $rc \
    "note:      measured on the build container (a shared cloud virtual machine), one session; not a specification (AH-23)"
rm -f .bin_tbw
exit $status
