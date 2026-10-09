#!/usr/bin/env bash
# F1-63 extra lab steps:
#   evidence  - toolchain evidence for the passport: wavefront/warp size as compiled for gfx90a and sm_80
#   forensic  - passport_check.cpp on the colleague's passport (passport_bad.txt)
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
TOOLS="$(clang++-17 --version | head -n 1) via hipcc (HIP 5.7); $(nvcc --version | tail -n 2 | head -n 1)"
COMPILED="compiled only: no GPU is needed to read generated code"
step evidence "probe_amd.hip and probe.cu" "$TOOLS" "$COMPILED" \
  "echo '--- gfx90a (hipcc): the store of warpSize and the kernel metadata'; hipcc -std=c++17 -O2 --offload-arch=gfx90a --cuda-device-only -S probe_amd.hip -o .tmp.s 2>/dev/null && grep -E 'v_mov_b32.*(64|0x40)|\.wavefront_size:' .tmp.s | sed -E 's/^[[:space:]]+//'; echo '--- sm_80 (nvcc): PTX, then SASS'; nvcc -arch=sm_80 -ptx probe.cu -o .tmp.ptx && grep -E 'WARP_SZ' .tmp.ptx | sed -E 's/^[[:space:]]+//'; nvcc -arch=sm_80 -cubin probe.cu -o .tmp.cubin && cuobjdump -sass .tmp.cubin | grep -E 'MOV.*0x20|IMAD.MOV.*0x20' | sed -E 's#^[[:space:]]+/\*[0-9a-f]+\*/[[:space:]]+##; s#[[:space:]]*;.*##'"
step forensic "passport_check.cpp with passport_bad.txt" "$GXX" "-" \
  "g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined passport_check.cpp -o .bin_forensic && ./.bin_forensic < passport_bad.txt"
rm -f .tmp.s .tmp.ptx .tmp.cubin .bin_forensic
exit $status
