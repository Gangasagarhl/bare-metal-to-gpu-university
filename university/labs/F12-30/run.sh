#!/usr/bin/env bash
# F12-30 extra runs: the game-day scenario on the build that acknowledged after one copy
# (exit code 1 is the expected, recorded result: check A1 fails), the same scenario on the
# fixed build, and the revised claims ledger.
set -u
CXX="${CXX:-g++}"
FLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
ver="$($CXX --version | head -n 1)"
$CXX $FLAGS gsdemo.cpp -o .bin_gs || exit 1
$CXX $FLAGS claims.cpp -o .bin_claims || exit 1
for case in gameday gameday_fixed claims_fixed; do
    prog=gsdemo; [ "$case" = claims_fixed ] && prog=claims
    ./.bin_${prog/gsdemo/gs} < "${case}.in" > "${case}.out" 2>&1; rc=$?
    {
        echo "listing:   ${prog}.cpp with ${case}.in"
        echo "toolchain: $ver"
        echo "command:   $CXX $FLAGS ${prog}.cpp -o ${prog} && ./${prog} < ${case}.in"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
        echo "exit code: $rc"
        echo "stdin:     ${case}.in"
    } > "${case}.log"
done
rm -f .bin_gs .bin_claims
exit 0
