#!/usr/bin/env bash
# F6-03 extra lab steps (compiled only; no GPU needed):
#   ptx_spaces  - every load and store of spaces.cu in PTX, with its state space
#   resources   - ptxas report for spaces.cu: registers, shared, constant, stack
#   sass_local  - does the final machine code of spaces.cu still use local memory?
#   bins_report - ptxas report for both histogram kernels (forensic evidence)
#   bins_sass   - local-memory instructions (LDL/STL) per histogram kernel
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
COMPILED="compiled only: no GPU is needed to read generated code"
step ptx_spaces "spaces.cu" "$NVCC" "$COMPILED" \
  "nvcc -arch=sm_80 -ptx spaces.cu -o .tmp.ptx && grep -E '^[[:space:]]*\.(const|local|shared)|(ld|st)\.(global|shared|const|local|param)|bar\.sync' .tmp.ptx | sed -E 's/^[[:space:]]+//'"
step resources "spaces.cu" "$NVCC" "$COMPILED" \
  "nvcc -arch=sm_80 -Xptxas -v -cubin spaces.cu -o .tmp.cubin"
step sass_local "spaces.cu" "$NVCC" "$COMPILED" \
  "nvcc -arch=sm_80 -cubin spaces.cu -o .tmp.cubin && printf 'local-memory instructions (LDL/STL) in the SASS of spaces: ' && (cuobjdump -sass .tmp.cubin | grep -cE 'LDL|STL' || true)"
step bins_report "bins.cu" "$NVCC" "$COMPILED" \
  "nvcc -arch=sm_80 -Xptxas -v -cubin bins.cu -o .tmp.cubin"
step bins_sass "bins.cu" "$NVCC" "$COMPILED" \
  "nvcc -arch=sm_80 -cubin bins.cu -o .tmp.cubin && for f in _Z8binsSlowPKiPKfPfi _Z9binsFixedPKiPKfPfi; do printf '%s: %s LDL/STL instructions\n' \$f \$(cuobjdump -sass -fun \$f .tmp.cubin | grep -cE 'LDL|STL'); done && echo 'first local-memory lines of binsSlow:' && cuobjdump -sass -fun _Z8binsSlowPKiPKfPfi .tmp.cubin | grep -E 'LDL|STL' | head -n 4 | sed -E 's#^[[:space:]]+/\*([0-9a-f]+)\*/[[:space:]]+(.*[^[:space:]])[[:space:]]*;.*#\1  \2#'"
rm -f .tmp.ptx .tmp.cubin
exit $status
