#!/usr/bin/env bash
# F10-23 extra step: run the lockstep SITL three times and check that the three
# outputs are byte-for-byte identical (lockstep makes the run independent of how the
# operating system schedules the two processes). Recorded in the run_lab.sh format.
set -u
cd "$(dirname "$0")"
name=repeat
bin=./.bin_repeat
cmd="g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined sitl_lockstep.cpp -o repeat"
{
    echo "listing:   sitl_lockstep.cpp (three runs compared)"
    echo "toolchain: $(g++ --version | head -n 1)"
    echo "command:   $cmd; then three runs compared with cmp"
    echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
} > "$name.log"
if ! g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined sitl_lockstep.cpp -o "$bin" > "$name.out" 2>&1; then
    echo "result:    BUILD FAILED" >> "$name.log"; exit 1
fi
rc=0
for i in 1 2 3; do
    timeout 30 "$bin" > ".run$i.txt" 2>&1 || rc=1
done
{
    echo "run 1: $(wc -l < .run1.txt) lines, sha256 $(sha256sum .run1.txt | cut -c1-16)"
    echo "run 2: $(wc -l < .run2.txt) lines, sha256 $(sha256sum .run2.txt | cut -c1-16)"
    echo "run 3: $(wc -l < .run3.txt) lines, sha256 $(sha256sum .run3.txt | cut -c1-16)"
    if cmp -s .run1.txt .run2.txt && cmp -s .run1.txt .run3.txt; then
        echo "identical: yes"
    else
        echo "identical: NO"; rc=1
    fi
} > "$name.out"
echo "exit code: $rc" >> "$name.log"
rm -f "$bin" .run1.txt .run2.txt .run3.txt
exit $rc
