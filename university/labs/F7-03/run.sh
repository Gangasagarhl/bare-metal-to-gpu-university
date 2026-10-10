#!/usr/bin/env bash
# F7-03 extra lab steps (no GPU needed):
#   hipmap          - the HIP -> CUDA name map, read from HIP's own NVIDIA back-end header
#   api_tour_nvidia - api_tour.hip built through the NVIDIA back end, then run
#   leftover_nvidia - the half-ported leftover.hip built through the NVIDIA back end
#                     (run_lab.sh's own AMD build of it is the expected failure leftover.out)
#   unchecked_warnings - the compiler's warnings for unchecked.hip on both back ends ([[nodiscard]])
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
NOGPU="untested on hardware: the build container has no GPU (AH-26); the build is real, the run shows the runtime's own error"
HDR=/usr/include/hip/nvidia_detail/nvidia_hip_runtime_api.h
step hipmap "hipmap.py" "Python $(python3 -c 'import sys; print(sys.version.split()[0])'); header from package libamdhip64-dev $(dpkg-query -W -f='${Version}' libamdhip64-dev 2>/dev/null)" "-" \
  "python3 -I hipmap.py $HDR hipMalloc hipFree hipMemcpy hipMemcpyHostToDevice hipMemcpyDeviceToHost hipMemset hipMemcpyAsync hipMemcpyToSymbol hipMallocManaged hipHostMalloc hipHostFree hipDeviceSynchronize hipGetLastError hipPeekAtLastError hipGetErrorString hipGetErrorName hipGetDeviceCount hipSetDevice hipGetDeviceProperties hipStream_t hipStreamCreate hipStreamSynchronize hipStreamDestroy hipEvent_t hipEventCreate hipEventRecord hipEventSynchronize hipEventElapsedTime hipEventDestroy hipError_t hipDeviceProp_t" || status=1
step api_tour_nvidia "api_tour.hip" "$HIPV with HIP_PLATFORM=nvidia; $NVV" "$NOGPU" \
  "HIP_PLATFORM=nvidia CUDA_PATH=/usr hipcc -std=c++17 -x cu api_tour.hip -o .tmp_tour_nv && ./.tmp_tour_nv"
# this build SUCCEEDS on the NVIDIA back end although the AMD build fails: that is the lesson
step leftover_nvidia "leftover.hip" "$HIPV with HIP_PLATFORM=nvidia; $NVV" "compiled only: the build is the evidence" \
  "HIP_PLATFORM=nvidia CUDA_PATH=/usr hipcc -std=c++17 -x cu leftover.hip -o .tmp_left_nv && echo 'build succeeded: .tmp_left_nv created'" || status=1
step unchecked_warnings "unchecked.hip" "$HIPV; $NVV" "compiled only: the warnings are the evidence" \
  "echo '== AMD (hipcc, gfx90a):' && hipcc -std=c++17 -O2 --offload-arch=gfx90a unchecked.hip -o .tmp_un 2>&1 && echo '== NVIDIA back end:' && HIP_PLATFORM=nvidia CUDA_PATH=/usr hipcc -std=c++17 -x cu unchecked.hip -o .tmp_un_nv 2>&1 | grep -E 'warning' | sort -u" || status=1
rm -f .tmp_tour_nv .tmp_left_nv .tmp_un .tmp_un_nv
exit $status
