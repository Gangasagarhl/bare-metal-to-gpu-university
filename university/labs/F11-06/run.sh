#!/usr/bin/env bash
# F11-06 lab: the chain of trust of a three-processor robot, modelled on the host (no robot,
# drone, flight controller or motor driver is available in this build).
#   robot      six design scenarios
#   forensic   the evidence pack of the forensic lab
set -u -o pipefail
cd "$(dirname "$0")"
status=0
B=.build
rm -rf "$B"; mkdir -p "$B" keys
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
FLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
TC="$(g++ --version | head -n 1); $(openssl version)"
NOTE="hardware:  untested on hardware; a host model of a robot's processors, no robot or drone"
g++ $FLAGS robot.cc -lcrypto -o $B/robot > $B/build.txt 2>&1 || { cat $B/build.txt; status=1; }
$B/robot demo > robot.out 2>&1; rc=$?
rec robot "fwimage.h (from F11-04) robot.cc" "$TC" "g++ $FLAGS robot.cc -lcrypto -o robot && ./robot demo" "$rc" "$NOTE"
[ "$rc" = 0 ] || status=1
$B/robot forensic > forensic.out 2>&1; rc=$?
rec forensic "fwimage.h (from F11-04) robot.cc" "$TC" "./robot forensic" "$rc" "$NOTE"
[ "$rc" = 0 ] || status=1
rm -rf "$B"
exit $status
