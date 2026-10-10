#!/usr/bin/env bash
# F12-13 run.sh - the real A/B benchmark (timing build, -O2, no sanitizers).
# The deterministic listings (omission, bootstrap, drift) are .cpp files built by run_lab.sh.
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

g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -O2 ab_bench.cc -o $B/ab_bench || exit 1
timeout 300 $B/ab_bench > ab_bench.out 2>&1; rc=$?
rec ab_bench ab_bench.cc "g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -O2 ab_bench.cc -o ab_bench; ./ab_bench" \
    "$rc" "measured on the build container (a shared cloud virtual machine); timings change from run to run and are not a specification (AH-23)"
[ "$rc" = 0 ] || status=1

rm -rf "$B"
exit $status
