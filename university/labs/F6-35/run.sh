#!/usr/bin/env bash
# F6-35 extra lab steps.
#   sgemm_cublas - Listing 5 built with -lcublas (the runner's own nvcc line does not link cuBLAS),
#                  then run (no GPU: the run shows the library's own error)
#   cub_resources - ptxas report of the CUB BlockReduce kernel for sm_80 (compiled only)
#   lib_versions  - versions of the installed library packages, as the package manager prints them
#   header_quotes - the CUB and cuBLAS header lines this chapter quotes (source H1, H2)
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
COMPILED="compiled only: no GPU is needed to read generated code"
NOGPU="untested on hardware: the build container has no NVIDIA GPU (AH-26); the build is real, the run shows the runtime's own error"
step sgemm_cublas "cublas/sgemm_cublas.cu" "$NVCC" "$NOGPU" \
  "nvcc -std=c++17 -O2 -lineinfo -Werror all-warnings cublas/sgemm_cublas.cu -lcublas -o .tmp_sg && ./.tmp_sg" may-fail
step cub_resources "cub_sum.cu" "$NVCC" "$COMPILED" \
  "nvcc -std=c++17 -arch=sm_80 -Xptxas -v -cubin cub_sum.cu -o .tmp.cubin 2>&1 | grep -E 'Compiling entry|Used' | grep -A1 'entry.*sumSquares' | sed -E 's/^ptxas info +: //'"
step thrust_v1 "thrust_device.cu (first version: the std::bad_alloc handler removed)" "$NVCC" "$NOGPU" \
  "sed '/catch (const std::bad_alloc/,/^    }\$/c\\    }' thrust_device.cu > .tmp_v1.cu && nvcc -std=c++17 -O2 .tmp_v1.cu -o .tmp_v1 && ./.tmp_v1" may-fail
step lib_versions "-" "dpkg-query" "read only: no GPU is needed" \
  "dpkg-query -W -f='\${Package} \${Version}\n' nvidia-cuda-dev libcublas12 libthrust-dev libcub-dev; dpkg-query -W 'libcudnn*' 2>&1 | head -n 1"
step header_quotes "/usr/include/cub/device/device_reduce.cuh, device_scan.cuh, block/block_reduce.cuh, /usr/include/cublas_v2.h, cublas_api.h" "$NVCC (headers of the same installation)" "read only: no GPU is needed" \
  "grep -n -m1 -A1 'When .nullptr., the' /usr/include/cub/device/device_reduce.cuh; grep -n -m1 -A3 'run-to-run. determinism' /usr/include/cub/device/device_reduce.cuh; grep -n -m1 -A1 'Results are not deterministic' /usr/include/cub/device/device_scan.cuh; grep -n -m1 'undefined in threads other than' /usr/include/cub/block/block_reduce.cuh; grep -n 'define cublasSgemm ' /usr/include/cublas_v2.h; grep -n -E 'CUBLAS_STATUS_NOT_INITIALIZED =|TF32 tensor cores|cublasGetStatusName|cublasSetMathMode|cublasSetStream_v2|define CUBLAS_VER_(MAJOR|MINOR|PATCH)' /usr/include/cublas_api.h"
rm -f .tmp_sg .tmp.cubin .tmp_v1.cu .tmp_v1
exit $status
