#!/usr/bin/env bash
# F6-18 extra lab steps (no GPU needed).
#   cub_radix   - what the installed CUB 2.0.1 header says about DeviceRadixSort: order, stability, key transformations
#   cub_policy  - CUB 2.0.1's tuning policy for sm_80 (radix bits, onesweep flag), read only
#   cub_kernels - the kernels that radix.cu really contains for sm_80 (ours and CUB's), demangled
#   sass_count  - __syncthreads_count in countZeros as SASS (sm_80)
#   cub_onesweep_atoms - shared-memory atomic instructions in CUB's onesweep kernel (sm_80), counted by kind
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
step cub_radix "/usr/include/cub/device/device_radix_sort.cuh" "CUB 2.0.1 (libcub-dev 2.0.1-2), header as installed" "read only: no GPU needed" \
  "grep -n -E 'least-significant to most-significant|For unsigned integral values|For signed integral values|For positive floating point|For negative floating point|DeviceRadixSort is stable' /usr/include/cub/device/device_radix_sort.cuh | head -n 6 | sed -E 's/[[:space:]]+\*[[:space:]]?/ /'"
NVCC="$(nvcc --version | tail -n 2 | head -n 1)"
COMPILED="compiled only: no GPU is needed to read generated code"
step cub_policy "/usr/include/cub/device/dispatch/dispatch_radix_sort.cuh" "CUB 2.0.1 (libcub-dev 2.0.1-2), header as installed" "read only: no GPU needed" \
  "grep -n -A 8 'struct Policy800' /usr/include/cub/device/dispatch/dispatch_radix_sort.cuh | grep -E 'Policy800|RADIX_BITS|ONESWEEP ' | sed -E 's/[[:space:]]+/ /g'"
step cub_kernels "radix.cu" "$NVCC" "$COMPILED" \
  "nvcc -arch=sm_80 -cubin radix.cu -o .tmp.cubin && cuobjdump -sass .tmp.cubin | grep 'Function :' | sed 's/.*Function : //' | cu++filt | sed -E 's/<.*//; s/^void //' | sort -u"
step sass_count "radix.cu" "$NVCC" "$COMPILED" \
  "nvcc -arch=sm_80 -cubin radix.cu -o .tmp.cubin && cuobjdump -sass -fun _Z10countZerosPKjiiPi .tmp.cubin | grep -E '^[[:space:]]+/\*[0-9a-f]{4}\*/' | sed -E 's#^[[:space:]]+/\*([0-9a-f]+)\*/[[:space:]]+(.*[^[:space:]])[[:space:]]*;.*#\1  \2#' | grep -E 'LDG|BAR|STG'"
step cub_onesweep_atoms "radix.cu" "$NVCC" "$COMPILED" \
  "nvcc -arch=sm_80 -cubin radix.cu -o .tmp.cubin && cuobjdump -sass .tmp.cubin | awk '/Function :/{p=(\$0 ~ /Onesweep/)} p' | grep -o -E 'ATOMS[.A-Z0-9]*|ATOMG[.A-Z0-9]*|RED[.A-Z0-9]*' | sort | uniq -c"
rm -f .tmp.cubin
exit $status
