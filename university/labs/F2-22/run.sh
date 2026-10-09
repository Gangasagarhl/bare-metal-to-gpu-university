#!/usr/bin/env bash
# Trimmed copies of two long sanitizer reports for the chapter page (AH-25: trims are marked).
# The full, untrimmed outputs stay in cycle.out and deep_list.out.
set -u
ver="$(g++ --version | head -n 1)"
trim() {   # trim <source> <target> <regex of lines to keep>
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
trim cycle.out cycle_trimmed '^owners|^end|ERROR|leak of|in main |SUMMARY'
trim deep_list.out deep_list_trimmed '^built|^done|DEADLYSIGNAL|ERROR|#[0-9] .*Node::~Node|SUMMARY|ABORTING'
exit 0
