#!/usr/bin/env bash
# F7-05 extra lab steps (no GPU needed):
#   isa            - AMD GPU assembly of vecAdd in vec_add.hip for gfx90a (isa_of.sh)
#   isa_b          - the same for vec_add_b.hip (forensic evidence "build B")
#   vec_add_nvidia - vec_add.hip built through HIP's NVIDIA back end, then run
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
step isa "vec_add.hip" "$HIPV (clang $(clang++-17 --version | head -n 1 | grep -oE '[0-9]+\.[0-9]+\.[0-9]+'))" "$COMPILED" \
  "./isa_of.sh vec_add.hip gfx90a" || status=1
step isa_b "vec_add_b.hip" "$HIPV" "$COMPILED" \
  "./isa_of.sh vec_add_b.hip gfx90a" || status=1
step vec_add_nvidia "vec_add.hip" "$HIPV with HIP_PLATFORM=nvidia; $NVV" "$NOGPU" \
  "HIP_PLATFORM=nvidia CUDA_PATH=/usr hipcc -std=c++17 -x cu vec_add.hip -o .tmp_va_nv && ./.tmp_va_nv"
rm -f .tmp_va_nv
exit $status
