#!/usr/bin/env bash
# MP4 starter lab, extra steps (run by run_lab.sh after suite.cpp, forensic_stride.cpp, sgemm_mp4.hip).
#   isa_amd        - resources and instruction counts of every instance for gfx90a, gfx942, gfx1100
#   fatbin         - one AMD binary for the three targets: bundle contents, then the run
#   nv_build       - the same source through HIP's NVIDIA path for sm_80: SASS counts, then the run
#   hazard_scan    - the F7-20 warp-size scanner (the learner's own tool) over the MP4 sources
#   mp3_sync       - MP4's copy of the MP3 shape matrix still equals MP3's own
#   report_empty   - report.py on the real targets template with no benchmark data
#   report_fixture - report.py on invented fixture data (tests the generator, measures nothing)
#   report_lowered - report.py when a target was edited after R1 (must warn)
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
    sed -i -E '/argument unused during compilation/d' "$name.out"
    echo "exit code: $rc" >> "$name.log"
    if [ "$rc" -ne 0 ]; then status=1; fi
}
NVCC="$(nvcc --version | tail -n 2 | head -n 1)"
CLANG="$(clang++-17 --version | head -n 1) (the compiler that hipcc drives; HIP 5.7)"
GXX="$(g++ --version | head -n 1)"
PY="$(python3 --version)"
COMPILED="compiled only: no GPU is needed to read generated code"
HIPS="hipcc -x hip -std=c++17 -O2 --cuda-device-only -S"

step isa_amd "sgemm_mp4.hip, isa_summary.py" "$CLANG; $PY" "$COMPILED" \
  "for t in gfx90a gfx942 gfx1100; do $HIPS --offload-arch=\$t sgemm_mp4.hip -o .tmp_\$t.s && python3 -I isa_summary.py amd \$t .tmp_\$t.s || exit 1; done"
step fatbin "sgemm_mp4.hip" "$CLANG" \
  "untested on hardware: the build container has no AMD GPU (AH-26); the bundle listing is real, the run shows the runtime's own error" \
  "hipcc -std=c++17 -O2 --offload-arch=gfx90a --offload-arch=gfx942 --offload-arch=gfx1100 sgemm_mp4.hip -o .tmp_amd && objcopy --dump-section .hip_fatbin=.tmp_fb .tmp_amd && /usr/lib/llvm-17/bin/clang-offload-bundler --list --type=o --input=.tmp_fb && echo '--- run' && { ./.tmp_amd; echo \"(program exit code \$?)\"; }"
step nv_build "sgemm_mp4.hip, isa_summary.py" "$NVCC (with the HIP 5.7 headers, NVIDIA platform); $PY" \
  "untested on hardware: the build container has no NVIDIA GPU (AH-26); the build and the SASS are real, the run shows the runtime's own error" \
  "nvcc -std=c++17 -O2 -arch=sm_80 -x cu -D__HIP_PLATFORM_NVIDIA__ sgemm_mp4.hip -o .tmp_nv && cuobjdump -sass .tmp_nv > .tmp_nv.sass && python3 -I isa_summary.py nvidia sm_80 .tmp_nv.sass && echo '--- run' && { ./.tmp_nv; echo \"(program exit code \$?)\"; }"
step hazard_scan "../F7-20/hazards.cpp over the MP4 sources" "$GXX" "-" \
  "g++ -std=c++20 -O1 ../F7-20/hazards.cpp -o .tmp_hz && for f in mp4_portable.hpp gemm_tile.hpp sgemm_kernel.hpp tuning.hpp launchers.hpp sgemm_mp4.hip mp4_mutants.hpp; do echo \"--- \$f\"; ./.tmp_hz < \$f || exit 1; done"
step mp3_sync "mp3_sync.py, shapes.hpp, ../MP3/mp3_suite.cpp" "$PY" "-" \
  "python3 -I mp3_sync.py ../MP3/mp3_suite.cpp shapes.hpp"
step report_empty "report.py, targets.json" "$PY" "-" "python3 -I report.py targets.json -"
step report_fixture "report.py, targets_fixture.json, bench_fixture.csv" "$PY" "-" \
  "python3 -I report.py targets_fixture.json bench_fixture.csv"
step report_lowered "report.py, targets_lowered.json, bench_fixture.csv" "$PY" "-" \
  "python3 -I report.py targets_lowered.json bench_fixture.csv"
rm -f .tmp_*.s .tmp_amd .tmp_fb .tmp_nv .tmp_nv.sass .tmp_hz
exit $status
