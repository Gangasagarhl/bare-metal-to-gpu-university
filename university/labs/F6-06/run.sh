#!/usr/bin/env bash
# F6-06 extra lab step:
#   header_events - the CUDA 12.0 header's own words on event timing and waiting
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
step header_events "(header) /usr/include/cuda_runtime_api.h" \
  "CUDA 12.0 header, Ubuntu package nvidia-cuda-dev $(dpkg-query -W -f='${Version}' nvidia-cuda-dev 2>/dev/null)" \
  "grep -n -A9 'Computes the elapsed time between two events' /usr/include/cuda_runtime_api.h | sed -E 's/^[0-9]+[-:] ?//' && echo && grep -n -A4 'Waiting for an event that was created with the ::cudaEventBlockingSync' /usr/include/cuda_runtime_api.h | sed -E 's/^[0-9]+[-:] ?//'"
exit $status
