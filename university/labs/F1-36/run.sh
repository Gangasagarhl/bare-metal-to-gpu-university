#!/usr/bin/env bash
# F1-36: the MESI model on two more traces, the false-sharing timing (-O2: a measurement),
# and the same program under ThreadSanitizer with a small count (to show there is no data race).
set -u -o pipefail
cd "$(dirname "$0")"
TC="$(g++ --version | head -n 1)"
MACH="$(uname -s) $(uname -m) (cloud build container, $(nproc) CPUs); CPU model: $(grep -m1 'model name' /proc/cpuinfo | cut -d: -f2 | sed 's/^ //')"
rec() {
    {
        echo "listing:   $2"
        echo "toolchain: $TC"
        echo "command:   $3"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $MACH"
        echo "exit code: $4"
        if [ $# -ge 5 ]; then echo "$5"; fi
    } > "$1.log"
}
status=0
SAN="g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
$SAN mesi.cpp -o .bin_mesi || exit 1
for t in pingpong padded; do
    ./.bin_mesi < $t.in > $t.out 2>&1; rc=$?; [ $rc = 0 ] || status=1
    rec $t mesi.cpp "$SAN mesi.cpp -o mesi && ./mesi < $t.in" $rc "stdin:     $t.in"
done
OPT="g++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror -pthread"
$OPT false_sharing.cc -o .bin_fs || exit 1
timeout 300 ./.bin_fs > false_sharing.out 2>&1; rc=$?; [ $rc = 0 ] || status=1
rec false_sharing false_sharing.cc "$OPT false_sharing.cc -o false_sharing && ./false_sharing" $rc \
    "note:      measured on the build container (a shared cloud virtual machine), one session; not a specification (AH-23)"
TSAN="g++ -std=c++20 -O1 -g -Wall -Wextra -Wpedantic -Werror -fsanitize=thread"
$TSAN false_sharing.cc -o .bin_fs_tsan || exit 1
timeout 300 ./.bin_fs_tsan 100000 > false_sharing_tsan.out 2>&1; rc=$?; [ $rc = 0 ] || status=1
rec false_sharing_tsan false_sharing.cc "$TSAN false_sharing.cc -o false_sharing_tsan && ./false_sharing_tsan 100000" $rc \
    "note:      ThreadSanitizer prints a report and exits non-zero if it finds a data race; timings under it are meaningless"
rm -f .bin_mesi .bin_fs .bin_fs_tsan
exit $status
