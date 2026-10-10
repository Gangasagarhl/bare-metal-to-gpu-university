#!/usr/bin/env bash
# F6-32 extra lab steps (each writes <name>.out and <name>.log):
#   timeline_pinned / timeline_pageable  the F6-28 model (Listing 2 of F6-28) on the two
#                     forensic scenarios; their CSV lines are the "exported timelines"
#   stats_pinned / stats_pageable        Listing 3 run on those two exports
#   nvprof_run        nvtx_pipeline.cu run under that profiler in this build (no GPU)
#   nvprof_help       the profiler that ships with this toolkit, its own help text (excerpt)
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
GXX="$(g++ --version | head -n 1)"
NVCC="$(nvcc --version | tail -n 2 | head -n 1)"
FLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
MODEL="model only: no GPU involved"
g++ $FLAGS ../F6-28/stream_model.cpp -o .bin_model || status=1
g++ $FLAGS timeline_stats.cpp -o .bin_stats || status=1
step timeline_pinned "../F6-28/stream_model.cpp with nooverlap_pinned.in" "$GXX" "$MODEL" \
  "./.bin_model < nooverlap_pinned.in"
step timeline_pageable "../F6-28/stream_model.cpp with nooverlap_pageable.in" "$GXX" "$MODEL" \
  "./.bin_model < nooverlap_pageable.in"
step stats_pinned "timeline_stats.cpp with timeline_pinned.out" "$GXX" "$MODEL" \
  "./.bin_stats < timeline_pinned.out"
step stats_pageable "timeline_stats.cpp with timeline_pageable.out" "$GXX" "$MODEL" \
  "./.bin_stats < timeline_pageable.out"
step nvprof_help "nvprof --help (excerpt)" "$NVCC" "-" \
  "nvprof --help 2>&1 | grep -A3 -E '^ +(-o, +)?--(print-gpu-trace|export-profile)( |$)'"
step nvprof_run "nvtx_pipeline.cu under nvprof" "$NVCC" "untested on hardware: no NVIDIA GPU in the build container; the output shows what the profiler itself reports" \
  "nvcc -std=c++17 -O2 -lineinfo nvtx_pipeline.cu -o .bin_pipe && nvprof --print-gpu-trace ./.bin_pipe p; echo \"(program exit status \$?)\"; true"
rm -f .bin_model .bin_stats .bin_pipe
exit $status
