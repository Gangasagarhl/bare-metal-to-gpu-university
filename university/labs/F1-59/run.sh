#!/usr/bin/env bash
# F1-59 extra lab step: the forensic evidence (peak_bw.cpp on the colleague's inputs).
set -u
cd "$(dirname "$0")"
status=0
GXX="$(g++ --version | head -n 1)"
CMD="g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined peak_bw.cpp -o .bin_forensic && ./.bin_forensic < forensic.in"
{
    echo "listing:   peak_bw.cpp with forensic.in"
    echo "toolchain: $GXX"
    echo "command:   $CMD"
    echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
} > forensic.log
bash -c "$CMD" > forensic.out 2>&1
rc=$?
echo "exit code: $rc" >> forensic.log
[ "$rc" -eq 0 ] || status=1
rm -f .bin_forensic
exit $status
