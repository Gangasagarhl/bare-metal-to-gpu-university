#!/usr/bin/env bash
# F7-18 extra lab step (compile only: no GPU is needed to read generated code).
#   instances_isa - register, LDS and workgroup metadata of the three compiled instances (gfx90a)
set -u
cd "$(dirname "$0")"
CLANG="$(clang++-17 --version | head -n 1) (the compiler that hipcc drives; HIP 5.7)"
cmd="hipcc -std=c++17 -O2 --offload-arch=gfx90a --cuda-device-only -S mini_ck.hip -o .tmp.s && grep -E '^[[:space:]-]+\\.(name|group_segment_fixed_size|vgpr_count|sgpr_count|vgpr_spill_count|max_flat_workgroup_size):' .tmp.s | sed -E 's/^[[:space:]-]+//'"
{
    echo "listing:   mini_ck.hip, mini_ck.hpp"
    echo "toolchain: $CLANG"
    echo "command:   $cmd"
    echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
    echo "hardware:  compiled only: no GPU is needed to read generated code"
} > instances_isa.log
bash -c "$cmd" > instances_isa.out 2>&1
rc=$?
sed -i -E '/argument unused during compilation/d' instances_isa.out
echo "exit code: $rc" >> instances_isa.log
rm -f .tmp.s
exit $rc
