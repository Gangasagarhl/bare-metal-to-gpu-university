#!/usr/bin/env bash
# F1-56 extra lab steps (run by run_lab.sh after the .cpp/.cu/.hip listings):
#   rocminfo  - AMD's own device report, run in the build container (no GPU)
#   archs     - the GPU targets this nvcc can compile for
#   forensic  - waves.cpp run on the forensic input (evidence pack)
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
ROCMINFO="rocminfo (Ubuntu package $(dpkg-query -W -f='${Version}' rocminfo 2>/dev/null))"
NOGPU="untested on hardware: the build container has no GPU; this is the tool's real output without one"

# rocminfo colours its messages with terminal escape codes; strip them so the page shows text.
step rocminfo "(tool) rocminfo" "$ROCMINFO" "$NOGPU" \
     "rocminfo 2>&1 | sed 's/\x1b\[[0-9;]*m//g'"
step archs "(tool) nvcc --list-gpu-arch" "$NVCC" "-" "nvcc --list-gpu-arch"
step forensic "waves.cpp with forensic.in" "$GXX" "-" \
     "g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined waves.cpp -o .bin_forensic && ./.bin_forensic < forensic.in"
rm -f .bin_forensic
exit $status
