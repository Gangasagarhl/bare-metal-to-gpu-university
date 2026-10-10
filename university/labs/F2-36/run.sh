#!/usr/bin/env bash
# F2-36: ThreadSanitizer runs of the mutex listings (they must be clean), and the
# contention measurement built with -O2 and no sanitizers.
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
for p in bank menu_shared; do
    g++ $W -O1 -g -fsanitize=thread $p.cpp -o .bin_$p || status=1
    timeout 120 ./.bin_$p > ${p}_tsan.out 2>&1; rc=$?
    sed -i "s#$(pwd)/##g; s# (BuildId: [0-9a-f]*)##g; s#\.bin_##g" ${p}_tsan.out
    rec ${p}_tsan $p.cpp "g++ $W -O1 -g -fsanitize=thread $p.cpp -o ${p}_tsan && ./${p}_tsan" "$rc" \
        "ThreadSanitizer build: no WARNING lines and exit code 0 mean no data race was detected in this run"
    [ "$rc" = 0 ] || status=1
done
g++ $W -O2 contention.cc -o .bin_contention || status=1
timeout 300 ./.bin_contention > contention.out 2>&1; rc=$?
rec contention contention.cc "g++ $W -O2 contention.cc -o contention && ./contention" "$rc" \
    "measured on the build container (a shared virtual machine); times change from run to run"
rm -f .bin_*
exit $status
