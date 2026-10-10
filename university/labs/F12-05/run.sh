#!/usr/bin/env bash
# F12-05 run.sh: Listing 1 (ledger.cpp) on the second round of the review thread,
# and the forensic evidence: the ledger of the dev.cpp review, the diff of the
# "fixed" commit, and the version after that commit run with the device removed.
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
step() {  # step <name> <source> <stdin file> <expected exit code> <time limit in s>
    local name="$1" src="$2" input="$3" want="$4" limit="$5" bin="./.bin_$1"
    header "$name" "$src" "$(g++ --version | head -n 1)" \
        "g++ $FLAGS $src -o ${src%.cpp}; timeout $limit ./${src%.cpp} < $input"
    if ! g++ $FLAGS "$src" -o "$bin" > "$name.out" 2>&1; then
        echo "result:    BUILD FAILED" >> "$name.log"; status=1; return
    fi
    timeout "$limit" "$bin" < "$input" > "$name.out" 2>&1
    local rc=$?
    if [ "$rc" = 124 ]; then
        echo "exit code: 124 (stopped by the ${limit} s time limit; expected $want)" >> "$name.log"
    else
        echo "exit code: $rc (expected $want)" >> "$name.log"
    fi
    echo "stdin:     $input" >> "$name.log"
    [ "$rc" = "$want" ] || status=1
    rm -f "$bin"
}
step ledger_v2 ledger.cpp thread_v2.txt 0 10
step ledger_forensic ledger.cpp thread_forensic.txt 0 10

header dev_diff "dev_v2.cpp, dev_v3.cpp" "$(diff --version | head -n 1)" \
    "diff -u --label a/dev.cpp --label b/dev.cpp dev_v2.cpp dev_v3.cpp"
diff -u --label a/dev.cpp --label b/dev.cpp dev_v2.cpp dev_v3.cpp > dev_diff.out
echo "exit code: $? (diff exits 1 when the files differ, as expected)" >> dev_diff.log

step unplug dev_v3.cpp unplug.in 124 3
exit $status
