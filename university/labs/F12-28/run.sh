#!/usr/bin/env bash
# F12-28 extra runs: Listing 1 on the forensic plans (the default run of mp8plan.cpp
# with mp8plan.in is done by run_lab.sh itself).
set -u
CXX="${CXX:-g++}"
FLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
ver="$($CXX --version | head -n 1)"
$CXX $FLAGS mp8plan.cpp -o .bin_plan || exit 1
status=0
for case in mp8plan_team mp8plan_fixed; do
    ./.bin_plan < "${case}.in" > "${case}.out" 2>&1; rc=$?
    {
        echo "listing:   mp8plan.cpp with ${case}.in"
        echo "toolchain: $ver"
        echo "command:   $CXX $FLAGS mp8plan.cpp -o mp8plan && ./mp8plan < ${case}.in"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
        echo "exit code: $rc"
        echo "stdin:     ${case}.in"
    } > "${case}.log"
done
rm -f .bin_plan
exit $status
