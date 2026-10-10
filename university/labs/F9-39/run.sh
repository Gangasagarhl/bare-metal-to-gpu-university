#!/usr/bin/env bash
# Extra runs for F9-39:
#   starve              executor_sim.cpp on starve.in: forensic evidence (course flags)
#   group_threads_tsan  group_threads.cpp built with ThreadSanitizer instead of ASan/UBSan
set -u
GXX="$(g++ --version | head -n 1)"
CXXFLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
status=0
hdr() {   # hdr <name> <listing> <command>
    {
        echo "listing:   $2"
        echo "toolchain: $GXX"
        echo "command:   $3"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
    } > "$1.log"
}
hdr starve executor_sim.cpp "g++ $CXXFLAGS executor_sim.cpp -o executor_sim; ./executor_sim < starve.in"
if g++ $CXXFLAGS executor_sim.cpp -o .bin_starve > .build.txt 2>&1; then
    timeout 10 ./.bin_starve < starve.in > starve.out 2>&1; rc=$?
    echo "exit code: $rc" >> starve.log; echo "stdin:     starve.in" >> starve.log
    [ "$rc" = 0 ] || status=1
else
    echo "result:    BUILD FAILED" >> starve.log; cat .build.txt >> starve.log; status=1
fi
TFLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=thread"
hdr group_threads_tsan group_threads.cpp "g++ $TFLAGS group_threads.cpp -o group_threads_tsan; ./group_threads_tsan"
if g++ $TFLAGS group_threads.cpp -o .bin_tsan > .build.txt 2>&1; then
    timeout 20 ./.bin_tsan > group_threads_tsan.out 2>&1; rc=$?
    echo "exit code: $rc" >> group_threads_tsan.log
    [ "$rc" = 0 ] || status=1
else
    echo "result:    BUILD FAILED" >> group_threads_tsan.log; cat .build.txt >> group_threads_tsan.log; status=1
fi
rm -f .build.txt .bin_starve .bin_tsan
exit $status
