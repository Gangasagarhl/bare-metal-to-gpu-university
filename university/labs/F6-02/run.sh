#!/usr/bin/env bash
# F6-02 extra lab steps (run by run_lab.sh after the listings):
#   ptx       - the PTX of brighten: where x, y and the row-major index come from
#   forensic  - grid_sim.cpp on forensic.in (the evidence pack)
#   worked    - grid_sim.cpp on worked.in (Listing 1's 1000 x 600 image, not drawn)
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
NVCC="$(nvcc --version | tail -n 2 | head -n 1)"
GXX="$(g++ --version | head -n 1)"
COMPILED="compiled only: no GPU is needed to read generated code"
step ptx "brighten.cu" "$NVCC" "$COMPILED" \
  "nvcc -arch=sm_80 -ptx brighten.cu -o .tmp.ptx && awk '/.entry _Z8brighten/,/^}/' .tmp.ptx"
step forensic "grid_sim.cpp with forensic.in" "$GXX" "-" \
  "g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined grid_sim.cpp -o .bin_forensic && ./.bin_forensic < forensic.in"
step worked "grid_sim.cpp with worked.in" "$GXX" "-" \
  "g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined grid_sim.cpp -o .bin_worked && ./.bin_worked < worked.in"
rm -f .tmp.ptx .bin_forensic .bin_worked
exit $status
