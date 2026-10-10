#!/usr/bin/env bash
# F6-30 extra lab steps (each writes <name>.out and <name>.log):
#   graph_api_help  the installed header's own description of the graph calls used
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
GXX="$(g++ --version | head -n 1)"
NVCC="$(nvcc --version | tail -n 2 | head -n 1)"
step graph_api_help "/usr/include/cuda_runtime_api.h (excerpt)" "$NVCC" "-" \
  "grep -n -E 'extern __host__ cudaError_t CUDARTAPI (cudaStreamBeginCapture|cudaStreamEndCapture|cudaGraphInstantiate|cudaGraphLaunch|cudaGraphExecKernelNodeSetParams|cudaGraphExecUpdate)\(' /usr/include/cuda_runtime_api.h | sed -E 's/^([0-9]+):extern __host__ cudaError_t CUDARTAPI /\1: /' | head -n 6"
rm -f forkjoin.dot
exit $status
