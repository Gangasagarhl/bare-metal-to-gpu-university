#!/usr/bin/env bash
# F0-80 extra lab steps (no GPU needed):
#   stdfloat_check - the lowp.hpp model against GCC's std::float16_t / std::bfloat16_t (needs -std=c++23)
#   fmad_ptx       - the PTX nvcc emits for a * x + b with its default setting and with --fmad=false
#   nvcc_fmad_help - what nvcc's own help text says about --fmad
#   reduce_ptx     - the floating-point instructions in the PTX of reduce.cu's kernel
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
GXX="$(g++ --version | head -n 1)"
NVCC="$(nvcc --version | tail -n 2 | head -n 1)"
COMPILED="compiled only: reading generated code needs no GPU"
step stdfloat_check "stdfloat_check.cc" "$GXX" "-" \
  "g++ -std=c++23 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined stdfloat_check.cc -o .tmp_sfc && ./.tmp_sfc"
step fmad_ptx "fma_demo.cuh" "$NVCC" "$COMPILED" \
  "for f in true false; do echo \"--- nvcc -x cu -ptx --fmad=\$f\"; nvcc -x cu -ptx --fmad=\$f fma_demo.cuh -o .tmp.ptx || exit 1; grep -E '^[[:space:]]*(fma|mul|add)\.[a-z]*\.?f32' .tmp.ptx | sed -E 's/^[[:space:]]+/    /'; done"
step nvcc_fmad_help "-" "$NVCC" "-" \
  "nvcc --help | grep -A3 -E '^--fmad '"
step reduce_ptx "reduce.cu" "$NVCC" "$COMPILED" \
  "nvcc -std=c++17 -O2 -ptx reduce.cu -o .tmp_r.ptx && grep -E '^[[:space:]]*(add|fma|mul)\.[a-z.]*f32' .tmp_r.ptx | sed -E 's/^[[:space:]]+/    /'"
rm -f .tmp_sfc .tmp.ptx .tmp_r.ptx
exit $status
