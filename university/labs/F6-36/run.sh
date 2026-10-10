#!/usr/bin/env bash
# F6-36 extra lab steps (no GPU needed).
#   forensic  - the residency model (Listing 2) on the two builds of the forensic lab
#   sass_sync - what grid.sync(), block.sync() and cg::reduce become in SASS for sm_80 (counts)
#   resources - ptxas register report of both kernels for sm_80 (what the occupancy query uses)
#   header_quotes - the cooperative groups header lines this chapter quotes (source H1)
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
step forensic "residency.cpp" "$GXX" "-" \
  "g++ $FLAGS residency.cpp -o .tmp_rs && ./.tmp_rs < forensic.in"
step sass_sync "cg_sum.cu" "$NVCC" "$COMPILED" \
  "nvcc -std=c++17 -arch=sm_80 -cubin cg_sum.cu -o .tmp.cubin && for f in _Z7tileSumPKfPfx _Z7gridSumPKfPfS1_x; do echo \"--- \$f\"; cuobjdump -sass -fun \$f .tmp.cubin | grep -oE '(SHFL\.[A-Z]+|BAR\.SYNC(\.[A-Z]+)*|RED\.[A-Z0-9.]+|ATOM[A-Z]*\.[A-Z0-9.]+|ATOMG\.[A-Z0-9.]+|MEMBAR\.[A-Z.]+|CCTL\.[A-Z.]+|ERRBAR|BPT\.TRAP|NANOSLEEP|WARPSYNC|LD\.E\.STRONG\.GPU|LDG\.E\.STRONG\.GPU|ST\.E\.STRONG\.GPU) ' | sort | uniq -c | sort -rn; done"
step resources "cg_sum.cu" "$NVCC" "$COMPILED" \
  "nvcc -std=c++17 -arch=sm_80 -Xptxas -v -cubin cg_sum.cu -o .tmp.cubin 2>&1 | grep -E 'Compiling entry|Used' | sed -E 's/^ptxas info +: //'"
step header_quotes "/usr/include/cooperative_groups.h, cooperative_groups/details/info.h, details/sync.h" "$NVCC (headers of the same installation)" "read only: no GPU is needed" \
  "grep -n -A3 'Threads within this this group are guaranteed' /usr/include/cooperative_groups.h; grep -n -A6 'return (_data.grid.gridWs != NULL);' /usr/include/cooperative_groups.h; grep -n 'Size must be one of' /usr/include/cooperative_groups.h; grep -n -A1 'meta_group_rank() {' /usr/include/cooperative_groups.h; grep -n '_CG_ABORT()' /usr/include/cooperative_groups/details/info.h; grep -n -E 'nb = 0x80000000|atom.add.release.gpu|ld.acquire.gpu.u32 %0|__barrier_sync\\(0\\)' /usr/include/cooperative_groups/details/sync.h | head -n 5"
rm -f .tmp_rs .tmp.cubin
exit $status
