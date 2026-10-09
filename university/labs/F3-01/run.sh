#!/usr/bin/env bash
# Extra runs for F3-01: watch a process talk to the kernel with strace.
set -u
gxx="$(g++ --version | head -n 1)"
st="$(strace -V | head -n 1)"
header() {   # header <name> <listing> <command> [toolchain]
    {
        echo "listing:   $2"
        echo "toolchain: ${4:-$gxx}"
        echo "command:   $3"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
    } > "$1.log"
}
flags="-std=c++20 -Wall -Wextra -Wpedantic -Werror -O2"
# Plain builds (no sanitizers): sanitizers add system calls of their own.
g++ $flags hello.cpp -o hello_plain || exit 1
g++ $flags ask_name.cpp -o ask_name_plain || exit 1

# 1. every system call of hello, in order
header hello_strace "hello.cpp (plain build) under strace" "strace ./hello_plain > /dev/null" "$st; $gxx"
strace -o hello_strace.out ./hello_plain > /dev/null; echo "exit code: $?" >> hello_strace.log

# 2. the same run, counted per system call
header hello_count "hello.cpp (plain build) under strace -c" "strace -c -o hello_count.out ./hello_plain > /dev/null" "$st; $gxx"
strace -c -o hello_count.out ./hello_plain > /dev/null; echo "exit code: $?" >> hello_count.log

# 3. forensic lab: the tool started by a script whose input never arrives
header frozen "ask_name.cpp (plain build) under strace, input pipe kept open, 2 s time limit" \
    "(sleep 5) | timeout 2 strace -o frozen.out ./ask_name_plain" "$st; $gxx"
(sleep 5) | timeout 2 strace -o frozen.out ./ask_name_plain > frozen_stdout.txt; rc=${PIPESTATUS[1]}
echo "exit code: $rc (124 = stopped by the 2 s time limit)" >> frozen.log
echo "stdout:    $(cat frozen_stdout.txt)" >> frozen.log
# a trimmed copy for the chapter page (AH-25: trimming is marked)
header frozen_tail "frozen.out from the run above, trimmed" "head -n 1 frozen.out; a marker line; tail -n 7 frozen.out"
total=$(wc -l < frozen.out)
{ head -n 1 frozen.out; echo "[... $((total - 8)) line(s) trimmed: loading the C++ and C libraries ...]"; tail -n 7 frozen.out; } > frozen_tail.out
echo "exit code: 0 (of the trimming step; frozen.out had $total lines)" >> frozen_tail.log
rm -f frozen_stdout.txt hello_plain ask_name_plain
exit 0
