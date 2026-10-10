#!/usr/bin/env bash
# F7-12 extra lab steps (compile only: no GPU is needed to read generated code).
#   ops_isa   - gfx90a assembly of sumAndCompact (wave_ops.hip)
#   ops_count - cross-lane instructions counted for gfx90a (wave64) and gfx1100 (wave32)
#   mask32_quiet - bug/mask32.hip with -Wall -Wextra only: the truncation compiles without a word
#   mask32    - bug/mask32.hip with -Wshorten-64-to-32 -Werror (compile expected to fail)
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

step ops_isa "wave_ops.hip" "$CLANG" "$COMPILED" \
  "hipcc -std=c++17 -O2 --offload-arch=gfx90a --cuda-device-only -S wave_ops.hip -o .tmp.s 2>/dev/null && awk '/^_Z13sumAndCompact.*:/,/s_endpgm/' .tmp.s | $ISA"
step ops_count "wave_ops.hip" "$CLANG" "$COMPILED" \
  "for t in gfx90a gfx1100; do hipcc -std=c++17 -O2 --offload-arch=\$t --cuda-device-only -S wave_ops.hip -o .tmp.s 2>/dev/null; awk '/^_Z13sumAndCompact.*:/,/s_endpgm/' .tmp.s | $ISA > .tmp.k; echo \"--- \$t (.wavefront_size \$(grep -E '^[[:space:]]+\.wavefront_size:' .tmp.s | awk '{print \$2}'))\"; for op in ds_bpermute_b32 v_mbcnt_lo_u32_b32 v_mbcnt_hi_u32_b32 v_bcnt_u32_b32 s_bcnt1_i32_b32 s_bcnt1_i32_b64 s_and_saveexec_b32 s_and_saveexec_b64; do printf '%-22s %s\n' \$op \$(grep -c -w \$op .tmp.k); done; done"
step mask32_quiet "bug/mask32.hip" "$CLANG" "$COMPILED" \
  "hipcc -std=c++17 -O2 --offload-arch=gfx90a --cuda-device-only -c -Wall -Wextra bug/mask32.hip -o /dev/null 2>&1 | grep -v 'argument unused'; echo \"(compiler exit code \${PIPESTATUS[0]}; any diagnostics would appear above this line)\""
step mask32 "bug/mask32.hip" "$CLANG" "$COMPILED" \
  "hipcc -std=c++17 -O2 --offload-arch=gfx90a --cuda-device-only -c -Wshorten-64-to-32 -Werror bug/mask32.hip -o /dev/null 2>&1 | grep -v 'argument unused'; exit \${PIPESTATUS[0]}" expect-fail
rm -f .tmp.s .tmp.k
exit $status
