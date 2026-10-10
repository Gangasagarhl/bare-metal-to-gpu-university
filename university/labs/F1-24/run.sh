#!/usr/bin/env bash
# F1-24: run the datapath tracer of Listing 1 on the forensic program as well.
set -u -o pipefail
cd "$(dirname "$0")"
FLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
g++ $FLAGS datapath.cpp -o .bin_dp || exit 1
./.bin_dp < forensic.s > forensic.out 2>&1; rc=$?
{
    echo "listing:   forensic.s (run with datapath.cpp)"
    echo "toolchain: $(g++ --version | head -n 1)"
    echo "command:   g++ $FLAGS datapath.cpp -o datapath && ./datapath < forensic.s"
    echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
    echo "exit code: $rc"
} > forensic.log
rm -f .bin_dp
exit $rc
