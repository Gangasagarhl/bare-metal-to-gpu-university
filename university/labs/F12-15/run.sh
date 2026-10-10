#!/usr/bin/env bash
# F12-15 run.sh - real scalability measurement on the build machine, fitted with the USL.
#   scale_usl : ./scale (Listing 3, -O2) piped into the USL fit of usl.cpp (Listing 2)
# The deterministic listings (capacity, usl with its exercise data, outage) are .cpp files
# built by run_lab.sh with sanitizers.
set -u
cd "$(dirname "$0")"
status=0
B=.build
rm -rf "$B"; mkdir -p "$B"
GXXV="$(g++ --version | head -n 1)"
CPU="$(grep -m1 'model name' /proc/cpuinfo | sed 's/.*: //')"
rec() {  # rec <name> <listing> <command> <exit code text> [note]
    {
        echo "listing:   $2"
        echo "toolchain: $GXXV"
        echo "command:   $3"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container, $(nproc) CPUs visible, $CPU)"
        echo "exit code: $4"
        if [ $# -ge 5 ]; then echo "note:      $5"; fi
    } > "$1.log"
}

g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -O2 -pthread scale.cc -o $B/scale || exit 1
g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -O2 usl.cpp -o $B/usl || exit 1
timeout 120 $B/scale > $B/scale.txt 2>&1; rc1=$?
{
    echo "--- raw measurement from ./scale"
    cat $B/scale.txt
    echo "--- USL fit by ./usl"
    $B/usl < $B/scale.txt
} > scale_usl.out 2>&1; rc2=$?
rc=$(( rc1 != 0 ? rc1 : rc2 ))
rec scale_usl "scale.cc, usl.cpp" "g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -O2 -pthread scale.cc -o scale; g++ ... -O2 usl.cpp -o usl; ./scale > scale.txt; ./usl < scale.txt" \
    "$rc" "measured on the build container (a shared cloud virtual machine, other jobs running); throughput changes from run to run and is not a specification (AH-23)"
[ "$rc" = 0 ] || status=1

rm -rf "$B"
exit $status
