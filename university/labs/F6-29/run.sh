#!/usr/bin/env bash
# F6-29 extra lab steps (each writes <name>.out and <name>.log):
#   forensic         Listing 2 on forensic.in (evidence pack of the forensic lab)
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
FLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
step forensic "pipeline_model.cpp with forensic.in" "$GXX" "model only: no GPU involved" \
  "g++ $FLAGS pipeline_model.cpp -o .bin_model && ./.bin_model < forensic.in"
rm -f .bin_model
exit $status
