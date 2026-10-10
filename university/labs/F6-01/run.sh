#!/usr/bin/env bash
# F6-01 extra lab steps (run by run_lab.sh after the listings):
#   ptx       - the PTX that nvcc generates for vecAdd (compiled only, no GPU needed)
#   cuobjdump_help - cuobjdump's own description of the options used in CU201
#   nvcc_help - nvcc's own description of the options used in CU201's compile-only steps
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
step ptx "vec_add.cu" "$NVCC" "$COMPILED" \
  "nvcc -arch=sm_80 -ptx vec_add.cu -o .tmp.ptx && awk '/.entry _Z6vecAdd/,/^}/' .tmp.ptx"
step nvcc_help "(tool) nvcc --help" "$NVCC" "-" \
  "nvcc --help | grep -E -A3 '^--(gpu-architecture|ptx|cubin|ptxas-options|generate-code|list-gpu-code) ' && nvcc --help | grep -E -A4 '^--fmad '"
step cuobjdump_help "(tool) cuobjdump --help" "$(cuobjdump --version | tail -n 2 | head -n 1) (cuobjdump)" "-" \
  "cuobjdump --help | grep -E -A2 '^--(dump-sass|function|list-elf|list-ptx) '"
rm -f .tmp.ptx
exit $status
