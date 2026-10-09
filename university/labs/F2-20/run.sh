#!/usr/bin/env bash
# Extra runs for F2-20: Listing 1 without sanitizers, and the stack limit of this container.
set -u
ver="$(g++ --version | head -n 1)"
header() {
    {
        echo "listing:   $2"
        echo "toolchain: $ver"
        echo "command:   $3"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
    } > "$1.log"
}
# 1. frames.cpp without sanitizers: the real stack, not AddressSanitizer's
cmd="g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -O0 frames.cpp -o frames_plain"
header frames_plain "frames.cpp (built without sanitizers)" "$cmd"
if ! $cmd > frames_plain.out 2>&1; then echo "result:    BUILD FAILED" >> frames_plain.log; exit 1; fi
./frames_plain > frames_plain.out 2>&1; echo "exit code: $?" >> frames_plain.log
rm -f ./frames_plain
# 2. the stack size limit the shell gives programs in this container
header stack_limit "shell built-in ulimit" "ulimit -s   (prints the limit in KiB)"
echo "stack limit (KiB): $(ulimit -s)" > stack_limit.out; echo "exit code: 0" >> stack_limit.log
# 3. a trimmed copy of deep.out for the chapter page (AH-25: trimming is marked)
header deep_trimmed "deep.cpp (output of the run above, trimmed)" "head -n 12 deep.out; a marker line; tail -n 4 deep.out"
total=$(wc -l < deep.out)
{ head -n 12 deep.out; echo "[... $((total - 16)) line(s) trimmed: more stack frames of the same report ...]"; tail -n 4 deep.out; } > deep_trimmed.out
echo "exit code: 0 (of the trimming step; deep.out had $total lines; deep.cpp's own exit code is in deep.log)" >> deep_trimmed.log
exit 0
