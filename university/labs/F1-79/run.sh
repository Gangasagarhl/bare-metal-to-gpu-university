#!/usr/bin/env bash
# F1-79 lab step that run_lab.sh cannot do alone: check the rev B netlist (forensic).
set -u -o pipefail
cd "$(dirname "$0")"
status=0
FLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
g++ $FLAGS netcheck.cpp -o .bin_forensic || exit 1
./.bin_forensic < rev_b.net > forensic.out 2>&1; rc=$?
{
    echo "listing:   netcheck.cpp, input rev_b.net"
    echo "toolchain: $(g++ --version | head -n 1)"
    echo "command:   ./netcheck < rev_b.net"
    echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
    echo "exit code: $rc"
} > forensic.log
[ "$rc" = 0 ] || status=1
rm -f .bin_forensic
exit $status
