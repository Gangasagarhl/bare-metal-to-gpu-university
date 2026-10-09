#!/usr/bin/env bash
# F0-75 extra lab step: what -ffast-math does to Kahan summation (g++ may simplify the compensation away).
#   kahan_fastmath - kahan.cpp built with -O2 and with -O2 -ffast-math, both run
set -u
cd "$(dirname "$0")"
status=0
step() {  # step <name> <listing> <toolchain line> <command>
    local name="$1" listing="$2" tc="$3" cmd="$4"
    {
        echo "listing:   $listing"
        echo "toolchain: $tc"
        echo "command:   $cmd"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
    } > "$name.log"
    bash -c "$cmd" > "$name.out" 2>&1
    local rc=$?
    echo "exit code: $rc" >> "$name.log"
    if [ "$rc" -ne 0 ]; then status=1; fi
}
GXX="$(g++ --version | head -n 1)"
step kahan_fastmath "kahan.cpp" "$GXX" \
  "g++ -std=c++20 -O2 kahan.cpp -o .tmp_strict && g++ -std=c++20 -O2 -ffast-math kahan.cpp -o .tmp_fast && echo '--- g++ -std=c++20 -O2' && ./.tmp_strict && echo '--- g++ -std=c++20 -O2 -ffast-math' && ./.tmp_fast"
rm -f .tmp_strict .tmp_fast
exit $status
