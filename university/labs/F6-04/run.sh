#!/usr/bin/env bash
# F6-04 extra lab steps (compiled only; no GPU needed):
#   sass_default - arithmetic instructions of saxpy as nvcc compiles it by default
#   sass_nofmad  - the same with --fmad=false (no contraction of a*x+y into one FMA)
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
COMPILED="compiled only: no GPU is needed to read generated code"
SASS="cuobjdump -sass -fun _Z5saxpyifPKfPf .tmp.cubin | grep -E '^[[:space:]]+/\*[0-9a-f]{4}\*/' | sed -E 's#^[[:space:]]+/\*([0-9a-f]+)\*/[[:space:]]+(.*[^[:space:]])[[:space:]]*;.*#\1  \2#' | grep -v '  NOP$'"
step sass_default "saxpy.cu" "$NVCC" "$COMPILED" \
  "nvcc -arch=sm_80 -O2 -cubin saxpy.cu -o .tmp.cubin && $SASS"
step sass_nofmad "saxpy.cu" "$NVCC" "$COMPILED" \
  "nvcc -arch=sm_80 -O2 --fmad=false -cubin saxpy.cu -o .tmp.cubin && $SASS"
rm -f .tmp.cubin
exit $status
