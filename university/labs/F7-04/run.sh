#!/usr/bin/env bash
# F7-04 extra lab steps (no GPU needed). The generated files go to gen/.
#   hipify_saxpy     - toyhipify.py on saxpy.cu (E1), its report and the diff
#   saxpy_amd        - gen/saxpy.hip built for gfx90a with hipcc, then run
#   saxpy_nvidia     - gen/saxpy.hip built through HIP's NVIDIA back end, then run
#   hipify_warp_sum  - toyhipify.py on warp_sum.cu, its report and the diff
#   warp_sum_amd     - gen/warp_sum.hip built for gfx90a (expected to FAIL to compile)
#   warp_sum_nvidia  - gen/warp_sum.hip built through the NVIDIA back end (compiles)
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
    sed -i "s#$(pwd)/##g; s#file://##g" "$name.out"
    echo "exit code: $rc" >> "$name.log"
    return $rc
}
HIPV="$(hipcc --version 2>/dev/null | head -n 1)"
NVV="$(nvcc --version | tail -n 2 | head -n 1)"
PYV="Python $(python3 -c 'import sys; print(sys.version.split()[0])')"
NOGPU="untested on hardware: the build container has no GPU (AH-26); the build is real, the run shows the runtime's own error"
HDR=/usr/include/hip/nvidia_detail/nvidia_hip_runtime_api.h
mkdir -p gen
step hipify_saxpy "toyhipify.py, saxpy.cu" "$PYV; header from libamdhip64-dev $(dpkg-query -W -f='${Version}' libamdhip64-dev 2>/dev/null)" "-" \
  "python3 -I toyhipify.py $HDR saxpy.cu gen/saxpy.hip && echo && echo 'diff -u saxpy.cu gen/saxpy.hip:' && diff -u saxpy.cu gen/saxpy.hip | tail -n +3; true" || status=1
step saxpy_amd "gen/saxpy.hip" "$HIPV (offload target gfx90a)" "$NOGPU" \
  "hipcc -std=c++17 -O2 --offload-arch=gfx90a gen/saxpy.hip -o .tmp_saxpy && ./.tmp_saxpy"
step saxpy_nvidia "gen/saxpy.hip" "$HIPV with HIP_PLATFORM=nvidia; $NVV" "$NOGPU" \
  "HIP_PLATFORM=nvidia CUDA_PATH=/usr hipcc -std=c++17 -x cu gen/saxpy.hip -o .tmp_saxpy_nv && ./.tmp_saxpy_nv"
step hipify_warp_sum "toyhipify.py, warp_sum.cu" "$PYV" "-" \
  "python3 -I toyhipify.py $HDR warp_sum.cu gen/warp_sum.hip" || status=1
# expected compile failure (exit code 1): __shfl_down_sync does not exist in HIP 5.7 for AMD
step warp_sum_amd "gen/warp_sum.hip" "$HIPV (offload target gfx90a)" "compiled only: the build is the evidence" \
  "hipcc -std=c++17 -O2 --offload-arch=gfx90a gen/warp_sum.hip -o .tmp_ws"
step warp_sum_nvidia "gen/warp_sum.hip" "$HIPV with HIP_PLATFORM=nvidia; $NVV" "compiled only: the build is the evidence" \
  "HIP_PLATFORM=nvidia CUDA_PATH=/usr hipcc -std=c++17 -x cu gen/warp_sum.hip -o .tmp_ws_nv && echo 'build succeeded: .tmp_ws_nv created'" || status=1
rm -f .tmp_saxpy .tmp_saxpy_nv .tmp_ws .tmp_ws_nv
exit $status
