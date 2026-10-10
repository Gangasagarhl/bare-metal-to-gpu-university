#!/usr/bin/env bash
# F12-27 run.sh: Listing 1 (claim_lint.cpp) on the worked example's release notes
# (draft and final) and on the forensic answer key's corrected report (claim_fixed.in).
set -u
cd "$(dirname "$0")"
FLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
status=0
step() {  # step <name> <stdin file>
    local name="$1" input="$2"
    {
        echo "listing:   claim_lint.cpp (run by run.sh)"
        echo "toolchain: $(g++ --version | head -n 1)"
        echo "command:   g++ $FLAGS claim_lint.cpp -o claim_lint; ./claim_lint < $input"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
    } > "$name.log"
    if ! g++ $FLAGS claim_lint.cpp -o ./.bin_cl > "$name.out" 2>&1; then
        echo "result:    BUILD FAILED" >> "$name.log"; status=1; return
    fi
    timeout 10 ./.bin_cl < "$input" > "$name.out" 2>&1
    local rc=$?
    echo "exit code: $rc" >> "$name.log"
    echo "stdin:     $input" >> "$name.log"
    [ "$rc" = 0 ] || status=1
    rm -f ./.bin_cl
}
step release_draft release_draft.in
step release_final release_final.in
step claim_fixed claim_fixed.in
exit $status
