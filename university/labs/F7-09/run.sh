#!/usr/bin/env bash
# F7-09 extra lab steps (compile only: no GPU is needed to read generated code).
#   w32_w64    - raw/branchy.hip for gfx1030 in wave32 (default) and wave64 (-mwavefrontsize64)
#   hip_w64    - the HIP 5.7 headers refuse wave64 on gfx1030 (compile expected to fail)
#   ballot_isa - ballot.hip for gfx90a (wave64) and gfx1100 (wave32)
set -u
cd "$(dirname "$0")"
status=0
step() {  # step <name> <listing> <toolchain line> <hardware note or -> <command> [expect-fail]
    local name="$1" listing="$2" tc="$3" hw="$4" cmd="$5" expect="${6:-}"
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
    sed -i "s#$(pwd)/##g" "$name.out"
    if [ -n "$expect" ]; then
        if [ "$rc" -ne 0 ]; then
            echo "result:    compile failed as expected (messages saved in $name.out)" >> "$name.log"
        else
            echo "result:    UNEXPECTED SUCCESS (compile was expected to fail)" >> "$name.log"; status=1
        fi
        return
    fi
    echo "exit code: $rc" >> "$name.log"
    if [ "$rc" -ne 0 ]; then status=1; fi
}
CLANG="$(clang++-17 --version | head -n 1) (the compiler that hipcc drives; HIP 5.7)"
COMPILED="compiled only: no GPU is needed to read generated code"
ISA="grep -v -E '^[[:space:]]*;' | sed -E 's/[[:space:]]*;.*$//'"
RAW="clang++-17 -x hip --cuda-device-only -nogpuinc -nogpulib -O2 -S raw/branchy.hip -o .tmp.s"

step w32_w64 "raw/branchy.hip" "$CLANG" "$COMPILED" \
  "for f in '' '-mwavefrontsize64'; do echo \"--- gfx1030 \${f:-(default)}\"; $RAW --offload-arch=gfx1030 \$f && awk '/^_Z7branchyPfi:/,/s_endpgm/' .tmp.s | $ISA && grep -E '^[[:space:]]+\.(wavefront_size|vgpr_count|sgpr_count):' .tmp.s; done"
step hip_w64 "ballot.hip" "$CLANG" "$COMPILED" \
  "hipcc -std=c++17 -O2 --offload-arch=gfx1030 -mwavefrontsize64 --cuda-device-only -S ballot.hip -o .tmp.s 2>&1 | grep -v 'argument unused'; exit \${PIPESTATUS[0]}" expect-fail
step ballot_isa "ballot.hip" "$CLANG" "$COMPILED" \
  "for t in gfx90a gfx1100; do echo \"--- \$t\"; hipcc -std=c++17 -O2 --offload-arch=\$t --cuda-device-only -S ballot.hip -o .tmp.s 2>/dev/null && awk '/^_Z13countPositive.*:/,/s_endpgm/' .tmp.s | $ISA | grep -v s_delay_alu; done"
rm -f .tmp.s
exit $status
