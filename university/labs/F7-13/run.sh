#!/usr/bin/env bash
# F7-13 extra lab steps (compile only: no GPU is needed to read generated code).
#   dot_isa     - the whole gfx90a assembly of dot() (comments removed)
#   loop_fma    - the loop body of dotFma(): the predicted change, checked
#   census      - tools/isa_census.cpp run on the assembly of both kernels
#   code_object - the same code as an ELF code object: symbols, disassembly with encodings, metadata
#   kd          - the kernel-descriptor directives (.amdhsa_*) the compiler wrote for dot()
#   bundle      - the code objects inside the complete host executable (roc-obj-ls; first two columns)
#   options     - two predicted changes: -munsafe-fp-atomics (the atomic) and -ffp-contract=fast (the loop arithmetic)
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
    sed -i "s#$(pwd)/##g" "$name.out"
    echo "exit code: $rc" >> "$name.log"
    if [ "$rc" -ne 0 ]; then status=1; fi
}
GXX="$(g++ --version | head -n 1)"
CLANG="$(clang++-17 --version | head -n 1) (the compiler that hipcc drives; HIP 5.7)"
COMPILED="compiled only: no GPU is needed to read generated code"
ISA="grep -v -E '^[[:space:]]*;' | sed -E 's/[[:space:]]*;.*$//'"
S="hipcc -std=c++17 -O2 --offload-arch=gfx90a --cuda-device-only -S dot.hip -o .tmp.s 2>/dev/null"

step dot_isa "dot.hip" "$CLANG" "$COMPILED" \
  "$S && awk '/^_Z3dot.*:/,/s_endpgm/' .tmp.s | $ISA | cat -n"
step loop_fma "dot.hip" "$CLANG" "$COMPILED" \
  "$S && awk '/^_Z6dotFma.*:/,/s_endpgm/' .tmp.s | $ISA | awk '/^.LBB1_2:/,/s_cbranch_execnz/'"
step census "tools/isa_census.cpp on the gfx90a assembly of dot.hip" "$GXX; $CLANG" "$COMPILED" \
  "g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined tools/isa_census.cpp -o .bin_census && $S && ./.bin_census < .tmp.s"
step code_object "dot.hip" "$CLANG; $(llvm-objdump-17 --version | grep -m1 'LLVM version' | sed 's/^ *//') (llvm-objdump-17)" "$COMPILED" \
  "hipcc -std=c++17 -O2 --offload-arch=gfx90a --cuda-device-only --no-gpu-bundle-output -c dot.hip -o .tmp.co 2>/dev/null && file -b .tmp.co && echo '--- symbols' && llvm-readelf-17 -s .tmp.co | grep -E '_Z3dot' && echo '--- first instructions of dot() with their encodings' && llvm-objdump-17 -d .tmp.co | awk '/<_Z3dot[^F]*>:/,0' | sed -n '2,13p' | sed -E 's/[[:space:]]+\/\/ /  \/\/ /' && echo '--- selected metadata of dot()' && llvm-readelf-17 -n .tmp.co | awk '/^  - \.agpr_count:/{n++} n==1' | grep -E '^[[:space:]]+-? ?\.(name|sgpr_count|vgpr_count|agpr_count|wavefront_size|group_segment_fixed_size|private_segment_fixed_size|kernarg_segment_size|max_flat_workgroup_size|sgpr_spill_count|vgpr_spill_count):'"
step kd "dot.hip" "$CLANG" "$COMPILED" \
  "$S && awk '/\.amdhsa_kernel _Z3dot[^F]/,/\.end_amdhsa_kernel/' .tmp.s | grep -E 'amdhsa_(kernel|group_segment|private_segment_fixed|kernarg_size|user_sgpr_count|next_free_vgpr|next_free_sgpr|accum_offset|wavefront_size32|system_vgpr_workitem_id|float_round|ieee_mode|tg_split)' "
step bundle "dot.hip" "$CLANG; roc-obj-ls from the container's ROCm packages" "$COMPILED" \
  "hipcc -std=c++17 -O2 --offload-arch=gfx90a --offload-arch=gfx1100 dot.hip -o .tmp.exe 2>/dev/null && echo 'count  bundle entry id (columns 1-2 of roc-obj-ls; column 3, the file location, omitted)' && roc-obj-ls .tmp.exe | awk '{print \$1\"      \"\$2}'"
step options "dot.hip" "$CLANG" "$COMPILED" \
  "for f in '' '-munsafe-fp-atomics' '-ffp-contract=fast'; do echo \"--- dot() with options: \${f:-(none)}\"; hipcc -std=c++17 -O2 --offload-arch=gfx90a --cuda-device-only \$f -S dot.hip -o .tmp.s 2>/dev/null; awk '/^_Z3dot.*:/,/s_endpgm/' .tmp.s | $ISA > .tmp.k; printf 'global_atomic_cmpswap: %s   global_atomic_add_f32: %s\\n' \$(grep -c global_atomic_cmpswap .tmp.k) \$(grep -c global_atomic_add_f32 .tmp.k); printf 'in loop .LBB0_2: v_pk_mul_f32: %s   v_fma*/v_fmac*: %s   v_add_f32: %s\\n' \$(awk '/^.LBB0_2:/,/s_cbranch_execnz/' .tmp.k | grep -c v_pk_mul_f32) \$(awk '/^.LBB0_2:/,/s_cbranch_execnz/' .tmp.k | grep -c -E 'v_fma|v_fmac|v_pk_fma') \$(awk '/^.LBB0_2:/,/s_cbranch_execnz/' .tmp.k | grep -c v_add_f32); done"
rm -f .tmp.s .tmp.k .tmp.co .tmp.exe .bin_census
exit $status
