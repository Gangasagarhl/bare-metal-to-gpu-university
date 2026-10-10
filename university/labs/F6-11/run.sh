#!/usr/bin/env bash
# F6-11 extra lab steps (no GPU needed).
#   forensic_res   - ptxas report for both transpose kernels (sm_80): shared memory per block
#   forensic_sass  - the shared-memory instructions of both kernels (sm_80 SASS), with their address forms
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

step forensic_res "transpose_bank.cu" "$NVCC" "$COMPILED" \
  "nvcc -arch=sm_80 -cubin -Xptxas -v transpose_bank.cu -o .tmp.cubin 2>&1 | grep -E 'Compiling|Used' | sed -E 's/ptxas info *: //'"
step forensic_sass "transpose_bank.cu" "$NVCC" "$COMPILED" \
  "nvcc -arch=sm_80 -cubin transpose_bank.cu -o .tmp.cubin && for k in _Z14transposeNoPadPKfPfii _Z12transposePadPKfPfii; do echo \"--- \$k\"; cuobjdump -sass -fun \$k .tmp.cubin | $SASSF | grep -E 'LDS|STS|BAR|(IMAD|LEA) R(0|21), '; done"
rm -f .tmp.cubin
exit $status
