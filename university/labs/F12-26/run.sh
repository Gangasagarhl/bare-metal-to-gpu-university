#!/usr/bin/env bash
# F12-26 run.sh: (1) Listing 1 (status_lint.cpp) on the rewritten update;
# (2) Listing 2 (watermelon.cpp) on the forensic status history.
set -u
cd "$(dirname "$0")"
FLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
status=0
step() {  # step <name> <source> <stdin file>
    local name="$1" src="$2" input="$3"
    {
        echo "listing:   $src (run by run.sh)"
        echo "toolchain: $(g++ --version | head -n 1)"
        echo "command:   g++ $FLAGS $src -o ${src%.cpp}; ./${src%.cpp} < $input"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
    } > "$name.log"
    if ! g++ $FLAGS "$src" -o ./.bin_s > "$name.out" 2>&1; then
        echo "result:    BUILD FAILED" >> "$name.log"; status=1; return
    fi
    timeout 10 ./.bin_s < "$input" > "$name.out" 2>&1
    local rc=$?
    echo "exit code: $rc" >> "$name.log"
    echo "stdin:     $input" >> "$name.log"
    [ "$rc" = 0 ] || status=1
    rm -f ./.bin_s
}
step status_rewrite status_lint.cpp status_rewrite.in
step watermelon_hl status_lint.cpp watermelon_headlines.txt
exit $status
