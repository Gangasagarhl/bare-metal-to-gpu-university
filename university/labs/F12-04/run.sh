#!/usr/bin/env bash
# F12-04 run.sh: evidence for the review lab and the course forensic lab.
#   comment_lint_v2  Listing 1 on the revised review (review_v2.txt)
#   change_diff      the change under review: unified diff of frame.cpp and its tests
#   v2_old_tests     version 2 of the parser run against the tests of version 1
#   incident         version 2 of the parser run on the frames recorded in the field
# Built with the same compiler and flags as run_lab.sh.
set -u
cd "$(dirname "$0")"
FLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
status=0
header() {  # header <name> <listing> <toolchain line> <command>
    {
        echo "listing:   $2 (run by run.sh)"
        echo "toolchain: $3"
        echo "command:   $4"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
    } > "$1.log"
}
step() {  # step <name> <source> <stdin file> <expected exit code>
    local name="$1" src="$2" input="$3" want="$4" bin="./.bin_$1"
    header "$name" "$src" "$(g++ --version | head -n 1)" \
        "g++ $FLAGS $src -o ${src%.cpp}; ./${src%.cpp} < $input"
    if ! g++ $FLAGS "$src" -o "$bin" > "$name.out" 2>&1; then
        echo "result:    BUILD FAILED" >> "$name.log"; status=1; return
    fi
    timeout 10 "$bin" < "$input" > "$name.out" 2>&1
    local rc=$?
    sed -i -e "s#$(pwd)/##g" -e "s#\.bin_$name#${src%.cpp}#g" "$name.out"
    if grep -q "^SUMMARY: AddressSanitizer" "$name.out"; then
        # keep the report up to its SUMMARY line; the shadow-byte map is trimmed (AH-25)
        cp "$name.out" "$name.full.txt"
        awk '{print} /^SUMMARY: AddressSanitizer/{exit}' "$name.full.txt" > "$name.out"
        echo "[…] shadow-byte map and legend trimmed; untrimmed report: $name.full.txt" >> "$name.out"
    fi
    echo "exit code: $rc (expected $want)" >> "$name.log"
    echo "stdin:     $input" >> "$name.log"
    [ "$rc" = "$want" ] || status=1
    rm -f "$bin"
}
step comment_lint_v2 comment_lint.cpp review_v2.txt 0

header change_diff "frame_v1.cpp, frame_v2.cpp, frame_v1.in, frame_v2.in" "$(diff --version | head -n 1)" \
    "diff -u --label a/frame.cpp --label b/frame.cpp frame_v1.cpp frame_v2.cpp; diff -u --label a/frame_tests.in --label b/frame_tests.in frame_v1.in frame_v2.in"
{
    diff -u --label a/frame.cpp --label b/frame.cpp frame_v1.cpp frame_v2.cpp
    diff -u --label a/frame_tests.in --label b/frame_tests.in frame_v1.in frame_v2.in
} > change_diff.out
echo "exit code: $? (diff exits 1 when the files differ, as expected)" >> change_diff.log

step v2_old_tests frame_v2.cpp frame_v1.in 1
step incident frame_v2.cpp incident.in 1
exit $status
