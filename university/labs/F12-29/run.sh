#!/usr/bin/env bash
# F12-29 extra runs: Listing 1 on the forensic gate records (the default run with
# gatecheck.in is done by run_lab.sh itself). Exit code 1 = outcome NOT YET, by design.
set -u
CXX="${CXX:-g++}"
FLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
ver="$($CXX --version | head -n 1)"
$CXX $FLAGS gatecheck.cpp -o .bin_gate || exit 1
for case in gatecheck_r3team gatecheck_r3after; do
    ./.bin_gate < "${case}.in" > "${case}.out" 2>&1; rc=$?
    {
        echo "listing:   gatecheck.cpp with ${case}.in"
        echo "toolchain: $ver"
        echo "command:   $CXX $FLAGS gatecheck.cpp -o gatecheck && ./gatecheck < ${case}.in"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
        echo "exit code: $rc"
        echo "stdin:     ${case}.in"
    } > "${case}.log"
done
rm -f .bin_gate
exit 0
