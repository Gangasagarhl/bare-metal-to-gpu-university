#!/usr/bin/env bash
# F9-68 run.sh: the start-up simulator on three more unit files (a failed camera, a dependency
# cycle, the forensic evidence of release 5, and both releases with a failed motor bridge). Listings 1 and 2 with startup.in run through
# run_lab.sh itself.
set -u -o pipefail
cd "$(dirname "$0")"
. ../F9-67/rblib.sh
B=.build
status=0
rm -rf "$B"; mkdir -p "$B"
g++ $HOSTFLAGS startup.cpp -o $B/startup || exit 1
one() {   # one NAME INPUT EXPECTED_EXIT NOTE
    timeout 10 $B/startup < "$2" > "$1.out" 2>&1; local rc=$?
    rec "$1" "startup.cpp" "$GXX_VER" "g++ $HOSTFLAGS startup.cpp -o startup; ./startup < $2" "$rc" "stdin:     $2" "$4"
    [ "$rc" = "$3" ] || { echo "F9-68 $1: exit $rc, expected $3" >&2; status=1; }
}
one startup_camfail startup_camfail.in 0 "note:      the camera unit is made to fail"
one startup_cycle startup_cycle.in 1 "note:      exit code 1 = a dependency cycle was found (expected)"
one forensic_startup startup_forensic.in 0 "note:      the unit file of release 5 (forensic evidence)"
# the same two unit files with the motor bridge made to fail (one sed edit, shown in the log)
sed '/^unit motor_bridge/s/$/ fail/' startup.in > $B/release4_bridgefail.in
sed '/^unit motor_bridge/s/$/ fail/' startup_forensic.in > $B/release5_bridgefail.in
one bridgefail_r4 $B/release4_bridgefail.in 1 "note:      startup.in with ' fail' appended to the motor_bridge line by sed; exit 1 = robot_ready never ready (expected)"
one bridgefail_r5 $B/release5_bridgefail.in 0 "note:      startup_forensic.in with ' fail' appended to the motor_bridge line by sed (forensic answer key)"
rm -rf "$B"
exit $status
