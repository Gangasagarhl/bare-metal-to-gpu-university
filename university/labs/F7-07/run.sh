#!/usr/bin/env bash
# F7-07 extra lab steps (no GPU needed):
#   header_lines          - how HIP 5.7's headers define warpSize and __shfl_xor on each back end
#   warp_size_isa         - warpSize compiled for gfx90a and for gfx1030: the constant in the ISA
#   warp_size_ptx         - warpSize compiled through the NVIDIA back end: what PTX reads
#   shuffle_count         - shuffle instructions per kernel and target (AMD ISA and NVIDIA PTX)
#   wave_sum_nvidia       - wave_sum.hip built through the NVIDIA back end, then run
#   wave_sum_fixed_nvidia - wave_sum_fixed.hip built through the NVIDIA back end, then run
#   nvidia_driver         - what hipcc really runs on the NVIDIA platform (HIPCC_VERBOSE=1)
#   vec_add_ptx           - F7-05's vec_add.hip through the NVIDIA back end: PTX and SASS of vecAdd
#   nvidia_fatbin         - what is inside the executables: HIP-on-NVIDIA versus plain nvcc
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
    sed -i "s#$(pwd)/##g; s#file://##g" "$name.out"
    echo "exit code: $rc" >> "$name.log"
    return $rc
}
HIPV="$(hipcc --version 2>/dev/null | head -n 1)"
NVV="$(nvcc --version | tail -n 2 | head -n 1)"
COMPILED="compiled only: no GPU is needed to read generated code"
NOGPU="untested on hardware: the build container has no GPU (AH-26); the build is real, the run shows the runtime's own error"
NVBUILD="HIP_PLATFORM=nvidia CUDA_PATH=/usr hipcc -std=c++17 -x cu"
step header_lines "/usr/include/hip/amd_detail/amd_warp_functions.h, /usr/include/hip/nvidia_detail/nvidia_hip_runtime_api.h" \
  "headers from libamdhip64-dev $(dpkg-query -W -f='${Version}' libamdhip64-dev 2>/dev/null)" "-" \
  "cd /usr/include/hip && grep -n 'static constexpr int warpSize' amd_detail/amd_warp_functions.h | sed 's#^#amd_detail/amd_warp_functions.h:#' && grep -n -A5 '^int __shfl_xor(int var' amd_detail/amd_warp_functions.h | sed 's#^#amd_detail/amd_warp_functions.h:#' && grep -n -A1 '^unsigned long long int __ballot(int predicate)' amd_detail/amd_device_functions.h | sed 's#^#amd_detail/amd_device_functions.h:#' && grep -n 'define __shfl_xor\|define __ballot' nvidia_detail/nvidia_hip_runtime_api.h | sed 's#^#nvidia_detail/nvidia_hip_runtime_api.h:#'" || status=1
step warp_size_isa "warp_size.hip" "$HIPV" "$COMPILED" \
  "for a in gfx90a gfx1030; do echo \"== --offload-arch=\$a\"; ./isa_of.sh warp_size.hip \$a | grep -E 'target:|v_mov_b32|global_store|wavefront_size'; done" || status=1
step warp_size_ptx "warp_size.hip" "$HIPV with HIP_PLATFORM=nvidia; $NVV" "$COMPILED" \
  "$NVBUILD -arch=sm_80 -ptx warp_size.hip -o .tmp_ws.ptx && sed -n '/.entry _Z13writeWarpSizePi/,/^}/p' .tmp_ws.ptx | grep -vE '^\s*$'" || status=1
step shuffle_count "wave_sum.hip, wave_sum_fixed.hip" "$HIPV; $NVV" "$COMPILED" \
  "for f in wave_sum wave_sum_fixed; do for a in gfx90a gfx1030; do printf '%-15s %-8s ds_bpermute_b32 count: %s\n' \$f.hip \$a \$(./isa_of.sh \$f.hip \$a | grep -c ds_bpermute_b32); done; $NVBUILD -arch=sm_80 -ptx \$f.hip -o .tmp_\$f.ptx; printf '%-15s %-8s shfl.sync.bfly count:  %s\n' \$f.hip sm_80 \$(grep -c 'shfl.sync.bfly' .tmp_\$f.ptx); done; echo; echo 'PTX shuffle lines of wave_sum.hip (sm_80):'; grep 'shfl.sync.bfly' .tmp_wave_sum.ptx | sed -E 's/^[[:space:]]+//'" || status=1
step wave_sum_nvidia "wave_sum.hip" "$HIPV with HIP_PLATFORM=nvidia; $NVV" "$NOGPU" \
  "$NVBUILD wave_sum.hip -o .tmp_wsum_nv && ./.tmp_wsum_nv"
step wave_sum_fixed_nvidia "wave_sum_fixed.hip" "$HIPV with HIP_PLATFORM=nvidia; $NVV" "$NOGPU" \
  "$NVBUILD wave_sum_fixed.hip -o .tmp_wfix_nv && ./.tmp_wfix_nv"
VA=../F7-05/vec_add.hip
step nvidia_driver "$VA" "$HIPV with HIP_PLATFORM=nvidia; $NVV" "$COMPILED" \
  "HIPCC_VERBOSE=1 $NVBUILD $VA -o .tmp_va_nv 2>&1 | sed -E 's/^hipcc-cmd: //' | tr -s ' '" || status=1
step vec_add_ptx "$VA" "$HIPV with HIP_PLATFORM=nvidia; $NVV" "$COMPILED" \
  "$NVBUILD -arch=sm_80 -ptx $VA -o .tmp_va.ptx && echo '== PTX (sm_80)' && sed -n '/^\t*mov.u32/,/^}/p' .tmp_va.ptx | grep -vE '^\s*$' && $NVBUILD -arch=sm_80 -cubin $VA -o .tmp_va.cubin && echo '== SASS (sm_80)' && cuobjdump -sass .tmp_va.cubin | grep -E '^\s+/\*[0-9a-f]{4}\*/' | grep -vE 'NOP|BRA 0x' | sed -E 's#^\s+/\*([0-9a-f]{4})\*/\s+#\1  #; s#\s*/\* 0x[0-9a-f]+ \*/##; s#\s+;#;#'" || status=1
step nvidia_fatbin "$VA, ../F7-04/saxpy.cu" "$HIPV with HIP_PLATFORM=nvidia; $NVV" "$COMPILED" \
  "$NVBUILD -gencode arch=compute_80,code=sm_80 -gencode arch=compute_86,code=sm_86 -gencode arch=compute_86,code=compute_86 $VA -o .tmp_va2 && echo '== HIP on NVIDIA, vec_add.hip, sm_80 + sm_86 + PTX:' && cuobjdump --list-elf .tmp_va2 | sed -E 's/tmpxft_[0-9a-f]+_[0-9]+-//' && cuobjdump --list-ptx .tmp_va2 && echo 'shared GPU runtime libraries (ldd):' && (ldd .tmp_va2 | grep -E 'libcudart|libamdhip64' | awk '{print \$1}' | grep . || echo '(none listed)') && nvcc -std=c++17 -gencode arch=compute_80,code=sm_80 -gencode arch=compute_86,code=sm_86 -gencode arch=compute_86,code=compute_86 ../F7-04/saxpy.cu -o .tmp_sx && echo '== plain nvcc, saxpy.cu, same -gencode options:' && cuobjdump --list-elf .tmp_sx | sed -E 's/tmpxft_[0-9a-f]+_[0-9]+-//' && cuobjdump --list-ptx .tmp_sx && echo 'shared GPU runtime libraries (ldd):' && (ldd .tmp_sx | grep -E 'libcudart|libamdhip64' | awk '{print \$1}' | grep . || echo '(none listed)')" || status=1
rm -f .tmp_va_nv .tmp_va.ptx .tmp_va.cubin .tmp_va2 .tmp_sx
rm -f .tmp_ws.ptx .tmp_wave_sum.ptx .tmp_wave_sum_fixed.ptx .tmp_wsum_nv .tmp_wfix_nv
exit $status
