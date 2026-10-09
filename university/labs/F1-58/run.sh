#!/usr/bin/env bash
# F1-58 extra lab steps (no GPU needed: they read what the compilers generate).
#   resources   - ptxas resource report for transposeTiled (sm_80)
#   sass_smem   - the shared-memory, barrier and global-memory instructions of transposeTiled (sm_80 SASS)
#   amd_lds     - the gfx90a assembly lines of transposeTiled that touch the LDS or global memory, and its metadata
#   forensic    - ptxas reports for pressure.cu, default and with -maxrregcount=16, and the count of local-memory instructions
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
CLANG="$(clang++-17 --version | head -n 1) (the compiler that hipcc drives; HIP 5.7)"
COMPILED="compiled only: no GPU is needed to read generated code"
SASSF="grep -E '^[[:space:]]+/\*[0-9a-f]{4}\*/' | sed -E 's#^[[:space:]]+/\*([0-9a-f]+)\*/[[:space:]]+(.*[^[:space:]])[[:space:]]*;.*#\1  \2#'"

step resources "transpose.cu" "$NVCC" "$COMPILED" \
  "nvcc -arch=sm_80 -cubin -Xptxas -v transpose.cu -o .tmp.cubin 2>&1 | grep -E 'transposeTiled|Used|spill'"
step sass_smem "transpose.cu" "$NVCC" "$COMPILED" \
  "nvcc -arch=sm_80 -cubin transpose.cu -o .tmp.cubin && cuobjdump -sass .tmp.cubin | $SASSF | grep -E 'LDS|STS|BAR|LDG|STG'"
step amd_lds "transpose_amd.hip" "$CLANG" "$COMPILED" \
  "hipcc -std=c++17 -O2 --offload-arch=gfx90a --cuda-device-only -S transpose_amd.hip -o .tmp.s 2>/dev/null && awk '/^_Z14transposeTiled.*:/,/s_endpgm/' .tmp.s | grep -E 'ds_|s_barrier|global_load|global_store' | sed -E 's/[[:space:]]*;.*$//' && grep -E '^[[:space:]]+\.(group_segment_fixed_size|vgpr_count|sgpr_count|wavefront_size):' .tmp.s"
step forensic "pressure.cu" "$NVCC" "$COMPILED" \
  "echo '--- build A: nvcc -arch=sm_80 (default)'; nvcc -arch=sm_80 -cubin -Xptxas -v pressure.cu -o .tmpA.cubin 2>&1 | grep -E 'Used|spill'; echo '--- build B: nvcc -arch=sm_80 -maxrregcount=16'; nvcc -arch=sm_80 -maxrregcount=16 -cubin -Xptxas -v pressure.cu -o .tmpB.cubin 2>&1 | grep -E 'Used|spill'; echo '--- local-memory instructions (LDL/STL) in the SASS of A and of B:'; cuobjdump -sass .tmpA.cubin | grep -c -E 'LDL|STL' || true; cuobjdump -sass .tmpB.cubin | grep -c -E 'LDL|STL'"
rm -f .tmp.cubin .tmp.s .tmpA.cubin .tmpB.cubin
exit $status
