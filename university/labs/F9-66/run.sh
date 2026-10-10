#!/usr/bin/env bash
# F9-66 run.sh: wake-up latency of a 1 ms periodic thread on the build machine, under four
# conditions, plus the forensic evidence (a higher-priority SCHED_FIFO thread on the same CPU).
# The timing build uses -O2 without sanitizers (sanitizers change timing); the same source is
# also built and run once with AddressSanitizer and UBSan as the correctness check (AH-27).
set -u -o pipefail
cd "$(dirname "$0")"
. ../F9-67/rblib.sh
status=0
B=.build
rm -rf "$B"; mkdir -p "$B"
ok() { [ "$1" = "$2" ] || { echo "F9-66 step $3: exit $1, expected $2" >&2; status=1; }; }
KVER="kernel:    $(uname -r) ($(uname -v | grep -o 'PREEMPT[A-Z_]*' | head -n 1) in uname -v; no PREEMPT_RT)"
HW="hardware:  measured inside the cloud build container (a virtual machine with $(nproc) virtual CPUs), not on a robot computer"

g++ $HOSTFLAGS -pthread latency.cc -o $B/lat_san && timeout 20 $B/lat_san 200 1000 1 other > latency_sanitizers.out 2>&1; rc=$?
rec latency_sanitizers "latency.cc" "$GXX_VER" "g++ $HOSTFLAGS -pthread latency.cc -o latency; ./latency 200 1000 1 other" "$rc" \
    "note:      correctness run with sanitizers; its timing is not used"
ok "$rc" 0 latency_sanitizers
g++ $MEASFLAGS latency.cc -o $B/lat || status=1

run() {   # run NAME POLICY LOAD
    timeout 30 $B/lat 2000 1000 1 "$2" "$3" > "$1.out" 2>&1; local rc=$?
    rec "$1" "latency.cc" "$GXX_VER" "g++ $MEASFLAGS latency.cc -o latency; ./latency 2000 1000 1 $2 $3" "$rc" "$KVER" "$HW"
    ok "$rc" 0 "$1"
}
run lat_other_idle other none
run lat_fifo_idle fifo:80 none
run lat_other_busy other busy
run lat_fifo_busy fifo:80 busy
run forensic_lat fifo:80 hog:90:20:100

rm -rf "$B"
exit $status
