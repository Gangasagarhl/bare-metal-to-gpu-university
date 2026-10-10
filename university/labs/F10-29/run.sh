#!/usr/bin/env bash
# F10-29 lab steps: run the board generator on rev A, build and run the firmware-side code
# against the generated header, show the generator's errors for a mistyped board file, then
# (forensic) run the generator and the same code for rev B. flash_layout.cpp is built and run
# by run_lab.sh itself.
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
HW="hardware:  untested on hardware; U-MCU1 and U-FC1 are pretend parts, nothing was flashed"

# 1. rev A: check and generate
python3 gen_board.py umcu1.txt uboard.txt $B/board_gen.h > gen.out 2>&1; rc=$?
sed -i "s#$B/##g" gen.out
rec gen "gen_board.py umcu1.txt uboard.txt" "$PY" "python3 gen_board.py umcu1.txt uboard.txt board_gen.h" "$rc"
[ "$rc" = 0 ] || status=1
sed "s#$B/##g" $B/board_gen.h > board_gen.h.txt     # kept so the chapter can show it

# 2. firmware-side code against the generated header
{ g++ $CXXFLAGS -I$B board_use.cc -o $B/board_use && $B/board_use; } > board_use.out 2>&1; rc=$?
rec board_use "board_use.cc + generated board_gen.h" "$GXX" "g++ $CXXFLAGS -I. board_use.cc -o board_use && ./board_use" "$rc" "$HW"
[ "$rc" = 0 ] || status=1

# 3. a mistyped board file: the generator must refuse it
python3 gen_board.py umcu1.txt uboard_bad.txt $B/bad.h > gen_bad.out 2>&1; rc=$?
sed -i "s#$B/##g" gen_bad.out
rec gen_bad "gen_board.py umcu1.txt uboard_bad.txt" "$PY" "python3 gen_board.py umcu1.txt uboard_bad.txt bad.h" "$rc (1 = errors found; expected)"
[ "$rc" = 1 ] || status=1

# 4. forensic: rev B
python3 gen_board.py umcu1.txt uboard_revb.txt $B/board_gen.h > forensic_gen.out 2>&1; rc=$?
sed -i "s#$B/##g" forensic_gen.out
rec forensic_gen "gen_board.py umcu1.txt uboard_revb.txt" "$PY" "python3 gen_board.py umcu1.txt uboard_revb.txt board_gen.h" "$rc"
[ "$rc" = 0 ] || status=1
{ g++ $CXXFLAGS -I$B board_use.cc -o $B/board_use_b && $B/board_use_b; } > forensic_use.out 2>&1; rc=$?
rec forensic_use "board_use.cc + board_gen.h generated from uboard_revb.txt" "$GXX" "g++ $CXXFLAGS -I. board_use.cc -o board_use && ./board_use" "$rc" "$HW"
[ "$rc" = 0 ] || status=1
rm -rf "$B"
exit $status
