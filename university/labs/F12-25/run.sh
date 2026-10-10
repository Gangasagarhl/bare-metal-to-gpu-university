#!/usr/bin/env bash
# F12-25 run.sh: Listing 1 (mentor_log.cpp) on the forensic pair's records (drift.in).
set -u
cd "$(dirname "$0")"
FLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
status=0
name=drift
{
    echo "listing:   mentor_log.cpp (run by run.sh)"
    echo "toolchain: $(g++ --version | head -n 1)"
    echo "command:   g++ $FLAGS mentor_log.cpp -o mentor_log; ./mentor_log < drift.in"
    echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
} > "$name.log"
if g++ $FLAGS mentor_log.cpp -o ./.bin_ml > "$name.out" 2>&1; then
    timeout 10 ./.bin_ml < drift.in > "$name.out" 2>&1
    rc=$?
    echo "exit code: $rc" >> "$name.log"
    echo "stdin:     drift.in" >> "$name.log"
    [ "$rc" = 0 ] || status=1
else
    echo "result:    BUILD FAILED" >> "$name.log"; status=1
fi
rm -f ./.bin_ml
exit $status
