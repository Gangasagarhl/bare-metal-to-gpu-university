#!/usr/bin/env bash
# F9-45 lab steps (timing_terms.cpp is built and run by run_lab.sh itself).
#  1. loop_1khz.cc with sanitizers, short smoke run (checks the code, not the timing);
#  2. loop_1khz.cc at -O2 without sanitizers, absolute releases, 3000 cycles (the measurement);
#  3. the same program with relative sleeps (the forensic evidence).
# Timing programs are .cc files so that run_lab.sh does not build them with sanitizers only:
# sanitizers add work to every memory access and would distort the measurement.
set -u -o pipefail
cd "$(dirname "$0")"
status=0
B=.build
rm -rf "$B"; mkdir -p "$B"
TC="$(g++ --version | head -n 1)"
SAN="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
OPT="-std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror"
MEAS="measured: on the build container (a virtual machine shared with other work); numbers change from run to run and are not a property of any product (AH-23)"
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

# 1. sanitizer smoke run
g++ $SAN loop_1khz.cc -o $B/loop_san && timeout 20 $B/loop_san absolute 50 > loop_smoke.out 2>&1
rc=$?; rec loop_smoke loop_1khz.cc "g++ $SAN loop_1khz.cc -o loop_1khz; ./loop_1khz absolute 50" "$rc" \
    "note:      sanitizer build: checks memory and undefined behaviour; its timing is not used"
[ "$rc" = 0 ] || status=1

# 2. the measurement: absolute releases
g++ $OPT loop_1khz.cc -o $B/loop || status=1
timeout 30 $B/loop absolute 3000 > loop_abs.out 2>&1; rc=$?
rec loop_abs loop_1khz.cc "g++ $OPT loop_1khz.cc -o loop_1khz; ./loop_1khz absolute 3000" "$rc" "$MEAS"
[ "$rc" = 0 ] || status=1

# 3. forensic evidence: relative sleeps
timeout 30 $B/loop relative 3000 > loop_rel.out 2>&1; rc=$?
rec loop_rel loop_1khz.cc "./loop_1khz relative 3000 (same binary as step 2)" "$rc" "$MEAS"
[ "$rc" = 0 ] || status=1

rm -rf "$B"
exit $status
