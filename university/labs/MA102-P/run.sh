#!/bin/bash
# Runs the reference solution on one more input (the runner only feeds status_exam.in to status_exam.cpp).
# Writes status_case2.out/.log for the packet "AB 00": the byte candidates encode by hand in Part A, item 4.
set -u
CXX="${CXX:-g++}"
CXXFLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
status=0
$CXX $CXXFLAGS status_exam.cpp -o ./.bin_ref || exit 1
for case in "status_case2:AB 00"; do
    name="${case%%:*}"; input="${case#*:}"
    echo "$input" > "${name}.in"
    {
        echo "listing:   status_exam.cpp (run on ${name}.in)"
        echo "toolchain: $($CXX --version | head -n 1)"
        echo "command:   $CXX $CXXFLAGS status_exam.cpp -o status_exam; ./status_exam < ${name}.in"
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
