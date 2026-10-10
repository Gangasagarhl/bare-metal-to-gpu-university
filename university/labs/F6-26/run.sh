#!/usr/bin/env bash
# F6-26 extra lab steps (no GPU needed to read generated code):
#   resources   - ptxas report of gemmWmma<half> and gemmWmma<__nv_bfloat16> for sm_80
#   ptx_wmma    - the wmma instructions in the PTX, sm_80, counted per kernel
#   sass_hmma   - the tensor-core instructions (HMMA) in the SASS for sm_80 and sm_90, counted
#   sass_loop   - the main loop of gemmWmma<__nv_bfloat16> in SASS for sm_80 (loads, HMMA, branch)
#   forensic_arch - forensic: what the default build (no -arch) contains, against the sm_80 build
#   build_sm80  - Listing 2 built for sm_80 with cuBLAS and run (no GPU: the runtime's error)
set -u
cd "$(dirname "$0")"
status=0
step() {  # step <name> <listing> <toolchain line> <hardware note or -> <command> [may-fail]
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
    if [ "$rc" -ne 0 ] && [ "${6:-}" != "may-fail" ]; then status=1; fi
}
NVCC="$(nvcc --version | tail -n 2 | head -n 1)"
GXX="$(g++ --version | head -n 1)"
COMPILED="compiled only: no GPU is needed to read generated code"
NOGPU="untested on hardware: the build container has no NVIDIA GPU (AH-26); the build is real, the run shows the runtime's own error"
SASSF="grep -E '^[[:space:]]+/\*[0-9a-f]{4}\*/' | sed -E 's#^[[:space:]]+/\*([0-9a-f]+)\*/[[:space:]]+(.*[^[:space:]])[[:space:]]*;.*#\1  \2#'"
H=_Z8gemmWmmaI6__halfEvPKT_S3_Pfiii
B=_Z8gemmWmmaI13__nv_bfloat16EvPKT_S3_Pfiii

step resources "wmma_gemm.cu" "$NVCC" "$COMPILED" \
  "nvcc -arch=sm_80 -Xptxas -v -cubin wmma_gemm.cu -o .tmp.cubin 2>&1 | grep -E 'Compiling entry|Used|spill' | sed -E 's/^ptxas info +: //; s/^ +//'"
step ptx_wmma "gemm_wmma.cuh (via wmma_gemm.cu)" "$NVCC" "$COMPILED" \
  "nvcc -arch=sm_80 -ptx wmma_gemm.cu -o .tmp.ptx && for f in $H $B; do echo \"--- \$f\"; awk \"/.entry \$f/,/^}/\" .tmp.ptx | grep -o -E 'wmma\.[a-z_]+\.sync[.a-z0-9_]*' | sort | uniq -c; done"
step sass_hmma "gemm_wmma.cuh (via wmma_gemm.cu)" "$NVCC" "$COMPILED" \
  "for a in sm_80 sm_90; do nvcc -arch=\$a -cubin wmma_gemm.cu -o .tmp_\$a.cubin || exit 1; for f in $H $B; do printf '%s %s: ' \$a \$f; cuobjdump -sass -fun \$f .tmp_\$a.cubin | grep -o -E 'HMMA[.A-Z0-9]*' | sort | uniq -c | tr -s ' ' | tr '\n' ';'; echo; done; done"
step sass_loop "gemm_wmma.cuh (via wmma_gemm.cu), sass_loop.py" "$NVCC; $(python3 --version)" "$COMPILED" \
  "nvcc -arch=sm_80 -cubin wmma_gemm.cu -o .tmp.cubin && cuobjdump -sass -fun $B .tmp.cubin | $SASSF | python3 -I sass_loop.py"
step forensic_arch "wmma_gemm.cu" "$NVCC" "$COMPILED" \
  "nvcc -std=c++17 -O2 wmma_gemm.cu -o .tmp_default && nvcc -std=c++17 -O2 -arch=sm_80 wmma_gemm.cu -o .tmp_sm80 && for b in .tmp_default .tmp_sm80; do echo \"--- binary built with: \$( [ \$b = .tmp_default ] && echo 'nvcc -std=c++17 -O2 wmma_gemm.cu' || echo 'nvcc -std=c++17 -O2 -arch=sm_80 wmma_gemm.cu')\"; echo 'machine code (ELF) inside:'; cuobjdump -lelf \$b | sed -E 's/^ELF file +[0-9]+: //'; echo 'PTX inside:'; cuobjdump -lptx \$b | sed -E 's/^PTX file +[0-9]+: //'; printf 'HMMA instructions in its SASS: %s; BPT.TRAP instructions: %s\n' \$(cuobjdump -sass \$b | grep -c HMMA) \$(cuobjdump -sass \$b | grep -c 'BPT.TRAP'); done"
step build_sm80 "wmma_gemm.cu" "$NVCC" "$NOGPU" \
  "nvcc -std=c++17 -O2 -lineinfo -arch=sm_80 -DUSE_CUBLAS wmma_gemm.cu -lcublas -o .tmp_e7 && ./.tmp_e7" may-fail
rm -f .tmp.cubin .tmp_*.cubin .tmp.ptx .tmp_default .tmp_sm80 .tmp_e7
exit $status
