#!/usr/bin/env bash
# F12-03 run.sh: (1) Listing 1 (decide.cpp) on the forensic team's matrix and on the
# reviewers' corrected matrix; (2) Listing 2 (lookup_alt.cpp) as a measurement build
# (-O2, no sanitizers), because sanitizers change timings.
set -u
cd "$(dirname "$0")"
FLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
status=0
header() {  # header <name> <listing> <command>
    {
        echo "listing:   $2 (run by run.sh)"
        echo "toolchain: $(g++ --version | head -n 1)"
        echo "command:   $3"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container, $(nproc) CPUs visible)"
    } > "$1.log"
}
step() {  # step <name> <source> <stdin file>
    local name="$1" src="$2" input="$3" bin="./.bin_$1"
    header "$name" "$src" "g++ $FLAGS $src -o ${src%.cpp}; ./${src%.cpp} < $input"
    if ! g++ $FLAGS "$src" -o "$bin" > "$name.out" 2>&1; then
        echo "result:    BUILD FAILED" >> "$name.log"; status=1; return
    fi
    timeout 10 "$bin" < "$input" > "$name.out" 2>&1
    local rc=$?
    echo "exit code: $rc" >> "$name.log"
    echo "stdin:     $input" >> "$name.log"
    [ "$rc" = 0 ] || status=1
    rm -f "$bin"
}
step decide_team decide.cpp decide_team.in
step decide_fixed decide.cpp decide_fixed.in

O2="-std=c++20 -Wall -Wextra -Wpedantic -Werror -O2"
header lookup_alt_O2 lookup_alt.cpp "g++ $O2 lookup_alt.cpp -o lookup_alt_O2; ./lookup_alt_O2"
if g++ $O2 lookup_alt.cpp -o ./.bin_lookup_O2 > lookup_alt_O2.out 2>&1; then
    timeout 30 ./.bin_lookup_O2 < /dev/null > lookup_alt_O2.out 2>&1
    rc=$?
    echo "exit code: $rc" >> lookup_alt_O2.log
    [ "$rc" = 0 ] || status=1
else
    echo "result:    BUILD FAILED" >> lookup_alt_O2.log; status=1
fi
rm -f ./.bin_lookup_O2
exit $status
