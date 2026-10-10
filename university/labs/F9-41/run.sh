#!/usr/bin/env bash
# Extra runs for F9-41 (same compiler and flags as run_lab.sh):
#   hears_nothing  qos_lab.cpp on hears_nothing.in: forensic evidence ("subscriber hears nothing")
set -u
GXX="$(g++ --version | head -n 1)"
CXXFLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
status=0
runcase() {   # runcase <name> <source.cpp> <input>
    local name="$1" src="$2" in="$3"
    {
        echo "listing:   $src"
        echo "toolchain: $GXX"
        echo "command:   g++ $CXXFLAGS $src -o ${src%.cpp}; ./${src%.cpp} < $in"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
    } > "$name.log"
    if g++ $CXXFLAGS "$src" -o ".bin_$name" > .build.txt 2>&1; then
        timeout 10 "./.bin_$name" < "$in" > "$name.out" 2>&1; rc=$?
        echo "exit code: $rc" >> "$name.log"; echo "stdin:     $in" >> "$name.log"
        [ "$rc" = 0 ] || status=1
    else
        echo "result:    BUILD FAILED" >> "$name.log"; cat .build.txt >> "$name.log"; status=1
    fi
    rm -f .build.txt ".bin_$name"
}
runcase hears_nothing qos_lab.cpp hears_nothing.in
exit $status
