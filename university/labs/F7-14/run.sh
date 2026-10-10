#!/usr/bin/env bash
# F7-14 extra lab steps: what this build container can and cannot profile.
#   tools_present - which ROCm profiling and management tools exist in the container
#   rocminfo      - the ROCm agent listing tool, run for real (no GPU: its own error message)
#   resources     - the static kernel facts a profiler's kernel table also shows, from the compiler
set -u
cd "$(dirname "$0")"
status=0
step() {  # step <name> <listing> <toolchain line> <hardware note or -> <command> [allow-fail]
    local name="$1" listing="$2" tc="$3" hw="$4" cmd="$5" allow="${6:-}"
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
    sed -i "s#$(pwd)/##g; s/\x1b\[[0-9;]*m//g" "$name.out"
    echo "exit code: $rc" >> "$name.log"
    if [ "$rc" -ne 0 ] && [ -z "$allow" ]; then status=1; fi
}
CLANG="$(clang++-17 --version | head -n 1) (the compiler that hipcc drives; HIP 5.7)"
NOGPU="untested on hardware: the build container has no AMD GPU and no ROCm kernel driver (AH-26); the exit code is the tool's own"

step tools_present "(none: a shell check)" "$(bash --version | head -n 1)" "-" \
  "for c in rocprofv3 rocprof rocprof-compute omniperf rocprof-sys-run omnitrace rocm-smi amd-smi rocminfo hipcc; do if command -v \$c >/dev/null; then echo \"\$c: present (\$(command -v \$c))\"; else echo \"\$c: not installed in this container\"; fi; done"
step rocminfo "(none: the tool itself)" "rocminfo $(dpkg-query -W -f='${Version}' rocminfo 2>/dev/null) (Ubuntu package, ROCm 5.7)" "$NOGPU" \
  "rocminfo" allow-fail
step resources "timed.hip" "$CLANG" "compiled only: no GPU is needed to read the compiler's report" \
  "hipcc -std=c++17 -O2 --offload-arch=gfx90a --cuda-device-only -c timed.hip -o /dev/null -Rpass-analysis=kernel-resource-usage 2>&1 | grep -E 'remark' | sed -E 's/.*remark: +//; s/ \[-R.*//'"
exit $status
