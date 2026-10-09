#!/usr/bin/env bash
# F2-35: racy programs are not built by run_lab.sh (they are .cc files). This script builds
# them without a checker and with ThreadSanitizer, runs them, and records what happened.
set -u
cd "$(dirname "$0")"
TC="$(g++ --version | head -n 1)"
W="-std=c++20 -Wall -Wextra -Wpedantic -Werror"
rec() {  # rec <name> <listing> <command> <exit code> [note]
    {
        echo "listing:   $2"
        echo "toolchain: $TC"
        echo "command:   $3"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container), $(nproc) CPUs visible"
        if [ "$4" = 124 ]; then echo "exit code: 124 (stopped by the time limit)"; else echo "exit code: $4"; fi
        if [ $# -ge 5 ]; then echo "note:      $5"; fi
    } > "$1.log"
}
clean() {  # make reports independent of the machine: folder names, build ids, temp binary names
    sed -i "s#$(pwd)/##g; s# (BuildId: [0-9a-f]*)##g; s#\.bin_##g" "$1"
}
status=0
# 1. lost_update: plain build, five runs
g++ $W -O0 lost_update.cc -o .bin_lost_update || status=1
: > lost_update_plain.out; rc=0
for i in 1 2 3 4 5; do timeout 20 ./.bin_lost_update >> lost_update_plain.out 2>&1 || rc=$?; done
rec lost_update_plain lost_update.cc "g++ $W -O0 lost_update.cc -o lost_update && ./lost_update (five runs)" "$rc" \
    "the totals change from run to run; this file holds one build's five runs"
# 2. lost_update under ThreadSanitizer
g++ $W -O1 -g -fsanitize=thread lost_update.cc -o .bin_lost_update_tsan || status=1
timeout 60 ./.bin_lost_update_tsan > lost_update_tsan.out 2>&1; rc=$?; clean lost_update_tsan.out
rec lost_update_tsan lost_update.cc "g++ $W -O1 -g -fsanitize=thread lost_update.cc -o lost_update_tsan && ./lost_update_tsan" "$rc" \
    "the program returns 0; exit code 66 is ThreadSanitizer's own after it reported a race"
# 3. the fixed program under ThreadSanitizer
g++ $W -O1 -g -fsanitize=thread lost_update_fixed.cc -o .bin_fixed_tsan || status=1
timeout 60 ./.bin_fixed_tsan > lost_update_fixed_tsan.out 2>&1; rc=$?; clean lost_update_fixed_tsan.out
rec lost_update_fixed_tsan lost_update_fixed.cc "g++ $W -O1 -g -fsanitize=thread lost_update_fixed.cc -o fixed_tsan && ./fixed_tsan" "$rc"
# 4. hoisted_flag at -O0 and at -O2 (3 s time limit), and the -O2 assembly of worker()
g++ $W -O0 hoisted_flag.cc -o .bin_flag_O0 || status=1
timeout 3 ./.bin_flag_O0 > hoisted_flag_O0.out 2>&1; rc=$?
rec hoisted_flag_O0 hoisted_flag.cc "g++ $W -O0 hoisted_flag.cc -o flag_O0 && timeout 3 ./flag_O0" "$rc"
g++ $W -O2 hoisted_flag.cc -o .bin_flag_O2 || status=1
timeout 3 ./.bin_flag_O2 > hoisted_flag_O2.out 2>&1; rc=$?
rec hoisted_flag_O2 hoisted_flag.cc "g++ $W -O2 hoisted_flag.cc -o flag_O2 && timeout 3 ./flag_O2" "$rc" \
    "exit code 124 means the time limit stopped the program: the worker never saw the flag"
g++ $W -O2 -S hoisted_flag.cc -o - 2>&1 | sed -n '/^_Z6workerv:/,/ret/p' | grep -v '^\s*\.cfi' > hoisted_flag_asm.out
rec hoisted_flag_asm hoisted_flag.cc "g++ $W -O2 -S hoisted_flag.cc -o - (lines of worker() only, .cfi directives removed)" 0
# 5. forensic: stats.cc under ThreadSanitizer, and a plain -O2 run
g++ $W -O1 -g -fsanitize=thread stats.cc -o .bin_stats_tsan || status=1
timeout 60 ./.bin_stats_tsan > stats_tsan.out 2>&1; rc=$?; clean stats_tsan.out
rec stats_tsan stats.cc "g++ $W -O1 -g -fsanitize=thread stats.cc -o stats_tsan && ./stats_tsan" "$rc"
g++ $W -O2 stats.cc -o .bin_stats || status=1
: > stats_plain.out; rc=0
for i in 1 2 3 4 5; do timeout 20 ./.bin_stats >> stats_plain.out 2>&1 || rc=$?; done
rec stats_plain stats.cc "g++ $W -O2 stats.cc -o stats && ./stats (five runs)" "$rc" \
    "five runs of one build; whether and how much revenue is lost changes from run to run"
rm -f .bin_*
exit $status
