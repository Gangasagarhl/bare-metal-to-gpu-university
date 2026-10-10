#!/usr/bin/env bash
# F6-21 extra lab steps (no GPU needed to read generated code):
#   resources  - ptxas report of sgemmNaive for sm_80
#   ptx_naive  - the loads and multiply-adds of sgemmNaive in PTX
#   sass_naive - the unrolled inner loop of sgemmNaive in SASS for sm_80, and its instruction mix
#   build_sm80 - Listing 2 built for sm_80 with cuBLAS and run (no GPU: the runtime's error)
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

step resources "gemm_naive.cuh (via sgemm_naive.cu)" "$NVCC" "$COMPILED" \
  "nvcc -arch=sm_80 -Xptxas -v -cubin sgemm_naive.cu -o .tmp.cubin 2>&1 | grep -A2 'sgemmNaive' | grep -E 'Compiling|Used|spill' | sed -E 's/^ptxas info +: //'"
step ptx_naive "gemm_naive.cuh (via sgemm_naive.cu)" "$NVCC" "$COMPILED" \
  "nvcc -arch=sm_80 -ptx sgemm_naive.cu -o .tmp.ptx && awk '/.entry _Z10sgemmNaive/,/^}/' .tmp.ptx > .tmp_k.ptx && printf 'ld.global.f32: %s   fma.rn.f32: %s   (whole kernel)\n' \$(grep -c 'ld.global' .tmp_k.ptx) \$(grep -c 'fma.rn.f32' .tmp_k.ptx)"
step sass_naive "gemm_naive.cuh (via sgemm_naive.cu)" "$NVCC" "$COMPILED" \
  "nvcc -arch=sm_80 -cubin sgemm_naive.cu -o .tmp.cubin && cuobjdump -sass -fun _Z10sgemmNaivePKfS0_Pfiii .tmp.cubin | $SASSF > .tmp.sass && echo 'instruction mix of the whole kernel:' && grep -o -E '(LDG[.A-Z0-9]*|FFMA|STG[.A-Z0-9]*|BRA)' .tmp.sass | sort | uniq -c && echo 'the first loads and multiply-adds of the unrolled loop body:' && grep -E 'LDG|FFMA' .tmp.sass | head -n 12"
step build_sm80 "sgemm_naive.cu" "$NVCC" "$NOGPU" \
  "nvcc -std=c++17 -O2 -lineinfo -arch=sm_80 -DUSE_CUBLAS sgemm_naive.cu -lcublas -o .tmp_e6 && ./.tmp_e6" may-fail
rm -f .tmp.cubin .tmp.ptx .tmp_k.ptx .tmp.sass .tmp_e6
exit $status
