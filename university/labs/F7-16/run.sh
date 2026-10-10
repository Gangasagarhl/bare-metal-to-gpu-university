#!/usr/bin/env bash
# F7-16 extra lab steps (compile only: no GPU is needed to read generated code).
#   resources   - per-kernel metadata of Listing 1 for gfx90a: LDS bytes, VGPRs, SGPRs, spills
#   inner_loop  - instruction counts of sgemmLdsReg (gfx90a)
#   lds_limit   - a kernel asking for 4 bytes more LDS than the compiler allows (expected to fail)
#   kai_isa     - barrier count in Listing 1's sgemmLdsReg and in Kai's kernel (forensic lab)
set -u
cd "$(dirname "$0")"
status=0
step() {  # step <name> <listing> <toolchain line> <hardware note or -> <expect: ok|fail> <command>
    local name="$1" listing="$2" tc="$3" hw="$4" expect="$5" cmd="$6"
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
    sed -i -E '/argument unused during compilation/d' "$name.out"
    if [ "$expect" = "fail" ]; then
        if [ "$rc" -ne 0 ]; then
            echo "result:    compile failed as expected (messages saved in $name.out)" >> "$name.log"
        else
            echo "result:    UNEXPECTED SUCCESS (compile was expected to fail)" >> "$name.log"; status=1
        fi
    else
        echo "exit code: $rc" >> "$name.log"
        if [ "$rc" -ne 0 ]; then status=1; fi
    fi
}
CLANG="$(clang++-17 --version | head -n 1) (the compiler that hipcc drives; HIP 5.7)"
COMPILED="compiled only: no GPU is needed to read generated code"
HIPS="hipcc -std=c++17 -O2 --offload-arch=gfx90a --cuda-device-only -S"
META="grep -E '^[[:space:]]+\.(name|group_segment_fixed_size|vgpr_count|sgpr_count|vgpr_spill_count|sgpr_spill_count):' | sed -E 's/^[[:space:]]+//'"

step resources "sgemm_lds.hip" "$CLANG" "$COMPILED" ok \
  "$HIPS sgemm_lds.hip -o .tmp.s 2>/dev/null && cat .tmp.s | $META"
step inner_loop "sgemm_lds.hip" "$CLANG" "$COMPILED" ok \
  "$HIPS sgemm_lds.hip -o .tmp.s 2>/dev/null && awk '/^_Z11sgemmLdsRegPKfS0_Pfiii:/,/s_endpgm/' .tmp.s | grep -o -E '^[[:space:]]+(v_pk_fma_f32|v_fmac_f32_e32|v_fma_f32|ds_read_b128|ds_read_b96|ds_read_b64|ds_read2_b32|ds_read_b32|ds_write_b32|ds_write2_b32|global_load_dword|global_store_dword|s_barrier)' | sort | uniq -c | sed -E 's/^ +//'"
step lds_limit "lds_limit.hip.inc" "$CLANG" "$COMPILED" fail \
  "hipcc -x hip -std=c++17 -O2 --offload-arch=gfx90a --cuda-device-only -c lds_limit.hip.inc -o .tmp.o"
step kai_isa "sgemm_lds.hip, kai_kernel.inc" "$CLANG" "$COMPILED" ok \
  "$HIPS sgemm_lds.hip -o .tmp.s 2>/dev/null && echo \"sgemmLdsReg s_barrier count: \$(awk '/^_Z11sgemmLdsRegPKfS0_Pfiii:/,/s_endpgm/' .tmp.s | grep -c s_barrier)\" && hipcc -x hip -std=c++17 -O2 --offload-arch=gfx90a --cuda-device-only -S kai_kernel.inc -o .tmp2.s 2>/dev/null && echo \"kaiGemm     s_barrier count: \$(awk '/^_Z7kaiGemmPKfS0_Pfiii:/,/s_endpgm/' .tmp2.s | grep -c s_barrier)\" && cat .tmp2.s | $META"
rm -f .tmp.s .tmp2.s .tmp.o
exit $status
