#!/usr/bin/env bash
# F12-16 run.sh: Listing 1 (gameday.cpp) on two more scripts: the uncoordinated response
# (forensic evidence) and the coordinated one revealed only up to t+10 (game-master view).
# Built with the same compiler and flags as run_lab.sh.
set -u
cd "$(dirname "$0")"
FLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
status=0
g++ $FLAGS gameday.cpp -o ./.bin_gd || exit 1
step() {  # step <name> <stdin file> <args...>
    local name="$1" input="$2"; shift 2
    {
        echo "listing:   gameday.cpp (run by run.sh)"
        echo "toolchain: $(g++ --version | head -n 1)"
        echo "command:   g++ $FLAGS gameday.cpp -o gameday; ./gameday $* < $input"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
    } > "$name.log"
    timeout 10 ./.bin_gd "$@" < "$input" > "$name.out" 2>&1
    local rc=$?
    echo "exit code: $rc" >> "$name.log"
    echo "stdin:     $input" >> "$name.log"
    [ "$rc" = 0 ] || status=1
}
step gameday_chaos gameday_chaos.txt
step gameday_reveal gameday.in 10
rm -f ./.bin_gd
exit $status
