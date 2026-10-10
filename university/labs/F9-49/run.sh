#!/usr/bin/env bash
# F9-49 lab steps: build rt_node.cc (with uexec.h) with sanitizers for a short smoke run, then
# at -O2 without sanitizers; run the "bad" configuration (forensic evidence) and the "fixed" one.
# rt_node.cc is a .cc file so that run_lab.sh does not build it with sanitizers only:
# sanitizers add work to every memory access and would distort the measurement.
set -u -o pipefail
cd "$(dirname "$0")"
status=0
B=.build
rm -rf "$B"; mkdir -p "$B"
TC="$(g++ --version | head -n 1)"
SAN="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined -pthread"
OPT="-std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror -pthread"
MEAS="measured: on the build container (a virtual machine shared with other work, kernel without PREEMPT_RT), run as root; numbers change from run to run (AH-23)"
rec() {  # rec <name> <command> <exit code> [extra line]
    {
        echo "listing:   uexec.h rt_node.cc"
        echo "toolchain: $TC"
        echo "command:   $2"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container), kernel $(uname -r), $(nproc) CPUs"
        echo "exit code: $3"
        if [ $# -ge 4 ]; then echo "$4"; fi
    } > "$1.log"
}

g++ $SAN rt_node.cc -o $B/rt_node_san && timeout 30 $B/rt_node_san smoke > node_smoke.out 2>&1
rc=$?; rec node_smoke "g++ $SAN rt_node.cc -o rt_node; ./rt_node smoke" "$rc" \
    "note:      sanitizer build, 0.3 s run: checks memory and undefined behaviour; timing not used"
[ "$rc" = 0 ] || status=1

g++ $OPT rt_node.cc -o $B/rt_node || status=1
for mode in bad fixed; do
    timeout 30 $B/rt_node $mode > node_$mode.out 2>&1; rc=$?
    rec node_$mode "g++ $OPT rt_node.cc -o rt_node; ./rt_node $mode" "$rc" "$MEAS"
    [ "$rc" = 0 ] || status=1
done
grep -q "allocations inside control callbacks: 0 " node_fixed.out || status=1
rm -rf "$B"
exit $status
