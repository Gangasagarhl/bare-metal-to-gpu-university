#!/usr/bin/env bash
# F6-14 extra lab steps (no GPU needed).
#   ptx_index   - the PTX index arithmetic of sumBytes32 and sumBytes64 (forensic evidence)
#   sass_shfl   - the shuffle instructions of reduceV5<float> (sm_80 SASS)
#   cub_header  - the CUB 2.0.1 DeviceReduce::Sum declaration as installed (num_items type)
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

step ptx_index "overflow.cu" "$NVCC" "$COMPILED" \
  "nvcc -arch=sm_80 -ptx overflow.cu -o .tmp.ptx && for k in sumBytes32 sumBytes64; do echo \"--- \$k (from the first special-register read to the first global load)\"; awk \"/.entry _Z10\$k/,/^}/\" .tmp.ptx | sed -E 's/^[[:space:]]+//' | awk '/mov.u32.*%ctaid.x/{f=1} f; /ld.global/{exit}' | grep -vE '^(\\\$|//|\\{|\\})'; done"
step sass_shfl "reduce.cu" "$NVCC" "$COMPILED" \
  "nvcc -arch=sm_80 -cubin reduce.cu -o .tmp.cubin && cuobjdump -sass -fun _Z8reduceV5IfEvPKT_PS0_i .tmp.cubin | $SASSF | grep -E 'SHFL|BAR|STS|LDS|FADD'"
step cub_header "/usr/include/cub/device/device_reduce.cuh" "CUB 2.0.1 (libcub-dev 2.0.1-2), header as installed" "$COMPILED" \
  "grep -n 'define CUB_VERSION ' /usr/include/cub/version.cuh; awk 'NR>=325 && NR<=335' /usr/include/cub/device/device_reduce.cuh"
rm -f .tmp.ptx .tmp.cubin
exit $status
