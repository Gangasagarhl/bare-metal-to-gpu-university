#!/usr/bin/env bash
# F8-04 extra lab steps (all real, on the build container, which has no GPU):
#   host_topo  - what this machine itself shows: CPU/NUMA lines of lscpu, the PCI tree,
#                and whether the vendor topology tools are installed
#   p2p_enums  - the peer-attribute enumerations exactly as the installed CUDA and HIP headers
#                define them (the evidence for the note in Listing 3)
set -u
cd "$(dirname "$0")"
status=0
MACHINE="$(uname -s) $(uname -m) (cloud build container, no GPU)"
step() {  # step <name> <listing> <toolchain> <command>
    {
        echo "listing:   $2"
        echo "toolchain: $3"
        echo "command:   $4"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $MACHINE"
    } > "$1.log"
    bash -c "$4" > "$1.out" 2>&1
    local rc=$?
    echo "exit code: $rc" >> "$1.log"
    [ "$rc" = 0 ] || status=1
}
step host_topo "(tools) lscpu, lspci, command -v" "$(lscpu --version 2>&1 | head -n 1); $(lspci --version 2>&1 | head -n 1)" \
  "echo '--- lscpu (CPU and NUMA lines)'; lscpu | grep -E '^(Socket|NUMA|Core|Thread|Model name)'; echo '--- lspci -tv'; lspci -tv; echo '--- vendor topology tools'; for t in nvidia-smi rocm-smi amd-smi; do command -v \$t >/dev/null && echo \"\$t: installed\" || echo \"\$t: not installed\"; done"
step p2p_enums "(headers) /usr/include/driver_types.h, /usr/include/hip/hip_runtime_api.h" \
  "nvidia-cuda-dev $(dpkg-query -W -f='${Version}' nvidia-cuda-dev 2>/dev/null); libamdhip64-dev $(dpkg-query -W -f='${Version}' libamdhip64-dev 2>/dev/null)" \
  "echo '--- CUDA: driver_types.h'; grep -A5 'enum __device_builtin__ cudaDeviceP2PAttr ' /usr/include/driver_types.h; echo '--- HIP: hip_runtime_api.h'; grep -B1 -A4 'hipDevP2PAttrPerformanceRank = 0' /usr/include/hip/hip_runtime_api.h"
exit $status
