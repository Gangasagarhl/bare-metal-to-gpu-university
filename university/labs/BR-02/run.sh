#!/usr/bin/env bash
# BR-02 Listing 7: extra lab steps (no GPU needed; every GPU run stops at the runtime's "no device").
#   tools          - the toolchain identifies itself; which GPU tools are installed here
#   libs           - which CUDA and HIP/ROCm library files exist in this container
#   hipmap         - the HIP name of every CUDA runtime call used by Listing 1 and CU201's timing
#   hipify         - the university's teaching translator (F7-04) on Listing 1 -> gen/saxpy_neg.hip
#   port_amd       - gen/saxpy_neg.hip built for gfx90a (EXPECTED to fail: __ballot_sync)
#   port_nvidia    - gen/saxpy_neg.hip built through HIP's NVIDIA back end, then run
#   naive_amd      - gen/saxpy_neg_naive.hip (ballot renamed only) for gfx90a, then run
#   naive_warn     - the same build with -Wshorten-64-to-32 (the compiler can see the trap)
#   naive_nvidia   - the same file through the NVIDIA back end: whose __ballot is it?
#   fixed_nvidia   - saxpy_neg_fixed.hip through the NVIDIA back end, then run
#   isa_naive      - AMD ISA of the naive port (gfx90a), kernel body and metadata
#   isa_fixed      - AMD ISA of the fixed kernel (gfx90a)
#   isa_summary    - the lines that differ, for gfx90a and gfx1030
#   ptx_summary    - the vote and popcount in PTX: CUDA original and fixed HIP (sm_80)
#   lds            - one __shared__ tile: AMD ISA (ds_*, group segment) and PTX (.shared)
#   cublas_hipify  - the translator on a cuBLAS program, then the AMD build of its output
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
    sed -i -E "s#$(pwd)/##g; s#file://##g; s#tmpxft_[0-9a-f]+_[0-9a-f]+-[0-9]+_#tmpxft_<id>_#g" "$name.out"
    echo "exit code: $rc" >> "$name.log"
    return $rc
}
expect_fail() {  # the step's own command must fail; record that it did
    if [ "$1" = 0 ]; then
        echo "result:    UNEXPECTED SUCCESS (this step was expected to fail)" >> "$2.log"; status=1
    else
        echo "result:    failed as expected (the failure is the lesson)" >> "$2.log"
    fi
}
HIPV="$(hipcc --version 2>/dev/null | head -n 1)"
NVV="$(nvcc --version | tail -n 2 | head -n 1)"
PYV="Python $(python3 -c 'import sys; print(sys.version.split()[0])')"
COMPILED="compiled only: no GPU is needed to read generated code"
NOGPU="untested on hardware: the build container has no GPU (AH-26); the build is real, the run shows the runtime's own error"
HDR=/usr/include/hip/nvidia_detail/nvidia_hip_runtime_api.h
NVBUILD="HIP_PLATFORM=nvidia CUDA_PATH=/usr hipcc -std=c++17 -x cu"
ISA=../F7-07/isa_of.sh
mkdir -p gen

step tools "-" "$HIPV; $NVV" "-" \
  "hipcc --version 2>/dev/null | head -n 2; echo \"hipconfig --platform: \$(hipconfig --platform)\"; nvcc --version | tail -n 2 | head -n 1; echo; for t in hipcc nvcc hipify-perl hipify-clang roc-obj-ls rocminfo rocprof rocprofv3 compute-sanitizer cuobjdump nvdisasm; do if command -v \$t > /dev/null; then printf '%-18s installed\n' \$t; else printf '%-18s not installed\n' \$t; fi; done" || status=1

step libs "-" "$(uname -sr); dpkg-query" "-" \
  "for n in cublas cub thrust hipblas hipcub rocblas rocprim rocthrust; do m=\$(ls -d /usr/include/\$n /usr/include/\$n.h /usr/include/\${n}_v2.h /usr/include/\$n/ /usr/include/hip/\$n* /opt/rocm*/include/\$n* 2>/dev/null | sort -u | tr '\n' ' '); printf '%-10s %s\n' \$n \"\${m:-(no header or folder of this name)}\"; done; echo; echo 'installed packages whose name contains blas, cub, prim or thrust:'; dpkg-query -W -f='\${Package} \${Version}\n' 2>/dev/null | grep -E 'blas|cub|prim|thrust' | grep -vE 'openblas|libblas|liblapack|libcublas[a-z]*-?dev' ; true" || status=1

step hipmap "../F7-03/hipmap.py, $HDR" "$PYV; header from libamdhip64-dev $(dpkg-query -W -f='${Version}' libamdhip64-dev 2>/dev/null)" "-" \
  "python3 -I ../F7-03/hipmap.py $HDR hipGetDeviceCount hipMalloc hipMemcpy hipMemcpyHostToDevice hipMemcpyDeviceToHost hipMemset hipGetLastError hipFree hipGetErrorName hipGetErrorString hipGetDeviceProperties hipStreamCreate hipEventCreate hipEventRecord hipEventSynchronize hipEventElapsedTime hipHostMalloc" || status=1

step hipify "../F7-04/toyhipify.py, saxpy_neg.cu" "$PYV; header from libamdhip64-dev $(dpkg-query -W -f='${Version}' libamdhip64-dev 2>/dev/null)" "-" \
  "python3 -I ../F7-04/toyhipify.py $HDR saxpy_neg.cu gen/saxpy_neg.hip && echo && echo 'diff saxpy_neg.cu gen/saxpy_neg.hip:' && diff saxpy_neg.cu gen/saxpy_neg.hip; true" || status=1

step port_amd "gen/saxpy_neg.hip" "$HIPV (offload target gfx90a)" "$COMPILED" \
  "hipcc -std=c++17 -O2 --offload-arch=gfx90a gen/saxpy_neg.hip -o .tmp_port"
expect_fail $? port_amd

step port_nvidia "gen/saxpy_neg.hip" "$HIPV with HIP_PLATFORM=nvidia; $NVV" "$NOGPU" \
  "$NVBUILD gen/saxpy_neg.hip -o .tmp_port_nv && echo 'build succeeded' && ./.tmp_port_nv"

sed 's/__ballot_sync(0xffffffffu, negative)/__ballot(negative)/' gen/saxpy_neg.hip > gen/saxpy_neg_naive.hip
step naive_amd "gen/saxpy_neg_naive.hip" "$HIPV (offload target gfx90a)" "$NOGPU" \
  "echo 'diff gen/saxpy_neg.hip gen/saxpy_neg_naive.hip:'; diff gen/saxpy_neg.hip gen/saxpy_neg_naive.hip; hipcc -std=c++17 -O2 --offload-arch=gfx90a gen/saxpy_neg_naive.hip -o .tmp_naive && echo 'build succeeded, no warnings' && ./.tmp_naive"

step naive_warn "gen/saxpy_neg_naive.hip" "$HIPV (offload target gfx90a)" "$COMPILED" \
  "hipcc -std=c++17 -O2 -Wshorten-64-to-32 --offload-arch=gfx90a -c gen/saxpy_neg_naive.hip -o .tmp_naive.o" || status=1

step naive_nvidia "gen/saxpy_neg_naive.hip" "$HIPV with HIP_PLATFORM=nvidia; $NVV" "$COMPILED" \
  "$NVBUILD -c gen/saxpy_neg_naive.hip -o .tmp_naive_nv.o && echo 'build succeeded (with the warnings above)'" || status=1

step fixed_nvidia "saxpy_neg_fixed.hip" "$HIPV with HIP_PLATFORM=nvidia; $NVV" "$NOGPU" \
  "$NVBUILD saxpy_neg_fixed.hip -o .tmp_fixed_nv && echo 'build succeeded' && ./.tmp_fixed_nv"

step isa_naive "gen/saxpy_neg_naive.hip, $ISA" "$HIPV" "$COMPILED" \
  "$ISA gen/saxpy_neg_naive.hip gfx90a" || status=1
step isa_fixed "saxpy_neg_fixed.hip, $ISA" "$HIPV" "$COMPILED" \
  "$ISA saxpy_neg_fixed.hip gfx90a" || status=1
step isa_summary "gen/saxpy_neg_naive.hip, saxpy_neg_fixed.hip, $ISA" "$HIPV" "$COMPILED" \
  "for c in 'gen/saxpy_neg_naive.hip gfx90a' 'saxpy_neg_fixed.hip gfx90a' 'saxpy_neg_fixed.hip gfx1030'; do set -- \$c; echo \"== \$1 for \$2\"; $ISA \$1 \$2 | grep -E 'v_and_b32_e32 v0, (31|63)|s_bcnt1|s_mul_i32 s0|global_atomic_add|v_fmac_f32|wavefront_size' | sed -E 's/^[[:space:]]+//'; done" || status=1

step ptx_summary "saxpy_neg.cu, saxpy_neg_fixed.hip" "$NVV; $HIPV with HIP_PLATFORM=nvidia" "$COMPILED" \
  "nvcc -std=c++17 -O2 -arch=sm_80 -ptx saxpy_neg.cu -o .tmp_orig.ptx && $NVBUILD -O2 -arch=sm_80 -ptx saxpy_neg_fixed.hip -o .tmp_fixed.ptx && for f in orig fixed; do echo \"== \$f (sm_80)\"; grep -E 'vote|popc|and.b32.*31|WARP_SZ|rem.u32|atom' .tmp_\$f.ptx | sed -E 's/^[[:space:]]+//; s/[[:space:]]+/ /g'; done" || status=1

step lds "reverse_lds.hip" "$HIPV; $NVV" "$COMPILED" \
  "w=\$(mktemp -d) && cp reverse_lds.hip \$w/k.hip && (cd \$w && hipcc -std=c++17 -O2 --offload-arch=gfx90a --save-temps -c k.hip -o k.o 2> /dev/null) && echo '== AMD ISA, gfx90a' && grep -E '^\s+(ds_|s_barrier)|^\s+\.group_segment_fixed_size' \$w/k-hip-amdgcn-amd-amdhsa-gfx90a.s | sed -E 's/^[[:space:]]+//; s/[[:space:]]+/ /g' && $NVBUILD -arch=sm_80 -ptx reverse_lds.hip -o \$w/k.ptx && echo '== PTX, sm_80 (NVIDIA back end)' && grep -E 'shared|bar.sync' \$w/k.ptx | sed -E 's/^[[:space:]]+//; s/[[:space:]]+/ /g'; rc=\$?; rm -rf \$w; exit \$rc" || status=1

step cublas_hipify "extra/uses_cublas.cu, ../F7-04/toyhipify.py" "$PYV; $HIPV (offload target gfx90a); $NVV" "$COMPILED" \
  "nvcc -std=c++17 -O2 extra/uses_cublas.cu -lcublas -o .tmp_cublas && echo 'nvcc -lcublas build of the CUDA original: succeeded' && echo && python3 -I ../F7-04/toyhipify.py $HDR extra/uses_cublas.cu gen/uses_cublas.hip && echo && echo 'library lines left in gen/uses_cublas.hip:' && grep -n 'cublas' gen/uses_cublas.hip && echo && echo 'hipcc for gfx90a (first 6 lines of its messages):' && hipcc -std=c++17 -O2 --offload-arch=gfx90a gen/uses_cublas.hip -o .tmp_ucb > .tmp_ucb.txt 2>&1; rc=\$?; head -n 6 .tmp_ucb.txt; echo \"... (\$(grep -c 'error:' .tmp_ucb.txt) lines containing 'error:' in total)\"; exit \$rc"
expect_fail $? cublas_hipify

rm -f .tmp_naive_nv.o .tmp_port .tmp_port_nv .tmp_naive .tmp_naive.o .tmp_fixed_nv .tmp_orig.ptx .tmp_fixed.ptx .tmp_cublas .tmp_ucb .tmp_ucb.txt
exit $status
