#!/usr/bin/env bash
# F2-34: timing run of speedup.cc, built with -O2 and WITHOUT sanitizers (sanitizers distort timing).
set -u
cd "$(dirname "$0")"
name=speedup
cmd="g++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror speedup.cc -o speedup"
{
    echo "listing:   speedup.cc (timing build, no sanitizers)"
    echo "toolchain: $(g++ --version | head -n 1)"
    echo "command:   $cmd && ./speedup"
    echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "machine:   $(uname -s) $(uname -m) (cloud build container), $(nproc) CPUs visible"
} > "$name.log"
if ! g++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror speedup.cc -o .bin_speedup > "$name.build.txt" 2>&1; then
    echo "result:    BUILD FAILED" >> "$name.log"; cat "$name.build.txt" >> "$name.log"; rm -f "$name.build.txt"; exit 1
fi
rm -f "$name.build.txt"
timeout 120 ./.bin_speedup > "$name.out" 2>&1; rc=$?
echo "exit code: $rc" >> "$name.log"
echo "note:      measured on the build container (a shared virtual machine); times change from run to run" >> "$name.log"
rm -f ./.bin_speedup
exit 0
