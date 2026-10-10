#!/bin/bash
# Runs the reference solution on two more inputs (the runner only feeds helpers_exam.in to helpers_exam.cpp).
# Writes helpers_case2.out/.log (320 32: an exact fit) and helpers_case3.out/.log (321 32: one job more).
set -u
CXX="${CXX:-g++}"
CXXFLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
status=0
$CXX $CXXFLAGS helpers_exam.cpp -o ./.bin_ref || exit 1
for case in "helpers_case2:320 32" "helpers_case3:321 32"; do
    name="${case%%:*}"; input="${case#*:}"
    echo "$input" > "${name}.in"
    {
        echo "listing:   helpers_exam.cpp (run on ${name}.in)"
        echo "toolchain: $($CXX --version | head -n 1)"
        echo "command:   $CXX $CXXFLAGS helpers_exam.cpp -o helpers_exam; ./helpers_exam < ${name}.in"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
    } > "${name}.log"
    timeout 10 ./.bin_ref < "${name}.in" > "${name}.out" 2>&1; rc=$?
    echo "exit code: $rc" >> "${name}.log"
    echo "stdin:     ${name}.in" >> "${name}.log"
    [ "$rc" = 0 ] || status=1
done
rm -f ./.bin_ref
exit $status
