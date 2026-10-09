#!/usr/bin/env bash
# Forensic evidence for F2-25: the off-by-one program built WITHOUT sanitizers, as a team
# might ship it. Its output is whatever the byte after the vector happened to hold.
set -u
name=off_by_one_plain
cmd="g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -O2 off_by_one.cpp -o $name"
{
    echo "listing:   off_by_one.cpp (built without sanitizers)"
    echo "toolchain: $(g++ --version | head -n 1)"
    echo "command:   $cmd"
    echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
} > "$name.log"
if ! $cmd > "$name.out" 2>&1; then echo "result:    BUILD FAILED" >> "$name.log"; cat "$name.out" >> "$name.log"; exit 1; fi
for i in 1 2 3; do echo "run $i: $(./$name 2>&1)"; done > "$name.out"
echo "exit code: $? (three runs, one after another)" >> "$name.log"
rm -f "./$name"
ver="$(g++ --version | head -n 1)"
trim() {   # trim <source> <target> <regex of lines to keep>  (AH-25: trims are marked)
    local src="$1" dst="$2" keep="$3" total
    total=$(wc -l < "$src")
    awk -v re="$keep" '
        $0 ~ re { if (skipped > 0) print "[... " skipped " line(s) trimmed ...]"; skipped = 0; print; next }
        { skipped++ }
        END { if (skipped > 0) print "[... " skipped " line(s) trimmed ...]" }' "$src" > "$dst.out"
    {
        echo "listing:   ${src%.out}.cpp (output of the run_lab.sh run, trimmed)"
        echo "toolchain: $ver"
        echo "command:   trim of $src keeping lines that match: $keep"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
        echo "exit code: 0 (of the trimming step; $src had $total lines; the program's own exit code is in ${src%.out}.log)"
    } > "$dst.log"
}
trim stack_overrun.out stack_overrun_trimmed 'runtime error|ERROR|^WRITE|in main |located in stack|overflows this|SUMMARY'
trim off_by_one.out off_by_one_trimmed 'ERROR|^READ|in main |located|SUMMARY'
exit 0
