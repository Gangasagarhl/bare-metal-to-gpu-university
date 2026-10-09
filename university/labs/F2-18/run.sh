#!/usr/bin/env bash
# Second build of Listing 3 (addr_lab.cpp) WITHOUT sanitizers, to compare the layout of locals.
set -u
name=addr_lab_plain
ver="$(g++ --version | head -n 1)"
cmd="g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -O0 addr_lab.cpp -o addr_lab_plain"
{
    echo "listing:   addr_lab.cpp (built without sanitizers)"
    echo "toolchain: $ver"
    echo "command:   $cmd"
    echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
} > "$name.log"
if ! $cmd > "$name.build.txt" 2>&1; then
    echo "result:    BUILD FAILED" >> "$name.log"; cat "$name.build.txt" >> "$name.log"; rm -f "$name.build.txt"; exit 1
fi
rm -f "$name.build.txt"
./addr_lab_plain > "$name.out" 2>&1; rc=$?
echo "exit code: $rc" >> "$name.log"
rm -f ./addr_lab_plain
exit 0
