#!/usr/bin/env bash
# F9-46 lab steps (sched_sim.cpp and analysis.cpp are built and run by run_lab.sh itself).
#  1. deadline_linux.cc with sanitizers, short smoke run;
#  2. deadline_linux.cc at -O2: admission control, a periodic SCHED_DEADLINE job, throttling;
#  3. the kernel's limit on real-time bandwidth, read from /proc (used to explain step 2).
set -u -o pipefail
cd "$(dirname "$0")"
status=0
B=.build
rm -rf "$B"; mkdir -p "$B"
TC="$(g++ --version | head -n 1)"
SAN="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
OPT="-std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror"
MEAS="measured: on the build container (a virtual machine shared with other work), run as root; times change from run to run (AH-23)"
rec() {  # rec <name> <listing> <command> <exit code> [extra line]
    {
        echo "listing:   $2"
        echo "toolchain: $TC"
        echo "command:   $3"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container), kernel $(uname -r), $(nproc) CPUs"
        echo "exit code: $4"
        if [ $# -ge 5 ]; then echo "$5"; fi
    } > "$1.log"
}

g++ $SAN deadline_linux.cc -o $B/dl_san && timeout 30 $B/dl_san smoke > deadline_smoke.out 2>&1
rc=$?; rec deadline_smoke deadline_linux.cc "g++ $SAN deadline_linux.cc -o deadline_linux; ./deadline_linux smoke" "$rc" \
    "note:      sanitizer build, short run: checks memory and undefined behaviour"
[ "$rc" = 0 ] || status=1

g++ $OPT deadline_linux.cc -o $B/dl || status=1
timeout 60 $B/dl > deadline.out 2>&1; rc=$?
rec deadline deadline_linux.cc "g++ $OPT deadline_linux.cc -o deadline_linux; ./deadline_linux" "$rc" "$MEAS"
[ "$rc" = 0 ] || status=1
grep -q "accepted" deadline.out || status=1

{
    echo "/proc/sys/kernel/sched_rt_period_us:  $(cat /proc/sys/kernel/sched_rt_period_us)"
    echo "/proc/sys/kernel/sched_rt_runtime_us: $(cat /proc/sys/kernel/sched_rt_runtime_us)"
    echo "online CPUs (nproc): $(nproc)"
} > rt_limits.out 2>&1; rc=$?
TC="$(cat --version | head -n 1)"
rec rt_limits "(shell commands)" "cat /proc/sys/kernel/sched_rt_period_us /proc/sys/kernel/sched_rt_runtime_us; nproc" "$rc"

rm -rf "$B"
exit $status
