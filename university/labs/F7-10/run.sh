#!/usr/bin/env bash
# F7-10 extra lab steps (compile only: no GPU is needed to read the compiler's reports).
#   resources  - compiler resource report for every accum<K> and accumLB<K> of pressure.hip (gfx90a)
#   occ_check  - the occupancy calculator (occ.cpp) fed with those reports: predicted versus reported
#   spill_isa  - how the compiler keeps values it cannot fit: AGPR copies and scratch instructions
#   forensic   - evidence pack: resource report and ISA census of fir<16> and fir<32>
#   taps_isa   - the scalar and vector loads of fir<16>: uniform taps go to SGPRs, the window to VGPRs
#   forensic_fix - resource report of the fix candidates firLB<32,6> and firLB<32,8> (fir_fix.hip)
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
COMPILED="compiled only: no GPU is needed to read the compiler's reports"
REMARKS="grep -E 'Function Name|SGPRs:|VGPRs:|AGPRs:|ScratchSize|Occupancy|VGPRs Spill|LDS Size' | sed -E 's/.*remark: +//; s/ \[-R.*//; s/ \[bytes\/lane\]//; s/ \[waves\/SIMD\]//; s/ \[bytes\/block\]//' | paste -d ' ' - - - - - - - -"
# demangle _Z5accumILi8EEvPKfPfii -> accum<8>
DEM="sed -E 's/Function Name: _Z[0-9]+([A-Za-z]+)ILi([0-9]+)E[^ ]*/\1<\2>/'"

step resources "pressure.hip" "$CLANG" "$COMPILED" \
  "hipcc -std=c++17 -O2 --offload-arch=gfx90a --cuda-device-only -c pressure.hip -o /dev/null -Rpass-analysis=kernel-resource-usage 2>&1 | $REMARKS | $DEM | sed -E 's/  +/ /g'"
step occ_check "occ.cpp, fed with the compiler's numbers for pressure.hip" "$GXX; $CLANG" "$COMPILED" \
  "g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined occ.cpp -o .bin_occ && hipcc -std=c++17 -O2 --offload-arch=gfx90a --cuda-device-only -S pressure.hip -o .tmp.s 2>/dev/null && awk '/^_Z[0-9]+accum/ && /:/ {split(\$1,a,\":\"); name=a[1]} /; TotalNumVgprs:/ {v=\$3} /; LDSByteSize:/ {l=\$3} /; Occupancy:/ {print name, v, l, (name ~ /accumLB/ ? 256 : 1024), \$3}' .tmp.s | sed -E 's/^_Z[0-9]+([A-Za-z]+)ILi([0-9]+)E[^ ]*/\1<\2>/' | ./.bin_occ"
step spill_isa "pressure.hip" "$CLANG" "$COMPILED" \
  "hipcc -std=c++17 -O2 --offload-arch=gfx90a --cuda-device-only -S pressure.hip -o .tmp.s 2>/dev/null && for k in _Z5accumILi128EEvPKfPfii _Z5accumILi256EEvPKfPfii _Z7accumLBILi256EEvPKfPfii; do echo \"--- \$k\"; awk -v k=\"\$k:\" '\$1==k,/s_endpgm/' .tmp.s > .tmp.k; printf 'stores marked Spill by the compiler:  %s\n' \$(grep -c 'Spill' .tmp.k); printf 'loads marked Reload by the compiler:  %s\n' \$(grep -c 'Reload' .tmp.k); printf 'v_accvgpr_write (VGPR -> AGPR):       %s\n' \$(grep -c 'v_accvgpr_write' .tmp.k); printf 'v_accvgpr_read  (AGPR -> VGPR):       %s\n' \$(grep -c 'v_accvgpr_read' .tmp.k); grep -m 2 -E 'Spill|v_accvgpr_write' .tmp.k | sed -E 's/^[[:space:]]+/    /'; done"
step forensic "fir.hip" "$CLANG" "$COMPILED" \
  "hipcc -std=c++17 -O2 --offload-arch=gfx90a --cuda-device-only -c fir.hip -o /dev/null -Rpass-analysis=kernel-resource-usage 2>&1 | $REMARKS | sed -E 's/Function Name: _Z3firILi([0-9]+)E[^ ]*/fir<\1>/; s/  +/ /g'; hipcc -std=c++17 -O2 --offload-arch=gfx90a --cuda-device-only -S fir.hip -o .tmp.s 2>/dev/null; for k in 16 32; do awk -v k=\"_Z3firILi\${k}EEvPKfS1_Pfi:\" '\$1==k,/s_endpgm/' .tmp.s | sed -E 's/[[:space:]]*;.*$//' > .tmp.k; echo \"--- ISA census of fir<\$k>\"; printf 'instructions:                %s\n' \$(grep -c -E '^[[:space:]]+[a-z_0-9]+' .tmp.k); printf 'global_load (any width):     %s\n' \$(grep -c 'global_load' .tmp.k); printf 's_load (scalar loads):       %s\n' \$(grep -c 's_load' .tmp.k); printf 'v_fmac/v_fma (multiply-add): %s\n' \$(grep -c -E 'v_fmac|v_fma_f32|v_pk_fma' .tmp.k); printf 'highest VGPR index written:  v%s\n' \$(grep -o -E '\bv\[?[0-9]+' .tmp.k | grep -o -E '[0-9]+' | sort -n | tail -1); printf 'first multiply-add at instruction: %s\n' \$(grep -E '^[[:space:]]+[a-z_0-9]+' .tmp.k | grep -n -m1 -E 'v_fmac|v_fma_f32|v_pk_fma' | cut -d: -f1); printf 'global_load before it:       %s\n' \$(grep -E '^[[:space:]]+[a-z_0-9]+' .tmp.k | awk '/v_fmac|v_fma_f32|v_pk_fma/ {exit} /global_load/ {c++} END {print c+0}'); done"
step taps_isa "fir.hip" "$CLANG" "$COMPILED" \
  "hipcc -std=c++17 -O2 --offload-arch=gfx90a --cuda-device-only -S fir.hip -o .tmp.s 2>/dev/null && awk '\$1==\"_Z3firILi16EEvPKfS1_Pfi:\",/s_endpgm/' .tmp.s | sed -E 's/[[:space:]]*;.*$//' | grep -E 's_load|global_load'"
step forensic_fix "fir_fix.hip" "$CLANG" "$COMPILED" \
  "hipcc -std=c++17 -O2 --offload-arch=gfx90a --cuda-device-only -c fir_fix.hip -o /dev/null -Rpass-analysis=kernel-resource-usage 2>&1 | $REMARKS | sed -E 's/Function Name: _Z5firLBILi([0-9]+)ELi([0-9]+)E[^ ]*/firLB<\1,\2>/; s/  +/ /g'"
rm -f .tmp.s .tmp.k .bin_occ
exit $status
