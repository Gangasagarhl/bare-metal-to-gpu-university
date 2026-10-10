#!/usr/bin/env bash
# Extra run for F5-27: borgsim.cpp (course flags) on starve.in, the forensic evidence.
set -u
GXX="$(g++ --version | head -n 1)"
CXXFLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
{
    echo "listing:   borgsim.cpp"
    echo "toolchain: $GXX"
    echo "command:   g++ $CXXFLAGS borgsim.cpp -o borgsim; ./borgsim < starve.in"
    echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
} > starve.log
status=0
if g++ $CXXFLAGS borgsim.cpp -o .bin_borg > .build.txt 2>&1; then
    timeout 10 ./.bin_borg < starve.in > starve.out 2>&1; rc=$?
    echo "exit code: $rc" >> starve.log; echo "stdin:     starve.in" >> starve.log; [ "$rc" = 0 ] || status=1
else
    echo "result:    BUILD FAILED" >> starve.log; cat .build.txt >> starve.log; status=1
fi
rm -f .build.txt .bin_borg
exit $status
