#!/usr/bin/env bash
# F6-07 extra lab steps (no GPU needed):
#   header_fields - the CUDA 12.0 header's own description of each field the report prints
#   gpu_code      - the real GPU targets this nvcc can generate machine code for
#   fatbin_sm90   - what is inside an object built only for sm_90 (forensic evidence)
#   fatbin_fixed  - the same with PTX kept as well (forensic fix)
#   header_versions - the header's words for the two version numbers the report prints
#   header_noimage - the header's words for the error in the forensic lab
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
HDR="CUDA 12.0 header, Ubuntu package nvidia-cuda-dev $(dpkg-query -W -f='${Version}' nvidia-cuda-dev 2>/dev/null)"
COMPILED="compiled only: no GPU is needed to read generated code"
FIELDS='name|major|minor|multiProcessorCount|warpSize|totalGlobalMem|maxThreadsPerBlock|maxThreadsDim|maxGridSize|sharedMemPerBlock|regsPerBlock|totalConstMem|l2CacheSize|memoryBusWidth|asyncEngineCount|unifiedAddressing|ECCEnabled|pciBusID'
step header_fields "(header) /usr/include/driver_types.h, struct cudaDeviceProp" "$HDR" "-" \
  "awk '/^struct __device_builtin__ cudaDeviceProp/,/^}/' /usr/include/driver_types.h | grep -E '^ +[a-zA-Z_]+ +($FIELDS)(\\[[0-9]+\\])?;' | sed -E 's/^ +//; s/ +/ /g'"
step gpu_code "(tool) nvcc --list-gpu-code" "$NVCC" "-" "nvcc --list-gpu-code"
step fatbin_sm90 "../F6-04/saxpy.cu" "$NVCC" "$COMPILED" \
  "nvcc -gencode arch=compute_90,code=sm_90 -c ../F6-04/saxpy.cu -o .tmp.o && (cuobjdump --list-elf .tmp.o; cuobjdump --list-ptx .tmp.o) 2>&1 | sed -E 's/tmpxft_[0-9a-f_]+-/TMP-/'"
step fatbin_fixed "../F6-04/saxpy.cu" "$NVCC" "$COMPILED" \
  "nvcc -gencode arch=compute_80,code=sm_80 -gencode arch=compute_90,code=sm_90 -gencode arch=compute_90,code=compute_90 -c ../F6-04/saxpy.cu -o .tmp.o && (cuobjdump --list-elf .tmp.o; cuobjdump --list-ptx .tmp.o) 2>&1 | sed -E 's/tmpxft_[0-9a-f_]+-/TMP-/'"
step header_noimage "(header) /usr/include/driver_types.h" "$HDR" "-" \
  "for e in cudaErrorNoKernelImageForDevice cudaErrorInsufficientDriver; do awk -v e=\"\$e\" '/\\/\\*\\*/{buf=\"\"} {buf=buf \$0 \"\\n\"} \$0 ~ \"^ *\" e \" *=\" {printf \"%s\\n\", buf}' /usr/include/driver_types.h | sed -E 's/^ +//'; done"
step header_versions "(header) /usr/include/cuda_runtime_api.h" "$HDR" "-" \
  "awk '/brief Returns the latest version of CUDA supported by the driver/,/driverVersion is NULL/' /usr/include/cuda_runtime_api.h | sed -E 's/^ \\* ?//' && awk '/brief Returns the CUDA Runtime version/,/CUDA Toolkit version in the above format/' /usr/include/cuda_runtime_api.h | sed -E 's/^ \\* ?//'"
rm -f .tmp.o
exit $status
