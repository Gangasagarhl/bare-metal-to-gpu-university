#!/usr/bin/env bash
# F6-10 extra lab steps (no GPU needed: they read what the compiler generates).
#   resources  - ptxas resource report for both stencil kernels (sm_80)
#   sass_smem  - shared-memory, barrier and global-memory instructions of stencilStatic (sm_80 SASS)
set -u
cd "$(dirname "$0")"
status=0
step() {  # step <name> <listing> <toolchain line> <hardware note> <command>
    local name="$1" listing="$2" tc="$3" hw="$4" cmd="$5"
    {
        echo "listing:   $listing"
        echo "toolchain: $tc"
        echo "command:   $cmd"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
        echo "hardware:  $hw"
    } > "$name.log"
    bash -c "$cmd" > "$name.out" 2>&1
    local rc=$?
    echo "exit code: $rc" >> "$name.log"
    if [ "$rc" -ne 0 ]; then status=1; fi
}
NVCC="$(nvcc --version | tail -n 2 | head -n 1)"
COMPILED="compiled only: no GPU is needed to read generated code"
SASSF="grep -E '^[[:space:]]+/\*[0-9a-f]{4}\*/' | sed -E 's#^[[:space:]]+/\*([0-9a-f]+)\*/[[:space:]]+(.*[^[:space:]])[[:space:]]*;.*#\1  \2#'"

step resources "stencil.cu" "$NVCC" "$COMPILED" \
  "nvcc -arch=sm_80 -cubin -Xptxas -v stencil.cu -o .tmp.cubin 2>&1 | grep -E 'Compiling|Used' | sed -E 's/ptxas info *: //'"
step sass_smem "stencil.cu" "$NVCC" "$COMPILED" \
  "nvcc -arch=sm_80 -cubin stencil.cu -o .tmp.cubin && cuobjdump -sass -fun _Z13stencilStaticPKiPii .tmp.cubin | $SASSF | grep -E 'LDS|STS|BAR|LDG|STG'"
rm -f .tmp.cubin
exit $status
