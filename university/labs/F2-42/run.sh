#!/usr/bin/env bash
# F2-42: the lock-free listings under ThreadSanitizer, and the ring-versus-queue measurement
# built with -O2 and no sanitizers.
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
for p in spsc_test treiber_stack; do
    g++ $W -O1 -g -fsanitize=thread $p.cpp -o .bin_$p || status=1
    timeout 300 ./.bin_$p > ${p}_tsan.out 2>&1; rc=$?; clean ${p}_tsan.out
    rec ${p}_tsan $p.cpp "g++ $W -O1 -g -fsanitize=thread $p.cpp -o ${p}_tsan && ./${p}_tsan" "$rc" \
        "no WARNING lines and exit code 0 = no data race detected in this run"
    [ "$rc" = 0 ] || status=1
done
g++ $W -O2 ring_bench.cc -o .bin_ring_bench || status=1
timeout 300 ./.bin_ring_bench > ring_bench.out 2>&1; rc=$?
rec ring_bench ring_bench.cc "g++ $W -O2 ring_bench.cc -o ring_bench && ./ring_bench" "$rc" \
    "measured on the build container (a shared virtual machine, other jobs running); numbers change from run to run"
rm -f .bin_*
exit $status
