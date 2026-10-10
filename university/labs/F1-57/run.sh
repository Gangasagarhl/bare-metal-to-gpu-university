#!/usr/bin/env bash
# F1-57 extra lab steps: read the code the compilers really generate (no GPU needed).
#   ptx       - the PTX (virtual ISA) of collatzStep from warp_vote.cu
#   sass      - the SASS (machine code) for sm_80, with encodings and NOP padding removed
#   amd_isa   - the gfx90a assembly of collatzStep from warp_vote_amd.hip (kernel body + metadata lines)
#   wavesize  - the wavefront size the AMD compiler defines for four targets
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

step ptx "warp_vote.cu" "$NVCC" "$COMPILED" \
  "nvcc -arch=sm_80 -ptx warp_vote.cu -o .tmp.ptx && awk '/.entry _Z11collatzStep/,/^}/' .tmp.ptx"
step sass "warp_vote.cu" "$NVCC" "$COMPILED" \
  "nvcc -arch=sm_80 -cubin warp_vote.cu -o .tmp.cubin && cuobjdump -sass .tmp.cubin | grep -E '^[[:space:]]+/\*[0-9a-f]{4}\*/' | sed -E 's#^[[:space:]]+/\*([0-9a-f]+)\*/[[:space:]]+(.*[^[:space:]])[[:space:]]*;.*#\1  \2#' | grep -v '  NOP$'"
step amd_isa "warp_vote_amd.hip" "$CLANG" "$COMPILED" \
  "hipcc -std=c++17 -O2 --offload-arch=gfx90a --cuda-device-only -S warp_vote_amd.hip -o .tmp.s 2>/dev/null && awk '/^_Z11collatzStep.*:/,/s_endpgm/' .tmp.s | grep -v -E '^[[:space:]]*;' | sed -E 's/[[:space:]]*;.*$//' && grep -E '^[[:space:]]+\.(name|vgpr_count|sgpr_count|agpr_count|wavefront_size):|amdhsa_system_sgpr_workgroup_id_x|amdhsa_system_vgpr_workitem_id' .tmp.s | sed -E 's/^[[:space:]]+//'"
step wavesize "(compiler predefined macros)" "$CLANG" "$COMPILED" \
  "for t in gfx90a gfx942 gfx1030 gfx1100; do printf '%-8s ' \$t; echo | clang++-17 -x hip --offload-arch=\$t --cuda-device-only -nogpulib -nogpuinc -dM -E - | grep -E 'define __AMDGCN_WAVEFRONT_SIZE '; done; printf '%-8s ' 'gfx1100 -mwavefrontsize64'; echo | clang++-17 -x hip --offload-arch=gfx1100 -mwavefrontsize64 --cuda-device-only -nogpulib -nogpuinc -dM -E - | grep -E 'define __AMDGCN_WAVEFRONT_SIZE '"
rm -f .tmp.ptx .tmp.cubin .tmp.s
exit $status
