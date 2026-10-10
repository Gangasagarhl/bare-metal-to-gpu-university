#!/usr/bin/env bash
# F12-24 run.sh: Listing 1 (uncertain.cpp) on the forensic team's numbers (rewrite.in)
# and on a deliberately broken input whose probabilities do not add up to 1;
# also on accuracy06.in (check-yourself question 7).
set -u
cd "$(dirname "$0")"
FLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
status=0
step() {  # step <name> <stdin file> <expected exit code>
    local name="$1" input="$2" want="$3"
    {
        echo "listing:   uncertain.cpp (run by run.sh)"
        echo "toolchain: $(g++ --version | head -n 1)"
        echo "command:   g++ $FLAGS uncertain.cpp -o uncertain; ./uncertain < $input"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
    } > "$name.log"
    if ! g++ $FLAGS uncertain.cpp -o ./.bin_u > "$name.out" 2>&1; then
        echo "result:    BUILD FAILED" >> "$name.log"; status=1; return
    fi
    timeout 10 ./.bin_u < "$input" > "$name.out" 2>&1
    local rc=$?
    echo "exit code: $rc (expected $want)" >> "$name.log"
    echo "stdin:     $input" >> "$name.log"
    [ "$rc" = "$want" ] || status=1
    rm -f ./.bin_u
}
step rewrite rewrite.in 0
step bad_probs bad_probs.in 2
step accuracy06 accuracy06.in 0
exit $status
