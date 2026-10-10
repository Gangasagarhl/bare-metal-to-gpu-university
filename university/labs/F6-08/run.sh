#!/usr/bin/env bash
# F6-08 extra lab steps:
#   sass_copies - the machine code of both copy kernels (compiled only)
#   forensic    - the coalescing model on forensic.in (evidence pack)
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
CLEAN="grep -E '^[[:space:]]+/\*[0-9a-f]{4}\*/' | sed -E 's#^[[:space:]]+/\*([0-9a-f]+)\*/[[:space:]]+(.*[^[:space:]])[[:space:]]*;.*#\1  \2#' | grep -v '  NOP$'"
step sass_copies "copy_kernels.cu" "$NVCC" "$COMPILED" \
  "nvcc -arch=sm_80 -cubin copy_kernels.cu -o .tmp.cubin && echo '--- copyStrided' && cuobjdump -sass -fun _Z11copyStridedPKfPfii .tmp.cubin | $CLEAN && echo '--- copyOffset' && cuobjdump -sass -fun _Z10copyOffsetPKfPfii .tmp.cubin | $CLEAN"
step forensic "coalesce_model.cpp with forensic.in" "$GXX" "-" \
  "g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined coalesce_model.cpp -o .bin_forensic && ./.bin_forensic < forensic.in"
rm -f .tmp.cubin .bin_forensic
exit $status
