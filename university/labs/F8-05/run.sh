#!/usr/bin/env bash
# F8-05 extra lab steps:
#   ring_O2 - Listing 1 built with -O2 (no sanitizers, which change timing) and run with "big":
#             1 GiB acceptance tests on 2 and 4 ranks, and timings from 1 MiB to 256 MiB
#   race    - forensic evidence: ring_race.cc built with ThreadSanitizer. TSan exits with 66
#             when it reports races; that is the expected result here. Its report is trimmed
#             to the stack frames inside ring_race.cc ([...] marks the cut), with build ids, addresses
#             of the binary and the folder path removed.
set -u
cd "$(dirname "$0")"
status=0
GXX="$(g++ --version | head -n 1)"
MACHINE="$(uname -s) $(uname -m) (cloud build container, $(nproc) logical CPUs shared with other jobs, $(free -g | awk '/Mem:/{print $2}') GiB memory; threads stand in for GPUs)"
header() {
    {
        echo "listing:   $2"
        echo "toolchain: $GXX"
        echo "command:   $3"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $MACHINE"
    } > "$1.log"
}
header ring_O2 ring_allreduce.cpp "g++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror -pthread ring_allreduce.cpp -o ring_O2; ./ring_O2 big"
if g++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror -pthread ring_allreduce.cpp -o .bin_F805_ring 2>> ring_O2.log; then
    timeout 300 ./.bin_F805_ring big > ring_O2.out 2>&1; rc=$?
    echo "exit code: $rc" >> ring_O2.log; [ "$rc" = 0 ] || status=1
else
    echo "result:    BUILD FAILED" >> ring_O2.log; status=1
fi
rm -f .bin_F805_ring

header race ring_race.cc "g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=thread ring_race.cc -o ring_race; ./ring_race (report trimmed, see run.sh)"
if g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=thread ring_race.cc -o .bin_F805_race 2>> race.log; then
    timeout 120 ./.bin_F805_race > .race_full.txt 2>&1; rc=$?
    here="$(pwd)/"
    grep -E '^(WARNING|  (Read|Write|Previous) |    #[0-9]+ .*ring_race\.cc|SUMMARY|20 runs|ThreadSanitizer: reported)' .race_full.txt \
        | sed -E -e "s#${here}##g" -e 's# \(BuildId: [0-9a-f]+\)##g' -e 's# \(\.bin_F805_race\+0x[0-9a-f]+\)##g' \
                 -e 's#\(pid=[0-9]+\)#(pid=...)#' -e 's#at 0x[0-9a-f]+#at 0x...#' > race.out
    echo "[...] (full report: $(wc -l < .race_full.txt) lines, trimmed by run.sh)" >> race.out
    rm -f .race_full.txt
    echo "exit code: $rc" >> race.log
    if [ "$rc" = 66 ]; then
        echo "result:    expected: ThreadSanitizer reported data races (exit code 66) in this deliberately broken program" >> race.log
    else
        echo "result:    UNEXPECTED (expected exit code 66)" >> race.log; status=1
    fi
else
    echo "result:    BUILD FAILED" >> race.log; status=1
fi
rm -f .bin_F805_race
exit $status
