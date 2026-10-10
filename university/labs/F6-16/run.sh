#!/usr/bin/env bash
# F6-16 extra lab steps (no GPU needed).
#   cub_lookback - the tile status values of CUB 2.0.1's decoupled look-back, as installed
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
step cub_lookback "/usr/include/cub/agent/single_pass_scan_operators.cuh" "CUB 2.0.1 (libcub-dev 2.0.1-2), header as installed" "read only: no GPU needed" \
  "grep -n -A6 '^enum ScanTileStatus' /usr/include/cub/agent/single_pass_scan_operators.cuh; grep -n 'WARP_ANY((tile_descriptor.status == SCAN_TILE_INVALID)' /usr/include/cub/agent/single_pass_scan_operators.cuh | head -n 1"
exit $status
