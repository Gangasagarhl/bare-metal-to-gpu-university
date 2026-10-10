#!/usr/bin/env bash
# F12-18 run.sh: Listing 2 (pm_lint.cpp) on the revised post-mortem (pm_revised.txt).
# Built with the same compiler and flags as run_lab.sh.
set -u
cd "$(dirname "$0")"
FLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
status=0
name=pm_lint_revised
{
    echo "listing:   pm_lint.cpp (run by run.sh)"
    echo "toolchain: $(g++ --version | head -n 1)"
    echo "command:   g++ $FLAGS pm_lint.cpp -o pm_lint; ./pm_lint < pm_revised.txt"
    echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
} > "$name.log"
g++ $FLAGS pm_lint.cpp -o ./.bin_pm || { echo "result:    BUILD FAILED" >> "$name.log"; exit 1; }
timeout 10 ./.bin_pm < pm_revised.txt > "$name.out" 2>&1
rc=$?
echo "exit code: $rc" >> "$name.log"
echo "stdin:     pm_revised.txt" >> "$name.log"
[ "$rc" = 0 ] || status=1
rm -f ./.bin_pm
exit $status
