#!/usr/bin/env bash
# Extra run for F5-13: the answer key of the forensic lab checks the history printed by
# vanishing.cpp with the checker of Listing 1 (histories.cpp). run_lab.sh has already run
# vanishing.cpp, so vanishing.out exists; the history is cut from it here, not copied by hand.
set -u
CXXFLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
name=vanishing_check
{
    echo "listing:   histories.cpp, input = the last section of vanishing.out"
    echo "toolchain: $(g++ --version | head -n 1)"
    echo "command:   g++ $CXXFLAGS histories.cpp -o histories; (echo history H7 ...; sed ... vanishing.out; echo end) | ./histories"
    echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
} > "$name.log"
if ! g++ $CXXFLAGS histories.cpp -o .bin_check > .build_check.txt 2>&1; then
    echo "result:    BUILD FAILED" >> "$name.log"; cat .build_check.txt >> "$name.log"
    rm -f .build_check.txt; exit 1
fi
rm -f .build_check.txt
{
    echo "history H7 the forensic lab's requests (value = version of thread t42)"
    sed -n '/as a history for histories.cpp/,$p' vanishing.out | tail -n +2
    echo "end"
} | timeout 60 ./.bin_check > "$name.out" 2>&1
rc=$?
echo "exit code: $rc" >> "$name.log"
rm -f .bin_check
exit $rc
