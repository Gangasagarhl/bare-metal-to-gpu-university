#!/usr/bin/env bash
# F7-08 extra lab steps (compile only: no GPU is needed to read generated code).
#   targets   - wave width the compiler uses for four AMD targets (macro and kernel metadata)
#   lane_isa  - assembly of waveInfo for gfx90a (wave64) and gfx1100 (wave32)
#   ptx_warp  - how CUDA encodes warpSize in PTX for sm_80
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
ISA="grep -v -E '^[[:space:]]*;' | sed -E 's/[[:space:]]*;.*$//'"

step targets "wave_info.hip" "$CLANG" "$COMPILED" \
  "for t in gfx90a gfx942 gfx1030 gfx1100; do m=\$(hipcc --offload-arch=\$t --cuda-device-only -dM -E wave_info.hip 2>/dev/null | grep -E 'define __AMDGCN_WAVEFRONT_SIZE ' | awk '{print \$3}'); hipcc -std=c++17 -O2 --offload-arch=\$t --cuda-device-only -S wave_info.hip -o .tmp.s 2>/dev/null; w=\$(grep -E '^[[:space:]]+\.wavefront_size:' .tmp.s | awk '{print \$2}'); echo \"\$t: __AMDGCN_WAVEFRONT_SIZE=\$m  .wavefront_size=\$w\"; done"
step lane_isa "wave_info.hip" "$CLANG" "$COMPILED" \
  "for t in gfx90a gfx1100; do echo \"--- \$t\"; hipcc -std=c++17 -O2 --offload-arch=\$t --cuda-device-only -S wave_info.hip -o .tmp.s 2>/dev/null && awk '/^_Z8waveInfoPi:/,/s_endpgm/' .tmp.s | $ISA; done"
step ptx_warp "wave_info_cuda.cu" "$NVCC" "$COMPILED" \
  "nvcc -arch=sm_80 -ptx wave_info_cuda.cu -o .tmp.ptx && grep -n -E 'WARP_SZ' .tmp.ptx"
rm -f .tmp.s .tmp.ptx
exit $status
