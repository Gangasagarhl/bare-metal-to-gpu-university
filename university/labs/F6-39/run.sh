#!/usr/bin/env bash
# F6-39 extra lab steps (no GPU needed).
#   forensic  - Listing 1 on the forensic cases (left-padded batch; guard off, then on)
#   resources - ptxas report of the attention kernel for sm_80 (registers, shared memory, spills)
#   sass_loop - the shared-memory loads, FMAs and barriers in the kernel's SASS for sm_80 (counts)
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
step forensic "attention.cpp" "$GXX" "-" \
  "g++ $FLAGS attention.cpp -o .tmp_at && ./.tmp_at < forensic.in"
step resources "attn_fwd.cu" "$NVCC" "$COMPILED" \
  "nvcc -std=c++17 -arch=sm_80 -Xptxas -v -cubin attn_fwd.cu -o .tmp.cubin 2>&1 | grep -E 'Compiling entry|Used|spill' | sed -E 's/^ptxas info +: //'"
step sass_loop "attn_fwd.cu" "$NVCC" "$COMPILED" \
  "nvcc -std=c++17 -arch=sm_80 -cubin attn_fwd.cu -o .tmp.cubin && cuobjdump -sass .tmp.cubin | grep -oE '(LDS(\.[0-9A-Z]+)*|STS(\.[0-9A-Z]+)*|LDG(\.[0-9A-Z]+)*|STG(\.[0-9A-Z]+)*|FFMA|BAR\.SYNC(\.[A-Z]+)*|MUFU\.EX2|LDL(\.[0-9A-Z]+)*|STL(\.[0-9A-Z]+)*) ' | sort | uniq -c | sort -rn"
rm -f .tmp_at .tmp.cubin
exit $status
