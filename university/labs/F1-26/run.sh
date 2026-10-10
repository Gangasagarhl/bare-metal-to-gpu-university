#!/usr/bin/env bash
# F1-26: the pipeline model on the sum program of F1-23, in the ideal mode.
set -u -o pipefail
cd "$(dirname "$0")"
FLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
g++ $FLAGS pipeline.cpp -o .bin_pipe || exit 1
./.bin_pipe ideal 12 < sum.s > sum_ideal.out 2>&1; rc=$?
{
    echo "listing:   sum.s (run with pipeline.cpp)"
    echo "toolchain: $(g++ --version | head -n 1)"
    echo "command:   g++ $FLAGS pipeline.cpp -o pipeline && ./pipeline ideal 12 < sum.s"
    echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
    echo "exit code: $rc"
} > sum_ideal.log
rm -f .bin_pipe
exit $rc
