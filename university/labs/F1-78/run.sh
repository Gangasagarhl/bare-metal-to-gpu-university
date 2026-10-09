#!/usr/bin/env bash
# F1-78 lab step that run_lab.sh cannot do alone: replay Priya's register writes
# (forensic.in) on the TIM2 model.
set -u -o pipefail
cd "$(dirname "$0")"
status=0
FLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
g++ $FLAGS umcu_tim.cpp -o .bin_forensic || exit 1
timeout 30 ./.bin_forensic < forensic.in > forensic.out 2>&1; rc=$?
{
    echo "listing:   umcu_tim.cpp, input forensic.in"
    echo "toolchain: $(g++ --version | head -n 1)"
    echo "command:   ./umcu_tim < forensic.in"
    echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
    echo "exit code: $rc"
} > forensic.log
[ "$rc" = 0 ] || status=1
rm -f .bin_forensic
exit $status
