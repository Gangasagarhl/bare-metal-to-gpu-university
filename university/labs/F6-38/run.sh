#!/usr/bin/env bash
# F6-38 extra lab steps (no GPU needed).
#   forensic      - the fused-softmax model (Listing 3) in the shipped "frozen" mode on growing inputs
#   forensic_flat - the same bug on flat inputs: the case the tests used
#   ptx_softmax   - the instructions the fused softmax kernel uses for exp and the warp merge (sm_80 PTX)
#   resources     - ptxas register report of both kernels for sm_80
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
FLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
step forensic "fused_trace.cpp" "$GXX" "-" \
  "g++ $FLAGS fused_trace.cpp -o .tmp_ft && ./.tmp_ft < forensic.in"
step forensic_flat "fused_trace.cpp" "$GXX" "-" \
  "g++ $FLAGS fused_trace.cpp -o .tmp_ft && ./.tmp_ft < forensic_flat.in"
step ptx_softmax "softmax_fused.cu" "$NVCC" "$COMPILED" \
  "nvcc -std=c++17 -arch=sm_80 -ptx softmax_fused.cu -o .tmp.ptx && awk '/.entry _Z11softmaxRows/,/^}/' .tmp.ptx | grep -oE '(ex2\.approx[.a-z0-9]*|shfl\.sync\.[a-z.0-9]+|max\.f32|div\.[a-z.0-9]+|rcp\.[a-z.0-9]+|ld\.global[.a-z0-9]*|st\.global[.a-z0-9]*)' | sort | uniq -c"
step resources "softmax_fused.cu" "$NVCC" "$COMPILED" \
  "nvcc -std=c++17 -arch=sm_80 -Xptxas -v -cubin softmax_fused.cu -o .tmp.cubin 2>&1 | grep -E 'Compiling entry|Used' | sed -E 's/^ptxas info +: //'"
rm -f .tmp_ft .tmp.ptx .tmp.cubin
exit $status
