#!/usr/bin/env bash
# F7-01 extra lab steps (no GPU needed):
#   tools        - the HIP toolchain identifies itself
#   bundle       - what is inside a HIP executable built for an AMD target
#   hello_nvidia - the same source built through HIP's NVIDIA (CUDA) back end, then run
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
step tools "(toolchain only)" "$HIPV" "-" \
  "hipcc --version | head -n 1; echo \"hipconfig --platform: \$(hipconfig --platform)\"; echo \"hipconfig --version:  \$(hipconfig --version)\"; echo \"nvcc: $NVV\"" || status=1
step bundle "hello_hip.hip" "$HIPV" "compiled only: no GPU is needed to list a bundle" \
  "hipcc -std=c++17 -O2 --offload-arch=gfx90a hello_hip.hip -o .tmp_hello && roc-obj-ls .tmp_hello | sed -E 's/#offset=.*//'" || status=1
# The NVIDIA back end: hipcc hands the source to nvcc; -x cu tells nvcc it is GPU source.
step hello_nvidia "hello_hip.hip" "$HIPV with HIP_PLATFORM=nvidia; $NVV" "$NOGPU" \
  "HIP_PLATFORM=nvidia CUDA_PATH=/usr hipcc -std=c++17 -x cu hello_hip.hip -o .tmp_hello_nv && ./.tmp_hello_nv"
# the run is expected to stop with the CUDA runtime's no-device error (exit code 1)
rm -f .tmp_hello .tmp_hello_nv
exit $status
