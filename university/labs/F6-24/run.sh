#!/usr/bin/env bash
# F6-24 extra lab steps (no GPU needed to read generated code):
#   resources  - ptxas report (registers, shared memory) of the variants, sm_80
#   ptx_async  - the cp.async instructions of the double-buffered kernel in PTX, sm_80
#   sass_async - asynchronous-copy instructions in the SASS for sm_80, counted, and for sm_75
#   sass_mix   - shared-memory loads and FFMAs of the 64x64 double-buffered kernel against F6-23's
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
D=_Z17sgemmDoubleBufferILi64ELi64ELi8ELi4ELi4EEvPKfS1_Pfiii

step resources "sgemm_dbuf.cu" "$NVCC" "$COMPILED" \
  "nvcc -arch=sm_80 -Xptxas -v -cubin sgemm_dbuf.cu -o .tmp.cubin 2>&1 | grep -E 'Compiling entry|Used|spill' | sed -E 's/^ptxas info +: //; s/^ +//'"
step ptx_async "gemm_dbuf.cuh (via sgemm_dbuf.cu)" "$NVCC" "$COMPILED" \
  "nvcc -arch=sm_80 -ptx sgemm_dbuf.cu -o .tmp.ptx && awk '/.entry $D/,/^}/' .tmp.ptx | grep -o -E 'cp\.async[.a-z0-9_]*|bar\.sync' | sort | uniq -c"
step sass_async "gemm_dbuf.cuh (via sgemm_dbuf.cu)" "$NVCC" "$COMPILED" \
  "for a in sm_80 sm_75; do nvcc -arch=\$a -cubin sgemm_dbuf.cu -o .tmp_\$a.cubin || exit 1; echo \"--- \$a, 64x64 double-buffered kernel\"; cuobjdump -sass -fun $D .tmp_\$a.cubin | $SASSF | grep -o -E '(LDGSTS[.A-Z0-9]*|LDGDEPBAR|DEPBAR[.A-Z0-9]*|LDG[.A-Z0-9]*|STS[.A-Z0-9]*|BAR\.SYNC)' | sort | uniq -c; done"
V=_Z12sgemmRegTileILi64ELi64ELi8ELi4ELi4ELb1ELi1EEvPKfS1_Pfiii
step sass_mix "gemm_dbuf.cuh and ../F6-23/gemm_regtile.cuh (via sgemm_dbuf.cu)" "$NVCC" "$COMPILED" \
  "nvcc -arch=sm_80 -cubin sgemm_dbuf.cu -o .tmp.cubin && for f in $V $D; do echo \"--- \$f\"; cuobjdump -sass -fun \$f .tmp.cubin | $SASSF > .tmp.sass; grep -v '@!PT' .tmp.sass | grep -o -E '(LDS[.A-Z0-9]*|FFMA)' | sort | uniq -c; printf '      (plus %s never-executed \"@!PT LDS RZ, [RZ]\" lines, not counted)\\n' \$(grep -c '@!PT LDS' .tmp.sass); done"
step build_sm80 "sgemm_dbuf.cu" "$NVCC" "$NOGPU" \
  "nvcc -std=c++17 -O2 -lineinfo -arch=sm_80 -DUSE_CUBLAS sgemm_dbuf.cu -lcublas -o .tmp_e6 && ./.tmp_e6" may-fail
rm -f .tmp.cubin .tmp_*.cubin .tmp.ptx .tmp.sass .tmp_e6
exit $status
