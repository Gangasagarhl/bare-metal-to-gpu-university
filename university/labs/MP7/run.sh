#!/usr/bin/env bash
# run.sh - MP7 starter lab, the steps that are not a single C++ program:
#   repeat : rebuild sitl_feature.cpp, run it twice, compare both runs with the recorded
#            sitl_feature.out (lockstep SITL must be bit-identical, guide gate evidence);
#   report : the SITL test report from sitl_results.csv (python3 report.py).
# Writes <step>.out and <step>.log in the format of run_lab.sh. Leaves no binaries behind.
set -u
cd "$(dirname "$0")" || exit 2
CXX="${CXX:-g++}"
CXXFLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
status=0
header() {  # step, listing, toolchain, command
    {
        echo "listing:   $2"
        echo "toolchain: $3"
        echo "command:   $4"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
        echo "hardware:  untested on hardware: course simulator only (no PX4, no ArduPilot, no flight controller in this build)"
    } > "$1.log"
}

# --- repeat
header repeat "sitl_feature.cpp (run twice) + sitl_feature.out" "$($CXX --version | head -n 1)" \
    "$CXX $CXXFLAGS sitl_feature.cpp; run twice; sha256sum of the three outputs"
tmp="$(mktemp -d)"
rc=0
if $CXX $CXXFLAGS sitl_feature.cpp -o "$tmp/sf" > "$tmp/build.txt" 2>&1; then
    ( cd "$tmp" && ./sf > run1.txt 2>&1; ./sf > run2.txt 2>&1 )
    h0="$(sha256sum < sitl_feature.out | cut -c1-16)"
    h1="$(sha256sum < "$tmp/run1.txt" | cut -c1-16)"
    h2="$(sha256sum < "$tmp/run2.txt" | cut -c1-16)"
    {
        echo "recorded sitl_feature.out  sha256 $h0"
        echo "rebuild, run 1             sha256 $h1"
        echo "rebuild, run 2             sha256 $h2"
        if [ "$h0" = "$h1" ] && [ "$h1" = "$h2" ]; then
            echo "verdict: identical (lockstep SITL is deterministic)"
        else
            echo "verdict: DIFFERENT"; rc=1
        fi
    } > repeat.out
else
    cp "$tmp/build.txt" repeat.out; rc=1
fi
rm -rf "$tmp"
echo "exit code: $rc" >> repeat.log
[ "$rc" = 0 ] || status=1

# --- report
header report "report.py" "$(python3 --version 2>&1)" "python3 -I report.py ."
python3 -I report.py . > report.out 2>&1; rc=$?
echo "exit code: $rc" >> report.log
[ "$rc" = 0 ] || status=1
exit $status
