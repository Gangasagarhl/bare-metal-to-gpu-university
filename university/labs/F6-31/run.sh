#!/usr/bin/env bash
# F6-31 extra lab steps (each writes <name>.out and <name>.log):
#   first_touch_noasan  Listing 2 built WITHOUT the sanitizers, to test the forensic
#                       hypothesis that the extra faults of first_touch.out come from
#                       AddressSanitizer's own shadow memory
set -u
cd "$(dirname "$0")"
status=0
step() {  # step <name> <listing> <toolchain line> <hardware note or -> <command>
    local name="$1" listing="$2" tc="$3" hw="$4" cmd="$5"
    {
        echo "listing:   $listing"
        echo "toolchain: $tc"
        echo "command:   $cmd"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
        if [ "$hw" != "-" ]; then echo "hardware:  $hw"; fi
    } > "$name.log"
    bash -c "$cmd" > "$name.out" 2>&1
    local rc=$?
    echo "exit code: $rc" >> "$name.log"
    if [ "$rc" -ne 0 ]; then status=1; fi
}
GXX="$(g++ --version | head -n 1)"
step first_touch_noasan "first_touch.cpp (no sanitizers)" "$GXX" "-" \
  "g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g first_touch.cpp -o .bin_noasan && ./.bin_noasan"
rm -f .bin_noasan
exit $status
