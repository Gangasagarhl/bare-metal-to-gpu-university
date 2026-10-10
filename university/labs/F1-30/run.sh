#!/usr/bin/env bash
# F1-30: build and run the racy counter three times without a checker, then once with
# ThreadSanitizer (-fsanitize=thread), and record what really happened.
set -u -o pipefail
cd "$(dirname "$0")"
rec() {
    {
        echo "listing:   race.cc"
        echo "toolchain: $(g++ --version | head -n 1)"
        echo "command:   $2"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container), $(nproc) CPUs visible"
        echo "exit code: $3"
        if [ $# -ge 4 ]; then echo "$4"; fi
    } > "$1.log"
}
B="g++ -std=c++20 -O0 -Wall -Wextra -Wpedantic -Werror race.cc -o race"
g++ -std=c++20 -O0 -Wall -Wextra -Wpedantic -Werror race.cc -o .bin_race || exit 1
: > race_plain.out
rc=0
for i in 1 2 3; do ./.bin_race >> race_plain.out 2>&1 || rc=$?; done
rec race_plain "$B && ./race (three runs)" "$rc" \
    "note:      the counts change from run to run; this file holds one build's three runs"
T="g++ -std=c++20 -O1 -g -fsanitize=thread race.cc -o race_tsan"
g++ -std=c++20 -O1 -g -fsanitize=thread race.cc -o .bin_tsan || exit 1
./.bin_tsan > race_tsan.out 2>&1; rc=$?
# keep the report readable: drop the machine's folder names and the build ids
sed -i "s#$(pwd)/##g; s# (BuildId: [0-9a-f]*)##g; s#\.bin_tsan#race_tsan#g" race_tsan.out
rec race_tsan "$T && ./race_tsan" "$rc" \
    "note:      the program itself returns 0; the non-zero exit code comes from ThreadSanitizer after it reported a race"
rm -f .bin_race .bin_tsan
# the atomic fetch_add of Listing 1 as x86-64 machine code: build threads.cpp at -O2 without
# sanitizers and show the lines of the disassembly around the lock-prefixed instruction
g++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror threads.cpp -o .bin_t2 \
    && objdump -d --no-show-raw-insn -M intel -C .bin_t2 | grep -B2 -A1 'lock ' > threads_lock.out; rc=$?
{
    echo "listing:   threads.cpp"
    echo "toolchain: $(g++ --version | head -n 1); $(objdump --version | head -n 1)"
    echo "command:   g++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror threads.cpp -o threads && objdump -d --no-show-raw-insn -M intel -C threads | grep -B2 -A1 'lock '"
    echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
    echo "exit code: $rc"
    echo "note:      added in the verification pass of 2026-10-10; shows the instruction GCC emitted for chunksDone.fetch_add(1)"
} > threads_lock.log
rm -f .bin_t2
[ -s race_tsan.out ] && [ -s threads_lock.out ]
