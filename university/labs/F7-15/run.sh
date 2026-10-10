#!/usr/bin/env bash
# F7-15 extra lab steps (compile only: no GPU is needed to read generated code).
#   isa_scan   - key instructions of blockScan for gfx90a (wave64) and gfx1100 (wave32)
#   hip_nvidia - Listing 1 built through HIP's NVIDIA path (nvcc + HIP headers) and run
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
    if [ "$rc" -ne 0 ] && [ "$name" != "hip_nvidia" ]; then status=1; fi
}
NVCC="$(nvcc --version | tail -n 2 | head -n 1)"
CLANG="$(clang++-17 --version | head -n 1) (the compiler that hipcc drives; HIP 5.7)"
COMPILED="compiled only: no GPU is needed to read generated code"

step isa_scan "block_scan.hip" "$CLANG" "$COMPILED" \
  "for t in gfx90a gfx1100; do hipcc -std=c++17 -O2 --offload-arch=\$t --cuda-device-only -S block_scan.hip -o .tmp.s 2>/dev/null || exit 1; echo \"--- \$t: instruction counts in blockScan\"; awk '/^_Z9blockScanPKiPiS1_i:/,/s_endpgm/' .tmp.s | grep -o -E '^[[:space:]]+(ds_bpermute_b32|ds_swizzle_b32|s_barrier|ds_write_b32|ds_store_b32|ds_read_b32|ds_load_b32|global_load_dword|global_load_b32|global_store_dword|global_store_b32)' | sort | uniq -c | sed -E 's/^ +//'; grep -E '^[[:space:]]+\.(group_segment_fixed_size|vgpr_count|wavefront_size):' .tmp.s | sed -E 's/^[[:space:]]+//'; done"
step hip_nvidia "block_scan.hip" "$NVCC (with the HIP 5.7 headers, NVIDIA platform)" \
  "untested on hardware: the build container has no NVIDIA GPU (AH-26); the build is real, the run shows the runtime's own error" \
  "nvcc -std=c++17 -O2 -arch=sm_80 -x cu -D__HIP_PLATFORM_NVIDIA__ block_scan.hip -o .tmp_nv && ./.tmp_nv"
rm -f .tmp.s .tmp_nv
exit $status
