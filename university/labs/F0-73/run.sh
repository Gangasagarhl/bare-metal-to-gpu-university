#!/usr/bin/env bash
# F0-73 extra lab step (no special hardware): the x86-64 instructions g++ emits for a float and a double add.
#   disasm         - objdump of addFloat/addDouble from addss.cpp compiled with -O2
#   nvcc_ftz_help  - what nvcc's own help text says about single-precision subnormals (no GPU needed)
set -u
cd "$(dirname "$0")"
status=0
step() {  # step <name> <listing> <toolchain line> <command>
    local name="$1" listing="$2" tc="$3" cmd="$4"
    {
        echo "listing:   $listing"
        echo "toolchain: $tc"
        echo "command:   $cmd"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
    } > "$name.log"
    bash -c "$cmd" > "$name.out" 2>&1
    local rc=$?
    echo "exit code: $rc" >> "$name.log"
    if [ "$rc" -ne 0 ]; then status=1; fi
}
GXX="$(g++ --version | head -n 1); $(objdump --version | head -n 1)"
step disasm "addss.cpp" "$GXX" \
  "g++ -std=c++20 -O2 -c addss.cpp -o .tmp.o && objdump -d --no-show-raw-insn -C .tmp.o | awk '/<addFloat|<addDouble/,/ret/' | sed -E 's/^[[:space:]]+[0-9a-f]+:[[:space:]]+/    /'"
NVCC="$(nvcc --version | tail -n 2 | head -n 1)"
step nvcc_ftz_help "-" "$NVCC" "nvcc --help | grep -A4 -E '^--ftz '"
rm -f .tmp.o
exit $status
