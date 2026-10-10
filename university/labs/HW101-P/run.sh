#!/bin/bash
# Runs the practical's reference solution on a second input (the runner only feeds bench_exam.in to
# bench_exam.cpp). Writes bench_case2.out/.log: 9 V supply, target 15 mA (a marking variant).
set -u
CXX="${CXX:-g++}"
CXXFLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
status=0
$CXX $CXXFLAGS bench_exam.cpp -o ./.bin_ref || exit 1
name="bench_case2"
printf '9.0 2.0 20 15\n220 330 390 470 560 680 0\n22000 220e-6 4.5\n1000000 10.0\n' > "${name}.in"
{
    echo "listing:   bench_exam.cpp (run on ${name}.in)"
    echo "toolchain: $($CXX --version | head -n 1)"
    echo "command:   $CXX $CXXFLAGS bench_exam.cpp -o bench_exam; ./bench_exam < ${name}.in"
    echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
} > "${name}.log"
timeout 10 ./.bin_ref < "${name}.in" > "${name}.out" 2>&1; rc=$?
echo "exit code: $rc" >> "${name}.log"
echo "stdin:     ${name}.in" >> "${name}.log"
[ "$rc" = 0 ] || status=1
rm -f ./.bin_ref
exit $status
