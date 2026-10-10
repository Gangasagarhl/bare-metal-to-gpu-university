#!/usr/bin/env bash
# F2-37: the B10 acceptance test under ThreadSanitizer (must report nothing), and the
# throughput/contention measurement built with -O2 and no sanitizers.
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
status=0
g++ $W -O1 -g -fsanitize=thread pc_test.cpp -o .bin_pc_tsan || status=1
start=$(date +%s)
timeout 400 ./.bin_pc_tsan > pc_test_tsan.out 2>&1; rc=$?
secs=$(( $(date +%s) - start ))
sed -i "s#$(pwd)/##g; s# (BuildId: [0-9a-f]*)##g; s#\.bin_##g" pc_test_tsan.out
rec pc_test_tsan pc_test.cpp "g++ $W -O1 -g -fsanitize=thread pc_test.cpp -o pc_test_tsan && ./pc_test_tsan" "$rc" \
    "ThreadSanitizer build, 10 million items; no WARNING lines and exit code 0 = no data race detected; wall time ${secs} s"
[ "$rc" = 0 ] || status=1
g++ $W -O2 pc_bench.cc -o .bin_pc_bench || status=1
timeout 400 ./.bin_pc_bench > pc_bench.out 2>&1; rc=$?
rec pc_bench pc_bench.cc "g++ $W -O2 pc_bench.cc -o pc_bench && ./pc_bench" "$rc" \
    "measured on the build container (a shared virtual machine); numbers change from run to run"
rm -f .bin_*
exit $status
