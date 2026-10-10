#!/usr/bin/env bash
# F12-23 run.sh: the forensic-lab analysis (history.cpp with the argument "analyse"),
# used by the answer key. run_lab.sh has already produced history.out (the evidence).
# Also Listing 1 on schedule_narrow.in (check-yourself question 8).
set -u
cd "$(dirname "$0")"
FLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
status=0
name=history_key
{
    echo "listing:   history.cpp (run by run.sh)"
    echo "toolchain: $(g++ --version | head -n 1)"
    echo "command:   g++ $FLAGS history.cpp -o history; ./history analyse < history.in"
    echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
} > "$name.log"
if g++ $FLAGS history.cpp -o ./.bin_hk > "$name.out" 2>&1; then
    timeout 10 ./.bin_hk analyse < history.in > "$name.out" 2>&1
    rc=$?
    echo "exit code: $rc" >> "$name.log"
    echo "stdin:     history.in" >> "$name.log"
    [ "$rc" = 0 ] || status=1
else
    echo "result:    BUILD FAILED" >> "$name.log"; status=1
fi
rm -f ./.bin_hk

name=schedule_narrow
{
    echo "listing:   schedule.cpp (run by run.sh)"
    echo "toolchain: $(g++ --version | head -n 1)"
    echo "command:   g++ $FLAGS schedule.cpp -o schedule; ./schedule < schedule_narrow.in"
    echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
} > "$name.log"
if g++ $FLAGS schedule.cpp -o ./.bin_sn > "$name.out" 2>&1; then
    timeout 20 ./.bin_sn < schedule_narrow.in > "$name.out" 2>&1
    rc=$?
    echo "exit code: $rc" >> "$name.log"
    echo "stdin:     schedule_narrow.in" >> "$name.log"
    [ "$rc" = 0 ] || status=1
else
    echo "result:    BUILD FAILED" >> "$name.log"; status=1
fi
rm -f ./.bin_sn
exit $status
