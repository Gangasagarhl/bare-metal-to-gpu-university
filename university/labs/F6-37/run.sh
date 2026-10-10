#!/usr/bin/env bash
# F6-37 extra lab steps (no GPU needed).
#   forensic - the TN-4 model (Listing 3) with the peer-access pattern of the forensic lab
#   p2p_api  - the peer and IPC declarations and error codes, read from the installed CUDA 12.0 headers
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
GXX="$(g++ --version | head -n 1)"
FLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
step forensic "topo_model.cpp" "$GXX" "-" \
  "g++ $FLAGS topo_model.cpp -o .tmp_tm && ./.tmp_tm < forensic.in"
step p2p_api "/usr/include/cuda_runtime_api.h, /usr/include/driver_types.h" "$NVCC (headers of the same installation)" "read only: no GPU is needed" \
  "grep -hE '^extern __host__ (__cudart_builtin__ )?cudaError_t CUDARTAPI cuda(DeviceCanAccessPeer|DeviceEnablePeerAccess|DeviceDisablePeerAccess|MemcpyPeerAsync|DeviceGetP2PAttribute|IpcGetMemHandle|IpcOpenMemHandle|IpcCloseMemHandle)\(' /usr/include/cuda_runtime_api.h | sed -E 's/extern __host__ (__cudart_builtin__ )?//; s/CUDARTAPI //'; grep -hE '^ *(cudaErrorPeerAccess[A-Za-z]+|cudaDevP2PAttr[A-Za-z]+) *=|define cudaIpcMemLazyEnablePeerAccess' /usr/include/driver_types.h | sed -E 's#/\*\*<.*##; s/ +/ /g'"
rm -f .tmp_tm
exit $status
