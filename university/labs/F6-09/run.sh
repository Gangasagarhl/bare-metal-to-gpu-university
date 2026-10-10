#!/usr/bin/env bash
# F6-09 extra lab steps:
#   sass_vector - the loads and stores of copyScalar and copyVector (compiled only)
#   forensic    - the bandwidth model on forensic.in (evidence pack)
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
GXX="$(g++ --version | head -n 1)"
COMPILED="compiled only: no GPU is needed to read generated code"
step sass_vector "bandwidth.cu" "$NVCC" "$COMPILED" \
  "nvcc -arch=sm_80 -cubin bandwidth.cu -o .tmp.cubin && for f in _Z10copyScalarPKfPfi _Z10copyVectorPK6float4PS_i; do echo \"--- \$f\"; cuobjdump -sass -fun \$f .tmp.cubin | grep -E 'LDG|STG' | sed -E 's#^[[:space:]]+/\*([0-9a-f]+)\*/[[:space:]]+(.*[^[:space:]])[[:space:]]*;.*#\1  \2#'; done"
step forensic "bw_model.cpp with forensic.in" "$GXX" "-" \
  "g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined bw_model.cpp -o .bin_forensic && ./.bin_forensic < forensic.in"
HDR="CUDA 12.0 header, Ubuntu package nvidia-cuda-dev $(dpkg-query -W -f='${Version}' nvidia-cuda-dev 2>/dev/null)"
step header_pinned "(header) /usr/include/cuda_runtime_api.h" "$HDR" "-" \
  "sed -n '/subsection MemcpySynchronousBehavior/,/subsection MemcpyAsynchronousBehavior/p' /usr/include/cuda_runtime_api.h | sed -E 's/^ \\* ?//' | grep -v -E '^(<ol>|</ol>|\\\\subsection.*)?\\s*$' && echo && awk '/brief Allocates page-locked memory on the host/{f=1} f{print} f&&/data exchange between host and device/{exit}' /usr/include/cuda_runtime_api.h | sed -E 's/^ \\* ?//'"
rm -f .tmp.cubin .bin_forensic
exit $status
