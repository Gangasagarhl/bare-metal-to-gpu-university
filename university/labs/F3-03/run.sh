#!/usr/bin/env bash
# Extra run for F3-03: the scheduling simulator on the forensic lab's input.
set -u
gxx="$(g++ --version | head -n 1)"
flags="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
{
    echo "listing:   sched_sim.cpp with input starve.in"
    echo "toolchain: $gxx"
    echo "command:   g++ $flags sched_sim.cpp -o sched_sim && ./sched_sim < starve.in"
    echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
} > starve.log
g++ $flags sched_sim.cpp -o .bin_starve || { echo "result:    BUILD FAILED" >> starve.log; exit 1; }
./.bin_starve < starve.in > starve.out 2>&1; echo "exit code: $?" >> starve.log
echo "stdin:     starve.in" >> starve.log
rm -f .bin_starve
exit 0
