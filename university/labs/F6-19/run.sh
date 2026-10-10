#!/usr/bin/env bash
# F6-19 extra lab steps (no GPU needed).
#   resources    - ptxas report: shared memory of the three tiled kernels (sm_80)
#   sass_swizzle - shared-memory instructions and the XOR (LOP3) of tiledSwizzle (sm_80 SASS)
set -u
cd "$(dirname "$0")"
status=0
step() {
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

step resources "transpose.cu" "$NVCC" "$COMPILED" \
  "nvcc -arch=sm_80 -cubin -Xptxas -v transpose.cu -o .tmp.cubin 2>&1 | grep -E 'Compiling|Used' | sed -E 's/ptxas info *: //'"
step sass_swizzle "transpose.cu" "$NVCC" "$COMPILED" \
  "nvcc -arch=sm_80 -cubin transpose.cu -o .tmp.cubin && cuobjdump -sass -fun _Z12tiledSwizzlePKfPfii .tmp.cubin | $SASSF | grep -E 'LDS|STS|BAR|LOP3'"
rm -f .tmp.cubin
exit $status
