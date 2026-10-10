#!/usr/bin/env bash
# F11-21 run.sh: the fixed design of the forensic case: Listing 3 (estop_timeline.cpp) with the e-stop read by the control thread (estop_fixed.in).
# Built with the same compiler and flags as run_lab.sh.
set -u
cd "$(dirname "$0")"
FLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
status=0
step() {  # step <name> <source> <stdin file>
    local name="$1" src="$2" input="$3" bin="./.bin_$1"
    {
        echo "listing:   $src (run by run.sh)"
        echo "toolchain: $(g++ --version | head -n 1)"
        echo "command:   g++ $FLAGS $src -o ${src%.cpp}; ./${src%.cpp} < $input"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
    } > "$name.log"
    if ! g++ $FLAGS "$src" -o "$bin" > "$name.out" 2>&1; then
        echo "result:    BUILD FAILED" >> "$name.log"; status=1; return
    fi
    timeout 10 "$bin" < "$input" > "$name.out" 2>&1
    local rc=$?
    echo "exit code: $rc" >> "$name.log"
    echo "stdin:     $input" >> "$name.log"
    [ "$rc" = 0 ] || status=1
    rm -f "$bin"
}
step estop_fixed estop_timeline.cpp estop_fixed.in
exit $status
