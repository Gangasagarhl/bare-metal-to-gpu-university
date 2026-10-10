#!/usr/bin/env bash
# F12-07 run.sh - processor-in-the-loop (PIL) under emulation: the same controller source is
# built for this machine and for AArch64; each build talks to the simulated world (plant_main)
# through two named pipes. The two command logs must be identical.
# Every step writes <name>.out (real output) and <name>.log (run record).
set -u -o pipefail
cd "$(dirname "$0")"
status=0
B=.build
rm -rf "$B"; mkdir -p "$B"
GXX="g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -O2"
AXX="aarch64-linux-gnu-g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -O2 -static"
GXXV="$(g++ --version | head -n 1)"
AXXV="$(aarch64-linux-gnu-g++ --version | head -n 1)"
QEMUV="$(qemu-aarch64 --version | head -n 1)"
rec() {  # rec <name> <listing> <toolchain> <command> <exit code text> [extra line]
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

$GXX plant_main.cc -o $B/plant && $GXX controller_main.cc -o $B/ctl_x86 &&
    $AXX controller_main.cc -o $B/ctl_a64 || { echo "build failed"; exit 1; }

# 1. what the target binary is
readelf -h $B/ctl_a64 | grep -E 'Class|Machine' > pil_target.out 2>&1; rc=$?
rec pil_target "controller_main.cc" "$AXXV; $(readelf --version | head -n 1)" \
    "$AXX controller_main.cc -o ctl_a64; readelf -h ctl_a64 | grep -E 'Class|Machine'" "$rc"

# 2. one closed-loop run per controller build, through named pipes, with a time limit
loop() {  # loop <out name> <controller command...>
    local out=$1; shift
    rm -f $B/down $B/up; mkfifo $B/down $B/up
    timeout 60 "$@" < $B/down > $B/up &
    local cpid=$!
    timeout 60 $B/plant $B/down $B/up > "$out.out" 2>&1; local prc=$?
    wait $cpid; local crc=$?
    echo "$prc $crc"
}
read prc crc < <(loop pil_x86 $B/ctl_x86)
rec pil_x86 "plant_main.cc wallsim.hpp controller_main.cc ../F12-06/speedctl.hpp" "$GXXV" \
    "$GXX plant_main.cc -o plant; $GXX controller_main.cc -o ctl_x86; mkfifo down up; timeout 60 ./ctl_x86 < down > up & timeout 60 ./plant down up" \
    "$prc (plant); controller exit code $crc"
[ "$prc" = 0 ] && [ "$crc" = 0 ] || status=1
read prc crc < <(loop pil_aarch64 qemu-aarch64 $B/ctl_a64)
rec pil_aarch64 "plant_main.cc wallsim.hpp controller_main.cc ../F12-06/speedctl.hpp" "$GXXV; $AXXV; $QEMUV" \
    "$GXX plant_main.cc -o plant; $AXX controller_main.cc -o ctl_a64; mkfifo down up; timeout 60 qemu-aarch64 ./ctl_a64 < down > up & timeout 60 ./plant down up" \
    "$prc (plant); controller exit code $crc" \
    "hardware:  untested on hardware; the AArch64 controller ran under QEMU user-mode emulation on x86-64, not on a robot computer"
[ "$prc" = 0 ] && [ "$crc" = 0 ] || status=1

# 3. the two runs must agree line by line
if diff pil_x86.out pil_aarch64.out > pil_compare.out 2>&1; then
    echo "pil_x86.out and pil_aarch64.out are identical ($(wc -l < pil_x86.out) lines)" > pil_compare.out
    rc=0
else
    rc=1; status=1
fi
rec pil_compare "(no listing: diff of the two logs)" "$(diff --version | head -n 1)" \
    "diff pil_x86.out pil_aarch64.out" "$rc"

rm -rf "$B"
exit $status
