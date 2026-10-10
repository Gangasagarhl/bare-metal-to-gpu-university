#!/usr/bin/env bash
# F7-17 extra lab steps (compile only unless noted: no GPU is needed to read generated code).
#   targets     - one MFMA builtin for five AMD targets: instruction, accumulator registers, or error
#   isa_gemm    - inner loop and register metadata of mfmaGemm (Listing 1) for gfx90a
#   wmma_rdna3  - the RDNA3 WMMA builtin for gfx1100 (wave32)
#   cuda_wmma   - the CUDA WMMA API: build for sm_80, run, and count HMMA instructions in SASS
#   lin_isa     - forensic evidence: Lin's kernel built for gfx942 and for gfx90a, plus gfx macros
set -u
cd "$(dirname "$0")"
status=0
step() {  # step <name> <listing> <toolchain line> <hardware note or -> <command> [allow-fail]
    local name="$1" listing="$2" tc="$3" hw="$4" cmd="$5" allow="${6:-no}"
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
    sed -i -E '/argument unused during compilation/d' "$name.out"
    echo "exit code: $rc" >> "$name.log"
    if [ "$rc" -ne 0 ] && [ "$allow" = "no" ]; then status=1; fi
}
NVCC="$(nvcc --version | tail -n 2 | head -n 1)"
CLANG="$(clang++-17 --version | head -n 1) (the compiler that hipcc drives; HIP 5.7)"
COMPILED="compiled only: no GPU is needed to read generated code"
HIPS="hipcc -x hip -std=c++17 -O2 --cuda-device-only -S"

step targets "one_mfma.hip.inc" "$CLANG" "$COMPILED" \
  "for t in gfx908 gfx90a gfx942 gfx1030 gfx1100; do echo \"--- \$t\"; if $HIPS --offload-arch=\$t one_mfma.hip.inc -o .tmp.s 2>.tmp.err; then grep -E 'v_mfma' .tmp.s | sed -E 's/^[[:space:]-]+//'; grep -E '^[[:space:]-]+\.agpr_count:' .tmp.s | sed -E 's/^[[:space:]-]+//'; else grep -E 'error' .tmp.err; fi; done"
step isa_gemm "mfma_gemm.hip" "$CLANG" "$COMPILED" \
  "$HIPS --offload-arch=gfx90a mfma_gemm.hip -o .tmp.s 2>/dev/null && echo 'inner loop (.LBB0_5), instruction counts:' && awk '/Inner Loop Header/,/s_cbranch_scc1/' .tmp.s | grep -o -E '^[[:space:]]+[a-z_0-9]+' | sort | uniq -c | sort -rn | sed -E 's/^ +//' && echo 'the MFMA line:' && grep -E 'v_mfma' .tmp.s | sed -E 's/^[[:space:]-]+//' && grep -E '^[[:space:]-]+\.(agpr_count|vgpr_count|sgpr_count|group_segment_fixed_size):|amdhsa_accum_offset' .tmp.s | sed -E 's/^[[:space:]-]+//'"
step wmma_rdna3 "wmma_rdna3.hip.inc" "$CLANG" "$COMPILED" \
  "$HIPS --offload-arch=gfx1100 wmma_rdna3.hip.inc -o .tmp.s 2>/dev/null && grep -E 'v_wmma' .tmp.s | sed -E 's/^[[:space:]-]+//' && grep -E '^[[:space:]]+\.wavefront_size:' .tmp.s | sed -E 's/^[[:space:]-]+//'"
step cuda_wmma "cuda_wmma.cu.inc" "$NVCC" \
  "untested on hardware: the build container has no NVIDIA GPU (AH-26); the build and the SASS are real, the run shows the runtime's own error" \
  "nvcc -std=c++17 -O2 -arch=sm_80 -x cu cuda_wmma.cu.inc -o .tmp_cw && { ./.tmp_cw; echo \"(program exit code \$?)\"; } && echo 'SASS matrix instructions in wmmaGemm:' && cuobjdump -sass .tmp_cw | grep -o -E 'HMMA[.A-Z0-9]*' | sort | uniq -c | sed -E 's/^ +//'"
step lin_isa "slow_gemm.hip.inc" "$CLANG" "$COMPILED" \
  "for t in gfx942 gfx90a; do echo \"--- built for \$t: matrix and FMA instructions in linGemm\"; $HIPS --offload-arch=\$t slow_gemm.hip.inc -o .tmp.s 2>/dev/null || exit 1; awk '/^_Z7linGemmPKDF16_S0_Pfiii:/,/s_endpgm/' .tmp.s | grep -o -E '^[[:space:]]+(v_mfma[a-z_0-9]*|v_fma[a-z_0-9]*|v_fmac[a-z_0-9]*|v_pk_fma[a-z_0-9]*|v_mad[a-z_0-9]*)' | sort | uniq -c | sed -E 's/^ +//'; echo \"total v_mfma: \$(grep -c v_mfma .tmp.s)\"; done; echo '--- predefined gfx macros when building for gfx942'; hipcc -x hip --offload-arch=gfx942 --cuda-device-only -dM -E slow_gemm.hip.inc 2>/dev/null | grep -E 'define __gfx[0-9a-f]+__ ' | sort"
rm -f .tmp.s .tmp.err .tmp_cw
exit $status
