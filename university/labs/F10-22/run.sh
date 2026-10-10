#!/usr/bin/env bash
# F10-22 extra step: build the threaded test with ThreadSanitizer instead of
# AddressSanitizer, run it, and record the result in the run_lab.sh format.
set -u
cd "$(dirname "$0")"
name=uorb_threads_tsan
cmd="g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=thread uorb_threads.cpp -o $name"
{
    echo "listing:   uorb_threads.cpp (ThreadSanitizer build)"
    echo "toolchain: $(g++ --version | head -n 1)"
    echo "command:   $cmd"
    echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
} > "$name.log"
if ! $cmd > "$name.out" 2>&1; then
    echo "result:    BUILD FAILED" >> "$name.log"; exit 1
fi
timeout 60 "./$name" >> "$name.out" 2>&1; rc=$?
echo "exit code: $rc" >> "$name.log"
rm -f "./$name"
[ "$rc" = 0 ]
