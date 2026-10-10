#!/usr/bin/env bash
# F4-42 run.sh: evidence for the path of a HIP dispatch, produced without a GPU.
#   strace : the files and devices the HIP runtime opens when vecadd starts (no GPU here)
#   isa    : the AMD GPU code hipcc generates for vecAdd (gfx90a), with its kernel descriptor
set -u -o pipefail
cd "$(dirname "$0")"
status=0
HIPV="$(hipcc --version 2>/dev/null | head -n 1) (offload target gfx90a)"
rec() {   # rec <name> <listing> <toolchain> <command> <exit code> [extra lines]
    local name="$1" listing="$2" tool="$3" cmd="$4" rc="$5"; shift 5
    { echo "listing:   $listing"; echo "toolchain: $tool"; echo "command:   $cmd"
      echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"; echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
      echo "exit code: $rc"; for e in "$@"; do echo "$e"; done; } > "$name.log"
}

hipcc -std=c++17 -O2 --offload-arch=gfx90a vecadd.hip -o .vecadd 2> .hb.txt || { cat .hb.txt; status=1; }
strace -f -o .st.txt -e trace=openat ./.vecadd > .prog.txt 2>&1; rc=$?
{ echo "== openat calls that touch devices, sysfs GPU topology or GPU device files =="
  grep -E '"/dev/|/sys/(class|devices/virtual)/kfd|/sys/class/drm' .st.txt | sed -E 's/^[0-9]+ +//'
  echo "== the program's own output =="; cat .prog.txt
  echo "== strace's last line =="; tail -n 1 .st.txt | sed -E 's/^[0-9]+/<pid>/'; } > strace.out
rec strace "vecadd.hip" "$HIPV; $(strace -V | head -n 1)" \
    "hipcc -std=c++17 -O2 --offload-arch=gfx90a vecadd.hip -o vecadd; strace -f -o trace.txt -e trace=openat ./vecadd" "$rc" \
    "note:      exit code 1 is the program's own CHECK failing: no GPU in the build container" \
    "hardware:  untested on hardware: the build container has no AMD GPU (AH-26)"
grep -q '/dev/kfd' strace.out || status=1

mkdir -p .st && ( cd .st && hipcc -std=c++17 -O2 --offload-arch=gfx90a --save-temps -c ../vecadd.hip -o vecadd.o ) > .st.log 2>&1; rc=$?
S=.st/vecadd-hip-amdgcn-amd-amdhsa-gfx90a.s
{ echo "== kernel code (from $(basename "$S")) =="
  sed -n '/^_Z6vecAddPKfS0_Pfi:/,/s_endpgm/p' "$S"
  echo "== kernel descriptor directives (selection) =="
  grep -E 'amdhsa_(group_segment_fixed_size|private_segment_fixed_size|kernarg_size|user_sgpr_count|user_sgpr_private_segment_buffer|user_sgpr_dispatch_ptr|user_sgpr_kernarg_segment_ptr|system_sgpr_workgroup_id_x|system_vgpr_workitem_id|next_free_vgpr|next_free_sgpr) ' "$S" | sed 's/^\s*//'
  echo "== compiler's kernel info =="; grep -E '^; (codeLenInByte|NumSgprs|NumVgprs|ScratchSize|Occupancy|LDSByteSize)' "$S"; } > isa.out
rec isa "vecadd.hip" "$HIPV" "hipcc -std=c++17 -O2 --offload-arch=gfx90a --save-temps -c vecadd.hip -o vecadd.o; excerpts of vecadd-hip-amdgcn-amd-amdhsa-gfx90a.s" "$rc" \
    "hardware:  untested on hardware: the code was generated, not run (no AMD GPU in the build container)"
[ "$rc" = 0 ] && grep -q s_endpgm isa.out || status=1

rm -rf .st .st.log .vecadd .hb.txt .prog.txt .st.txt vecadd.o
exit $status
