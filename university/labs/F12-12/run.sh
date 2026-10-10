#!/usr/bin/env bash
# F12-12 run.sh - the real-queue measurement (timing build, no sanitizers) and a short
# ThreadSanitizer run of the same program. Every step writes <name>.out and <name>.log.
set -u
cd "$(dirname "$0")"
status=0
B=.build
rm -rf "$B"; mkdir -p "$B"
GXXV="$(g++ --version | head -n 1)"
CPU="$(grep -m1 'model name' /proc/cpuinfo | sed 's/.*: //')"
rec() {  # rec <name> <listing> <command> <exit code text> [note]
    {
        echo "listing:   $2"
        echo "toolchain: $GXXV"
        echo "command:   $3"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container, $(nproc) CPUs visible, $CPU)"
        echo "exit code: $4"
        if [ $# -ge 5 ]; then echo "note:      $5"; fi
    } > "$1.log"
}

g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -O2 -pthread predict_real.cc -o $B/predict_real || exit 1
timeout 120 $B/predict_real > predict_real.out 2>&1; rc=$?
rec predict_real predict_real.cc "g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -O2 -pthread predict_real.cc -o predict_real; ./predict_real" \
    "$rc" "measured on the build container (a shared cloud virtual machine); timings change from run to run and are not a specification (AH-23)"
[ "$rc" = 0 ] || status=1

g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -O1 -fsanitize=thread -pthread predict_real.cc -o $B/predict_tsan || exit 1
timeout 120 $B/predict_tsan --quick > predict_tsan.out 2>&1; rc=$?
rec predict_tsan predict_real.cc "g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -O1 -fsanitize=thread -pthread predict_real.cc -o predict_tsan; ./predict_tsan --quick" \
    "$rc" "ThreadSanitizer build: checks for data races; its timings are meaningless"
[ "$rc" = 0 ] || status=1
grep -q "ThreadSanitizer" predict_tsan.out && status=1

rm -rf "$B"
exit $status
