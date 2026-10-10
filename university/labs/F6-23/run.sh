#!/usr/bin/env bash
# F6-23 extra lab steps (no GPU needed to read generated code):
#   resources   - ptxas report (registers, shared memory, spills) of all variants, sm_80
#   sass_mix    - instruction mix of the 2-D scalar and float4 kernels, sm_80
#   spill_report - forensic: ptxas report of the 128x128 kernel without and with __launch_bounds__(256, 4)
#   spill_sass  - forensic: local-memory instructions (LDL/STL) in both, and the first few
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
K1=_Z12sgemmRegTileILi128ELi128ELi8ELi8ELi8ELb1ELi1EEvPKfS1_Pfiii
K4=_Z12sgemmRegTileILi128ELi128ELi8ELi8ELi8ELb1ELi4EEvPKfS1_Pfiii
S44=_Z12sgemmRegTileILi64ELi64ELi8ELi4ELi4ELb0ELi1EEvPKfS1_Pfiii
V44=_Z12sgemmRegTileILi64ELi64ELi8ELi4ELi4ELb1ELi1EEvPKfS1_Pfiii

step resources "sgemm_regtile.cu" "$NVCC" "$COMPILED" \
  "nvcc -arch=sm_80 -Xptxas -v -cubin sgemm_regtile.cu -o .tmp.cubin 2>&1 | grep -E 'Compiling entry|Used|spill' | sed -E 's/^ptxas info +: //; s/^ +//'"
step sass_mix "gemm_regtile.cuh (via sgemm_regtile.cu)" "$NVCC" "$COMPILED" \
  "nvcc -arch=sm_80 -cubin sgemm_regtile.cu -o .tmp.cubin && for f in $S44 $V44 $K1; do echo \"--- \$f\"; cuobjdump -sass -fun \$f .tmp.cubin | $SASSF | grep -o -E '(LDG[.A-Z0-9]*|STS[.A-Z0-9]*|LDS[.A-Z0-9]*|FFMA|BAR[.A-Z0-9]*|STG[.A-Z0-9]*)' | sort | uniq -c; done"
step spill_report "spill.cu" "$NVCC" "$COMPILED" \
  "nvcc -arch=sm_80 -Xptxas -v -cubin spill.cu -o .tmp_s.cubin 2>&1 | grep -E 'Compiling entry|Used|spill' | sed -E 's/^ptxas info +: //; s/^ +//'"
step spill_sass "spill.cu" "$NVCC" "$COMPILED" \
  "nvcc -arch=sm_80 -cubin spill.cu -o .tmp_s.cubin && for f in $K1 $K4; do printf '%s: %s LDL, %s STL, %s FFMA\n' \$f \$(cuobjdump -sass -fun \$f .tmp_s.cubin | grep -c -E 'LDL') \$(cuobjdump -sass -fun \$f .tmp_s.cubin | grep -c -E 'STL') \$(cuobjdump -sass -fun \$f .tmp_s.cubin | grep -c -E 'FFMA'); done && echo 'first local-memory instructions of the MINB=4 version:' && cuobjdump -sass -fun $K4 .tmp_s.cubin | $SASSF | grep -E 'LDL|STL' | head -n 6"
step build_sm80 "sgemm_regtile.cu" "$NVCC" "$NOGPU" \
  "nvcc -std=c++17 -O2 -lineinfo -arch=sm_80 -DUSE_CUBLAS sgemm_regtile.cu -lcublas -o .tmp_e6 && ./.tmp_e6" may-fail
rm -f .tmp.cubin .tmp_s.cubin .tmp_e6
exit $status
