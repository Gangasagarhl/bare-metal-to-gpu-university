#!/usr/bin/env bash
# F6-17 extra lab steps (no GPU needed).
#   resources  - ptxas report of the three kernels (sm_80): shared memory per block
#   sass_atoms - the atomic instructions of histGlobal and histShared (sm_80 SASS)
#   cub_hist   - what the installed CUB 2.0.1 headers say about bins and privatization (read only)
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

step resources "histogram.cu" "$NVCC" "$COMPILED" \
  "nvcc -arch=sm_80 -cubin -Xptxas -v histogram.cu -o .tmp.cubin 2>&1 | awk '/Compiling entry function .*_Z10hist/{p=1} p&&/Compiling|Used/{print} /Used/{p=0}' | sed -E 's/ptxas info *: //'"
step sass_atoms "histogram.cu" "$NVCC" "$COMPILED" \
  "nvcc -arch=sm_80 -cubin histogram.cu -o .tmp.cubin && for k in _Z10histGlobalPKhmPj _Z10histSharedPKhmPj; do echo \"--- \$k\"; cuobjdump -sass -fun \$k .tmp.cubin | $SASSF | grep -E 'ATOM|RED|BAR|LDG|STS'; done"
rm -f .tmp.cubin
step cub_hist "/usr/include/cub/device/device_histogram.cuh, /usr/include/cub/agent/agent_histogram.cuh" "CUB 2.0.1 (libcub-dev 2.0.1-2), headers as installed" "read only: no GPU needed" \
  "cd /usr/include/cub && grep -n -E 'The number of histogram bins is' device/device_histogram.cuh | head -n 1 && grep -n -E 'Whether to prefer privatized shared-memory bins|Number of privatized shared-memory histogram bins|block-privatized smem histogram' agent/agent_histogram.cuh | head -n 4 | sed -E 's/[[:space:]]+/ /g'"
exit $status
