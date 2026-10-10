#!/usr/bin/env bash
# F9-70 run.sh: the course forensic case "robot froze for two seconds", reproduced with real
# SCHED_FIFO threads on the build machine: first with the control loop and the (simulated)
# camera driver sharing CPU 1, then with the control loop moved to CPU 2 (the re-measurement
# after isolating it). Timing builds use -O2 without sanitizers; a sanitizer build of the same
# source runs first as the correctness check (AH-27).
set -u -o pipefail
cd "$(dirname "$0")"
. ../F9-67/rblib.sh
status=0
B=.build
rm -rf "$B"; mkdir -p "$B"
ok() { [ "$1" = "$2" ] || { echo "F9-70 step $3: exit $1, expected $2" >&2; status=1; }; }
KVER="kernel:    $(uname -r) ($(uname -v | grep -o 'PREEMPT[A-Z_]*' | head -n 1) in uname -v; no PREEMPT_RT)"
HW="hardware:  measured inside the cloud build container (a virtual machine with $(nproc) virtual CPUs); the camera driver is simulated by a thread, untested with a real USB camera"

g++ $HOSTFLAGS -pthread freeze.cc -o $B/freeze_san && timeout 20 $B/freeze_san 2 1 > freeze_sanitizers.out 2>&1; rc=$?
rec freeze_sanitizers "freeze.cc flightrec.h" "$GXX_VER" "g++ $HOSTFLAGS -pthread freeze.cc -o freeze; ./freeze 2 1" "$rc" \
    "note:      correctness run with sanitizers; its timing is not used" "$KVER"
ok "$rc" 0 freeze_sanitizers
g++ $MEASFLAGS freeze.cc -o $B/freeze || status=1
timeout 20 $B/freeze 1 1 > freeze_shared.out 2>&1; rc=$?
rec freeze_shared "freeze.cc flightrec.h" "$GXX_VER" "g++ $MEASFLAGS freeze.cc -o freeze; ./freeze 1 1" "$rc" "$KVER" "$HW"
ok "$rc" 0 freeze_shared
timeout 20 $B/freeze 2 1 > freeze_isolated.out 2>&1; rc=$?
rec freeze_isolated "freeze.cc flightrec.h" "$GXX_VER" "g++ $MEASFLAGS freeze.cc -o freeze; ./freeze 2 1" "$rc" "$KVER" "$HW"
ok "$rc" 0 freeze_isolated
{ echo "CPUs online: $(nproc)"; echo "kernel: $(uname -r)"; echo "uname -v: $(uname -v)"
  echo "RT throttling: sched_rt_runtime_us=$(cat /proc/sys/kernel/sched_rt_runtime_us) sched_rt_period_us=$(cat /proc/sys/kernel/sched_rt_period_us)"
  echo "isolated CPUs (from /sys/devices/system/cpu/isolated): '$(cat /sys/devices/system/cpu/isolated 2>/dev/null)'"; } > machine.out
rec machine "(machine facts read from /proc and /sys)" "$(bash --version | head -n 1)" "cat /proc/sys/kernel/sched_rt_*; cat /sys/devices/system/cpu/isolated" "0"
rm -rf "$B"
exit $status
