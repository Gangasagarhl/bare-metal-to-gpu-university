#!/usr/bin/env bash
# F6-15 extra lab steps (no GPU needed).
#   ptx_warp   - PTX of warpTests and hardwareReduce: shfl.sync, vote.sync, redux.sync (sm_80)
#   sass_warp  - SASS warp instructions of the same kernels (sm_80)
#   amd_wave   - gfx90a assembly lines of waveTests that move data between lanes, and its wavefront size
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
CLANG="$(clang++-17 --version | head -n 1) (the compiler that hipcc drives; HIP 5.7)"
COMPILED="compiled only: no GPU is needed to read generated code"
SASSF="grep -E '^[[:space:]]+/\*[0-9a-f]{4}\*/' | sed -E 's#^[[:space:]]+/\*([0-9a-f]+)\*/[[:space:]]+(.*[^[:space:]])[[:space:]]*;.*#\1  \2#'"

step ptx_warp "warp_prims.cu" "$NVCC" "$COMPILED" \
  "nvcc -arch=sm_80 -ptx warp_prims.cu -o .tmp.ptx && grep -E 'shfl\.sync|vote\.sync|redux\.sync|popc' .tmp.ptx | sed -E 's/^[[:space:]]+//' | sort | uniq -c"
step sass_warp "warp_prims.cu" "$NVCC" "$COMPILED" \
  "nvcc -arch=sm_80 -cubin warp_prims.cu -o .tmp.cubin && cuobjdump -sass .tmp.cubin | $SASSF | grep -oE '(SHFL|VOTE|REDUX|POPC)[.A-Z]*' | sort | uniq -c"
step amd_wave "warp_prims_amd.hip" "$CLANG" "$COMPILED" \
  "hipcc -std=c++17 -O2 --offload-arch=gfx90a --cuda-device-only -S warp_prims_amd.hip -o .tmp.s 2>/dev/null && awk '/^_Z9waveTests.*:/,/s_endpgm/' .tmp.s | grep -oE '(ds_bpermute_b32|ds_swizzle_b32|v_mov_b32_dpp|v_add_u32_dpp|v_readlane_b32|v_cmp_[a-z0-9_]+|s_bcnt1_i32_b64)' | sort | uniq -c; grep -E '^[[:space:]]+\.wavefront_size:' .tmp.s | sed -E 's/^[[:space:]]+//'"
rm -f .tmp.ptx .tmp.cubin .tmp.s
exit $status
