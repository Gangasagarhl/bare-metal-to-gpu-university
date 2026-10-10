#!/usr/bin/env bash
# F1-61 extra lab steps (no GPU needed: they read what the compilers generate).
#   ptx_wmma   - the PTX lines of tile16 that use the matrix unit
#   sass_wmma  - the SASS of tile16 for sm_80 (encodings and NOP padding removed)
#   hmma_gens  - which HMMA instructions one WMMA 16x16x16 becomes for five GPU generations
#   amd_mfma   - the gfx90a assembly of mfmaOnce and its register metadata
#   amd_targets - the same MFMA builtin compiled for four AMD targets
#   forensic   - instruction mix of gemmNaive (FP32 loop) versus tile16 (WMMA), sm_80
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
SASSF="grep -E '^[[:space:]]+/\*[0-9a-f]{4}\*/' | sed -E 's#^[[:space:]]+/\*([0-9a-f]+)\*/[[:space:]]+(.*[^[:space:]])[[:space:]]*;.*#\1  \2#' | grep -v '  NOP$'"

step ptx_wmma "wmma_tile.cu" "$NVCC" "$COMPILED" \
  "nvcc -arch=sm_80 -ptx wmma_tile.cu -o .tmp.ptx && grep -E 'wmma\.' .tmp.ptx | sed -E 's/^[[:space:]]+//'"
step sass_wmma "wmma_tile.cu" "$NVCC" "$COMPILED" \
  "nvcc -arch=sm_80 -cubin wmma_tile.cu -o .tmp.cubin && cuobjdump -sass .tmp.cubin | $SASSF"
step hmma_gens "wmma_tile.cu" "$NVCC" "$COMPILED" \
  "for a in sm_70 sm_75 sm_80 sm_86 sm_90; do nvcc -arch=\$a -cubin wmma_tile.cu -o .tmp_\$a.cubin || exit 1; printf '%-6s ' \$a; cuobjdump -sass .tmp_\$a.cubin | grep -o -E 'HMMA[.A-Z0-9_]*' | sort | uniq -c | tr -s ' ' | tr '\n' ';'; echo; done"
step amd_mfma "mfma_amd.hip" "$CLANG" "$COMPILED" \
  "hipcc -std=c++17 -O2 --offload-arch=gfx90a --cuda-device-only -S mfma_amd.hip -o .tmp.s 2>/dev/null && awk '/^_Z8mfmaOnce.*:/,/s_endpgm/' .tmp.s | grep -v -E '^[[:space:]]*;' | sed -E 's/[[:space:]]*;.*$//' && grep -E '^[[:space:]]+\.(vgpr_count|agpr_count|wavefront_size):|amdhsa_accum_offset' .tmp.s"
step amd_targets "mfma_amd.hip" "$CLANG" "$COMPILED" \
  "for t in gfx90a gfx942 gfx1030 gfx1100; do echo \"--- \$t\"; hipcc -std=c++17 -O2 --offload-arch=\$t --cuda-device-only -S mfma_amd.hip -o .tmp_\$t.s 2>&1 | grep -E 'error' | sed -E 's/^[^ ]*error/error/'; grep -E 'v_mfma' .tmp_\$t.s 2>/dev/null | sed -E 's/^[[:space:]]+//'; done; true"
step forensic "gemm_plain.cu and wmma_tile.cu" "$NVCC" "$COMPILED" \
  "nvcc -arch=sm_80 -cubin gemm_plain.cu -o .tmp_g.cubin && nvcc -arch=sm_80 -cubin wmma_tile.cu -o .tmp_w.cubin && echo '--- gemmNaive (gemm_plain.cu): floating-point math instructions, counted' && cuobjdump -sass .tmp_g.cubin | grep -o -E '(FFMA|FMUL|FADD|HMMA[.A-Z0-9]*)' | sort | uniq -c && echo '--- tile16 (wmma_tile.cu): floating-point math instructions, counted' && cuobjdump -sass .tmp_w.cubin | grep -o -E '(FFMA|FMUL|FADD|HMMA[.A-Z0-9]*)' | sort | uniq -c"
rm -f .tmp.ptx .tmp.cubin .tmp_*.cubin .tmp.s .tmp_*.s
exit $status
