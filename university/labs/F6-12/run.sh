#!/usr/bin/env bash
# F6-12 extra lab steps (no GPU needed).
#   sass_gridsync - what cooperative_groups::this_grid().sync() compiles to (sm_80 SASS of bothPhases)
#   ptx_bar       - the PTX barrier instructions of phase1 (block-level barriers only)
set -u
cd "$(dirname "$0")"
status=0
step() {
    local name="$1" listing="$2" tc="$3" hw="$4" cmd="$5"
    {
        echo "listing:   $listing"
        echo "toolchain: $tc"
        echo "command:   $cmd"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
        echo "hardware:  $hw"
    } > "$name.log"
    bash -c "$cmd" > "$name.out" 2>&1
    local rc=$?
    echo "exit code: $rc" >> "$name.log"
    if [ "$rc" -ne 0 ]; then status=1; fi
}
NVCC="$(nvcc --version | tail -n 2 | head -n 1)"
COMPILED="compiled only: no GPU is needed to read generated code"
SASSF="grep -E '^[[:space:]]+/\*[0-9a-f]{4}\*/' | sed -E 's#^[[:space:]]+/\*([0-9a-f]+)\*/[[:space:]]+(.*[^[:space:]])[[:space:]]*;.*#\1  \2#'"

step sass_gridsync "grid_sync.cu" "$NVCC" "$COMPILED" \
  "nvcc -arch=sm_80 -cubin grid_sync.cu -o .tmp.cubin && cuobjdump -sass -fun _Z10bothPhasesPKiiPxS1_ .tmp.cubin | $SASSF | sed -n '/BPT.TRAP/,/BRA.CONV/p'"
step ptx_bar "grid_sync.cu" "$NVCC" "$COMPILED" \
  "nvcc -arch=sm_80 -ptx grid_sync.cu -o .tmp.ptx && awk '/.entry _Z6phase1/,/^}/' .tmp.ptx | grep -E 'bar\.|membar|atom' | sed -E 's/^[[:space:]]+//'"
rm -f .tmp.cubin .tmp.ptx
exit $status
