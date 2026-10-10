#!/usr/bin/env bash
# F7-19 extra lab steps.
#   cublas_ref  - Listing 2 built with nvcc and linked against cuBLAS, then run (no GPU here)
#   cublas_ver  - the cuBLAS version macros of the installed header
#   harness_O2  - Listing 3 built with -O2 and without sanitizers, then run (CPU measurement)
set -u
cd "$(dirname "$0")"
status=0
step() {  # step <name> <listing> <toolchain line> <hardware note or -> <command> [allow-fail]
    local name="$1" listing="$2" tc="$3" hw="$4" cmd="$5" allow="${6:-no}"
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
    if [ "$rc" -ne 0 ] && [ "$allow" = "no" ]; then status=1; fi
}
NVCC="$(nvcc --version | tail -n 2 | head -n 1)"
GXX="$(g++ --version | head -n 1)"
step cublas_ref "cublas_ref.cu.inc" "$NVCC" \
  "untested on hardware: the build container has no NVIDIA GPU (AH-26); the build and link are real, the run shows the runtime's own error" \
  "nvcc -std=c++17 -O2 -x cu cublas_ref.cu.inc -lcublas -o .tmp_cb && ./.tmp_cb" allow
step cublas_ver "/usr/include/cublas_api.h" "$NVCC" "-" \
  "grep -E '#define CUBLAS_VER_(MAJOR|MINOR|PATCH|BUILD)' /usr/include/cublas_api.h"
step harness_O2 "harness.cpp" "$GXX" "measured on the build container's CPU (no GPU); one run, numbers vary from run to run" \
  "g++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror harness.cpp -o .tmp_h && ./.tmp_h"
rm -f .tmp_cb .tmp_h
exit $status
