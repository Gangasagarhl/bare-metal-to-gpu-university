#!/usr/bin/env bash
# F7-11 extra lab steps (compile only: no GPU is needed to read generated code).
#   lds_isa   - LDS instructions of the naive (pitch 64), padded (pitch 65) and XOR-swizzled transpose, gfx90a
#   lds_occupancy - LDS size, VGPRs and the compiler's occupancy for lds_occ.hip and the transposes,
#               checked with F7-10's occupancy calculator (../F7-10/occ.cpp)
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
    sed -i "s#$(pwd)/##g" "$name.out"
    echo "exit code: $rc" >> "$name.log"
    if [ "$rc" -ne 0 ]; then status=1; fi
}
GXX="$(g++ --version | head -n 1)"
CLANG="$(clang++-17 --version | head -n 1) (the compiler that hipcc drives; HIP 5.7)"
COMPILED="compiled only: no GPU is needed to read generated code"
TABLE="awk '/^_Z/ && /:/ {split(\$1,a,\":\"); name=a[1]} /; TotalNumVgprs:/ {v=\$3} /; LDSByteSize:/ {l=\$3} /; Occupancy:/ {print name, v, l, 256, \$3}'"
DEM="sed -E 's/^_Z[0-9]+([A-Za-z]+)ILi([0-9]+)E[^ ]*/\1<\2>/; s/^_Z16transposeSwizzle[^ ]*/transposeSwizzle/'"

step lds_isa "transpose_lds.hip" "$CLANG" "$COMPILED" \
  "hipcc -std=c++17 -O2 --offload-arch=gfx90a --cuda-device-only -S transpose_lds.hip -o .tmp.s 2>/dev/null && for k in _Z13transposeTileILi64EEvPKfPfi _Z13transposeTileILi65EEvPKfPfi _Z16transposeSwizzlePKfPfi; do echo \"--- \$(echo \$k | c++filt)\"; awk -v k=\"\$k:\" '\$1==k,/s_endpgm/' .tmp.s | sed -E 's/[[:space:]]*;.*$//' > .tmp.k; grep -E 'ds_|s_barrier' .tmp.k; printf '    (whole kernel: %s instructions, %s v_xor_b32)\\n' \$(grep -c -E '^[[:space:]]+[a-z_0-9]+' .tmp.k) \$(grep -c v_xor_b32 .tmp.k); done"
step lds_occupancy "lds_occ.hip, transpose_lds.hip, ../F7-10/occ.cpp" "$GXX; $CLANG" "$COMPILED" \
  "g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined ../F7-10/occ.cpp -o .bin_occ && hipcc -std=c++17 -O2 --offload-arch=gfx90a --cuda-device-only -S lds_occ.hip -o .tmp1.s 2>/dev/null && hipcc -std=c++17 -O2 --offload-arch=gfx90a --cuda-device-only -S transpose_lds.hip -o .tmp2.s 2>/dev/null && cat .tmp1.s .tmp2.s | $TABLE | $DEM | ./.bin_occ"
rm -f .tmp.s .tmp1.s .tmp2.s .tmp.k .bin_occ
exit $status
