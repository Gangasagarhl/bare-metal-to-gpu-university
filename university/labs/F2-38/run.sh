#!/usr/bin/env bash
# F2-38: ThreadSanitizer runs (atomic listings must be clean; the forensic program is
# race-free yet wrong), the forensic program's plain runs, and the cost measurement.
set -u
cd "$(dirname "$0")"
TC="$(g++ --version | head -n 1)"
W="-std=c++20 -Wall -Wextra -Wpedantic -Werror"
rec() {
    {
        echo "listing:   $2"
        echo "toolchain: $TC"
        echo "command:   $3"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container), $(nproc) CPUs visible"
        echo "exit code: $4"
        if [ $# -ge 5 ]; then echo "note:      $5"; fi
    } > "$1.log"
}
clean() { sed -i "s#$(pwd)/##g; s# (BuildId: [0-9a-f]*)##g; s#\.bin_##g" "$1"; }
status=0
for p in atomic_counter cas spinlock; do
    g++ $W -O1 -g -fsanitize=thread $p.cpp -o .bin_$p || status=1
    timeout 120 ./.bin_$p > ${p}_tsan.out 2>&1; rc=$?; clean ${p}_tsan.out
    rec ${p}_tsan $p.cpp "g++ $W -O1 -g -fsanitize=thread $p.cpp -o ${p}_tsan && ./${p}_tsan" "$rc" \
        "ThreadSanitizer build: no WARNING lines and exit code 0 = no data race detected in this run"
    [ "$rc" = 0 ] || status=1
done
g++ $W -O2 oversell.cc -o .bin_oversell || status=1
: > oversell_plain.out; rc=0
for i in 1 2 3 4 5; do timeout 20 ./.bin_oversell >> oversell_plain.out 2>&1 || rc=$?; done
rec oversell_plain oversell.cc "g++ $W -O2 oversell.cc -o oversell && ./oversell (five runs)" "$rc" \
    "five runs of one build; the counts change from run to run"
g++ $W -O1 -g -fsanitize=thread oversell.cc -o .bin_oversell_tsan || status=1
timeout 60 ./.bin_oversell_tsan > oversell_tsan.out 2>&1; rc=$?; clean oversell_tsan.out
rec oversell_tsan oversell.cc "g++ $W -O1 -g -fsanitize=thread oversell.cc -o oversell_tsan && ./oversell_tsan" "$rc" \
    "ThreadSanitizer build of the forensic program"
g++ $W -O2 costs.cc -o .bin_costs || status=1
timeout 300 ./.bin_costs > costs.out 2>&1; rc=$?
rec costs costs.cc "g++ $W -O2 costs.cc -o costs && ./costs" "$rc" \
    "measured on the build container (a shared virtual machine); times change from run to run"
g++ $W -O2 -S atomic_counter.cpp -o - 2>/dev/null | grep -E 'lock|xadd|xchg|cmpxchg' | sort | uniq -c > atomic_counter_asm.out
rec atomic_counter_asm atomic_counter.cpp "g++ $W -O2 -S atomic_counter.cpp -o - | grep -E 'lock|xadd|xchg|cmpxchg' | sort | uniq -c" 0 \
    "counts of the locked/exchange instructions g++ emitted for Listing 1"
g++ $W -O1 needs_libatomic.cpp -o .bin_libatomic -latomic || status=1
timeout 20 ./.bin_libatomic > needs_libatomic_linked.out 2>&1; rc=$?
rec needs_libatomic_linked needs_libatomic.cpp "g++ $W -O1 needs_libatomic.cpp -o needs_libatomic -latomic && ./needs_libatomic" "$rc" \
    "the same file, linked with the GNU atomic support library"
rm -f .bin_*
exit $status
