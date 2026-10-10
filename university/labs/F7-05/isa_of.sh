#!/usr/bin/env bash
# isa_of.sh <source.hip> <gfx target> - print one HIP file's AMD GPU assembly, kernel bodies
# only (comments and assembler directives removed), then the kernel's resource metadata.
set -eu
src="$1"; arch="$2"
work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT
cp "$src" "$work/k.hip"
( cd "$work" && hipcc -std=c++17 -O2 --offload-arch="$arch" --save-temps -c k.hip -o k.o 2> /dev/null )
asm="$work/k-hip-amdgcn-amd-amdhsa-$arch.s"
echo "target: $(grep -m1 -oE 'amdgcn-amd-amdhsa--[a-z0-9]+' "$asm")"
# instructions: from each kernel label to s_endpgm, without comments or directives
awk '/^_Z[A-Za-z0-9_]*:/ {on=1; sub(/[[:space:]]*;.*/, ""); print; next}
     on && /^[[:space:]]*s_endpgm/ {print; on=0; next}
     on && /^\.LBB/ {print; next}
     on && /^[[:space:]]+[a-z]/ {sub(/[[:space:]]*;.*/, ""); print}' "$asm"
echo "metadata:"
grep -E '^\s+\.(name|sgpr_count|vgpr_count|wavefront_size|kernarg_segment_size|private_segment_fixed_size):' "$asm" \
  | sed -E 's/^[[:space:]]+/  /; s/:[[:space:]]+/: /'
