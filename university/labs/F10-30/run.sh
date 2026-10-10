#!/usr/bin/env bash
# F10-30 lab steps: generate the C++ header from the dialect XML, then build and run the
# frame demo, the mission upload and the forensic variant (two different dialect files).
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
PY="$(python3 --version 2>&1)"
GXX="$(g++ --version | head -n 1)"
CXXFLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
NOTE="note:      U-link is the university's teaching protocol, not MAVLink; no MAVLink library, autopilot or SITL was used"

python3 gen_dialect.py udialect.xml $B/udialect.h > gen.out 2>&1; rc=$?
rec gen "gen_dialect.py udialect.xml" "$PY" "python3 gen_dialect.py udialect.xml udialect.h" "$rc"
[ "$rc" = 0 ] || status=1
sed -n '/^struct UMissionItem {/,/^};/p' $B/udialect.h > udialect_item.h.txt   # shown in the chapter

for prog in frame_demo mission_upload; do
    { g++ $CXXFLAGS -I$B $prog.cc -o $B/$prog && $B/$prog; } > $prog.out 2>&1; rc=$?
    rec $prog "$prog.cc ulink.hpp mission.hpp + generated udialect.h" "$GXX" \
        "g++ $CXXFLAGS -I. $prog.cc -o $prog && ./$prog" "$rc" "$NOTE"
    [ "$rc" = 0 ] || status=1
done

python3 gen_dialect.py udialect_gcs.xml $B/gcsdialect.h gcsdialect > forensic_gen.out 2>&1; rc=$?
rec forensic_gen "gen_dialect.py udialect_gcs.xml" "$PY" "python3 gen_dialect.py udialect_gcs.xml gcsdialect.h gcsdialect" "$rc"
[ "$rc" = 0 ] || status=1
{ g++ $CXXFLAGS -I$B forensic_mission.cc -o $B/forensic && $B/forensic; } > forensic.out 2>&1; rc=$?
rec forensic "forensic_mission.cc + udialect.h + gcsdialect.h" "$GXX" "g++ $CXXFLAGS -I. forensic_mission.cc -o forensic && ./forensic" "$rc" "$NOTE"
[ "$rc" = 0 ] || status=1
sed -i "s#$B/##g" gen.out forensic_gen.out
rm -rf "$B"
exit $status
