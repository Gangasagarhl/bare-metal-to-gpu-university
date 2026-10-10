#!/usr/bin/env bash
# F10-31 lab steps: generate the U-link header from F10-30's dialect, build the mini-SITL
# (vehicle_sim.cc) and the companion program (companion.cc), run them as two processes that
# talk over UDP on the loopback interface, then build and run the forensic checker on the
# evidence that lostlink_gen.cpp printed (run_lab.sh ran it before this script).
set -u -o pipefail
cd "$(dirname "$0")"
status=0
B=.build
rm -rf "$B"; mkdir -p "$B"
rec() {  # rec <name> <listing> <toolchain> <command> <exit code> [extra line]
    {
        echo "listing:   $2"
        echo "toolchain: $3"
        echo "command:   $4"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
        echo "exit code: $5"
        if [ $# -ge 6 ]; then echo "$6"; fi
    } > "$1.log"
}
GXX="$(g++ --version | head -n 1); $(python3 --version 2>&1)"
CXXFLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
NOTE="note:      mini-SITL and U-link are the university's own; no ArduPilot SITL, MAVLink library or ground station was used; timings vary by about 0.1 s between runs"

python3 ../F10-30/gen_dialect.py ../F10-30/udialect.xml $B/udialect.h > /dev/null || status=1
g++ $CXXFLAGS -I$B -I../F10-30 vehicle_sim.cc -o $B/vehicle_sim > $B/build_v.txt 2>&1 || { cat $B/build_v.txt; status=1; }
g++ $CXXFLAGS -I$B -I../F10-30 companion.cc -o $B/companion > $B/build_c.txt 2>&1 || { cat $B/build_c.txt; status=1; }

# start the vehicle, wait for its port file, run the companion, wait for the vehicle to end
timeout 40 $B/vehicle_sim $B/port.txt 30000 > vehicle.out 2>&1 &
vpid=$!
for i in $(seq 1 50); do [ -s $B/port.txt ] && break; sleep 0.1; done
timeout 40 $B/companion $B/port.txt > companion.out 2>&1; crc=$?
wait $vpid; vrc=$?
rec vehicle "vehicle_sim.cc udp.hpp + ../F10-30 ulink.hpp mission.hpp + generated udialect.h" "$GXX" \
    "python3 ../F10-30/gen_dialect.py ../F10-30/udialect.xml udialect.h; g++ $CXXFLAGS -I. -I../F10-30 vehicle_sim.cc -o vehicle_sim; ./vehicle_sim port.txt 30000 &" "$vrc" "$NOTE"
rec companion "companion.cc udp.hpp + ../F10-30 ulink.hpp mission.hpp + generated udialect.h" "$GXX" \
    "g++ $CXXFLAGS -I. -I../F10-30 companion.cc -o companion; ./companion port.txt" "$crc" "$NOTE"
[ "$vrc" = 0 ] && [ "$crc" = 0 ] || status=1

# forensic answer-key tool: reconstruct the failsafe from the evidence
if [ -f lostlink_gen.out ]; then
    { g++ $CXXFLAGS lostlink_check.cc -o $B/check && $B/check lostlink_gen.out; } > lostlink_check.out 2>&1; rc=$?
else
    echo "lostlink_gen.out missing: run run_lab.sh, which runs lostlink_gen.cpp first" > lostlink_check.out; rc=1
fi
rec lostlink_check "lostlink_check.cc on lostlink_gen.out" "$(g++ --version | head -n 1)" "g++ $CXXFLAGS lostlink_check.cc -o check && ./check lostlink_gen.out" "$rc"
[ "$rc" = 0 ] || status=1
rm -rf "$B"
exit $status
