#!/usr/bin/env bash
# F9-50 lab steps (cm_demo.cpp and cm_forensic.cpp are built and run by run_lab.sh itself).
#  1. the forensic evidence: the difference between the two hardware interfaces;
#  2. cm_rt.cc with sanitizers, 1 s smoke run;
#  3. cm_rt.cc at -O2: 1 kHz for 5 s, then 5 s pinned to the last CPU; requirement check.
set -u -o pipefail
cd "$(dirname "$0")"
status=0
B=.build
rm -rf "$B"; mkdir -p "$B"
TC="$(g++ --version | head -n 1)"
SAN="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined -pthread"
OPT="-std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror -pthread"
MEAS="measured: on the build container (a virtual machine shared with other work, kernel without PREEMPT_RT), run as root; numbers change from run to run (AH-23)"
rec() {  # rec <name> <listing> <toolchain> <command> <exit code> [extra line]
    {
        echo "listing:   $2"
        echo "toolchain: $3"
        echo "command:   $4"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container), kernel $(uname -r), $(nproc) CPUs"
        echo "exit code: $5"
        if [ $# -ge 6 ]; then echo "$6"; fi
    } > "$1.log"
}

diff sim_hw.h sim_hw_v2.h > hw_diff.out; rc=$?
rec hw_diff "sim_hw.h sim_hw_v2.h" "$(diff --version | head -n 1)" "diff sim_hw.h sim_hw_v2.h" \
    "$rc (1 = the files differ, which is expected)"
[ "$rc" = 1 ] || status=1

g++ $SAN cm_rt.cc -o $B/cm_rt_san && timeout 30 $B/cm_rt_san 1 > cm_rt_smoke.out 2>&1; rc=$?
rec cm_rt_smoke "cm_rt.cc uctl.h sim_hw.h controllers.h" "$TC" "g++ $SAN cm_rt.cc -o cm_rt; ./cm_rt 1" "$rc" \
    "note:      sanitizer build, 1 s run: checks memory and undefined behaviour; timing not used"
[ "$rc" = 0 ] || status=1

g++ $OPT cm_rt.cc -o $B/cm_rt || status=1
timeout 30 $B/cm_rt 5 > cm_rt.out 2>&1; rc=$?
rec cm_rt "cm_rt.cc uctl.h sim_hw.h controllers.h" "$TC" "g++ $OPT cm_rt.cc -o cm_rt; ./cm_rt 5" "$rc" "$MEAS"
[ "$rc" = 0 ] || status=1
last=$(( $(nproc) - 1 ))
timeout 30 $B/cm_rt 5 "$last" > cm_rt_pinned.out 2>&1; rc=$?
rec cm_rt_pinned "cm_rt.cc uctl.h sim_hw.h controllers.h" "$TC" "./cm_rt 5 $last" "$rc" "$MEAS"
[ "$rc" = 0 ] || status=1
rm -rf "$B"
exit $status
