#!/usr/bin/env bash
# F6-13 extra lab steps (no GPU needed).
#   ptx_atom   - the PTX each counting kernel and the scope kernel compile to (sm_80)
#   sass_atom  - the SASS atomic/reduction instructions of the same kernels (sm_80)
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

step ptx_atom "atomics.cu" "$NVCC" "$COMPILED" \
  "nvcc -arch=sm_80 -ptx atomics.cu -o .tmp.ptx && for k in countPlain countAtomic countWarpAgg countBlockAgg scopes; do echo \"--- \$k\"; awk \"/.entry _Z[0-9]+\$k/,/^}/\" .tmp.ptx | grep -E 'ld\.global|st\.global|atom\.|red\.|vote|popc|add\.s32' | sed -E 's/^[[:space:]]+//'; done"
step sass_atom "atomics.cu" "$NVCC" "$COMPILED" \
  "nvcc -arch=sm_80 -cubin atomics.cu -o .tmp.cubin && for k in countPlain countAtomic countWarpAgg countBlockAgg; do echo \"--- \$k\"; cuobjdump -sass .tmp.cubin | awk \"/Function : _Z[0-9]+\$k/{f=1} /Function : /{if(\\\$0 !~ \\\"\$k\\\")f=0} f\" | $SASSF | grep -E 'ATOM|RED|LDG|STG|VOTE|POPC'; done"
rm -f .tmp.ptx .tmp.cubin
exit $status
