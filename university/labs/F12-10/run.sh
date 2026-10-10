#!/usr/bin/env bash
# F12-10 run.sh - the data-race test: 20 normal runs (count passes and failures), then one
# run built with ThreadSanitizer. Every step writes <name>.out and <name>.log.
set -u
cd "$(dirname "$0")"
status=0
B=.build
rm -rf "$B"; mkdir -p "$B"
GXXV="$(g++ --version | head -n 1)"
rec() {  # rec <name> <listing> <toolchain> <command> <exit code text>
    {
        echo "listing:   $2"
        echo "toolchain: $3"
        echo "command:   $4"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container, $(nproc) CPUs visible)"
        echo "exit code: $5"
    } > "$1.log"
}

g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -O0 -pthread race.cc -o $B/race || exit 1
pass=0; fail=0
for i in $(seq 1 20); do
    if timeout 20 $B/race > /dev/null 2>&1; then pass=$((pass + 1)); else fail=$((fail + 1)); fi
done
echo "race test, 20 runs of the same binary: $pass passed, $fail failed" > race_runs.out
rec race_runs race.cc "$GXXV" "g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -O0 -pthread race.cc -o race; run 20 times, count exit codes" "0 (counting script)"

g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -O1 -fsanitize=thread race.cc -o $B/race_tsan || exit 1
timeout 60 $B/race_tsan > race_tsan.out 2>&1; rc=$?
sed -i "s#$(pwd)/##g; s#(\.build/race_tsan+0x[0-9a-f]*)#(race_tsan)#g; s#(BuildId: [0-9a-f]*)##g" race_tsan.out
rec race_tsan race.cc "$GXXV" "g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -O1 -fsanitize=thread race.cc -o race_tsan; ./race_tsan" \
    "$rc (ThreadSanitizer makes the program exit with a failure status after a report)"
grep -q "WARNING: ThreadSanitizer: data race" race_tsan.out || status=1

rm -rf "$B"
exit $status
