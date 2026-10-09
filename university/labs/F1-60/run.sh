#!/usr/bin/env bash
# F1-60 extra lab steps:
#   forensic   - latency_sim.cpp on the forensic input (evidence pack)
#   amd_wait   - gfx90a assembly of a kernel with three independent loads: where the compiler waits
#   sass_loads - sm_80 SASS of the same kernel: the loads are issued before the first use
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
CLANG="$(clang++-17 --version | head -n 1) (the compiler that hipcc drives; HIP 5.7)"
COMPILED="compiled only: no GPU is needed to read generated code"
SASSF="grep -E '^[[:space:]]+/\*[0-9a-f]{4}\*/' | sed -E 's#^[[:space:]]+/\*([0-9a-f]+)\*/[[:space:]]+(.*[^[:space:]])[[:space:]]*;.*#\1  \2#' | grep -v '  NOP$'"

step forensic "latency_sim.cpp with forensic.in" "$GXX" "-" \
  "g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined latency_sim.cpp -o .bin_forensic && ./.bin_forensic < forensic.in"
step amd_wait "fma3_amd.hip" "$CLANG" "$COMPILED" \
  "hipcc -std=c++17 -O2 --offload-arch=gfx90a --cuda-device-only -S fma3_amd.hip -o .tmp.s 2>/dev/null && awk '/^_Z4fma3.*:/,/s_endpgm/' .tmp.s | grep -v -E '^[[:space:]]*;' | sed -E 's/[[:space:]]*;.*$//'"
step sass_loads "fma3.cu" "$NVCC" "$COMPILED" \
  "nvcc -arch=sm_80 -cubin fma3.cu -o .tmp.cubin && cuobjdump -sass .tmp.cubin | $SASSF"
rm -f .bin_forensic .tmp.s .tmp.cubin
exit $status
