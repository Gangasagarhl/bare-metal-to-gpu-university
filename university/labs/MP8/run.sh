#!/usr/bin/env bash
# MP8 starter lab, extra steps (run_lab.sh itself already ran contract_tests.cpp,
# skeleton.cpp with skeleton.in, inflate.cu and inflate_hip.hip):
#   skeleton_freeze   - fault injection: perception stops; the controller must reach its safe state
#   skeleton_noage    - forensic evidence: the same fault on a build without the age check (exit 1 expected)
#   rover_plan        - the learner's F12-28 planner on this project's plan
#   rover_r0          - the learner's F12-29 gate checker on this project's R0 record
#   resources         - compiler resource reports of the perception kernel (CUDA sm_80, HIP gfx90a)
set -u
cd "$(dirname "$0")"
CXX="${CXX:-g++}"
FLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
GXXV="$($CXX --version | head -n 1)"
status=0
rec() {  # rec <name> <listing> <toolchain> <command> <exit code text> [extra line]
    {
        echo "listing:   $2"
        echo "toolchain: $3"
        echo "command:   $4"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
        echo "exit code: $5"
        if [ $# -ge 6 ]; then echo "$6"; fi
    } > "$1.log"
}
SIM="hardware:  none needed; simulated robot, simulated time (the GPU kernel is replaced by its CPU stand-in behind contract C-grid)"

# 1-2. walking skeleton under fault injection
$CXX $FLAGS skeleton.cpp -o .bin_skeleton || exit 1
for case in skeleton_freeze skeleton_noage; do
    ./.bin_skeleton < "$case.in" > "$case.out" 2>&1; rc=$?
    rec "$case" "skeleton.cpp with $case.in" "$GXXV" "$CXX $FLAGS skeleton.cpp -o skeleton && ./skeleton < $case.in" "$rc" "$SIM"
    echo "stdin:     $case.in" >> "$case.log"
done
# expected: freeze passes (0), noage fails check A1 by design (1)
[ "$(grep -c 'exit code: 0' skeleton_freeze.log)" = 1 ] || status=1
[ "$(grep -c 'exit code: 1' skeleton_noage.log)" = 1 ] || status=1
rm -f .bin_skeleton

# 3. the plan, checked by the learner's own planner from F12-28
$CXX $FLAGS ../F12-28/mp8plan.cpp -o .bin_plan || exit 1
./.bin_plan < rover_plan.in > rover_plan.out 2>&1; rc=$?
rec rover_plan "../F12-28/mp8plan.cpp with rover_plan.in" "$GXXV" \
    "$CXX $FLAGS ../F12-28/mp8plan.cpp -o mp8plan && ./mp8plan < rover_plan.in" "$rc"
[ "$rc" = 0 ] || status=1
rm -f .bin_plan

# 4. the R0 gate record, checked by the learner's own gate checker from F12-29
$CXX $FLAGS ../F12-29/gatecheck.cpp -o .bin_gate || exit 1
./.bin_gate < rover_r0.in > rover_r0.out 2>&1; rc=$?
rec rover_r0 "../F12-29/gatecheck.cpp with rover_r0.in" "$GXXV" \
    "$CXX $FLAGS ../F12-29/gatecheck.cpp -o gatecheck && ./gatecheck < rover_r0.in" "$rc"
[ "$rc" = 0 ] || status=1
rm -f .bin_gate

# 5. what the compilers report for the perception kernel (no GPU needed to read this)
NVCCV="$(nvcc --version | tail -n 2 | head -n 1)"
HIPV="$(hipcc --version 2>/dev/null | head -n 1)"
{
    echo "-- CUDA, nvcc -arch=sm_80 -Xptxas -v"
    nvcc -std=c++17 -arch=sm_80 -cubin -Xptxas -v inflate.cu -o .tmp.cubin 2>&1 | grep -E 'Compiling|Used' | sed -E 's/ptxas info *: //'
    echo "-- HIP, hipcc --offload-arch=gfx90a -Rpass-analysis=kernel-resource-usage"
    hipcc -std=c++17 -O2 --offload-arch=gfx90a -Rpass-analysis=kernel-resource-usage -c inflate_hip.hip -o .tmp.o 2>&1 \
        | grep -E 'remark:' | sed -E 's/^.*remark: *//; s/ \[-Rpass-analysis=kernel-resource-usage\]//'
} > resources.out; rc=$?
rec resources "inflate.cu, inflate_hip.hip" "$NVCCV; HIP version $HIPV" \
    "nvcc -std=c++17 -arch=sm_80 -cubin -Xptxas -v inflate.cu; hipcc -std=c++17 -O2 --offload-arch=gfx90a -Rpass-analysis=kernel-resource-usage -c inflate_hip.hip" \
    "$rc" "hardware:  compiled only: no GPU is needed to read the compilers' reports; the kernels are untested on hardware"
rm -f .tmp.cubin .tmp.o

exit $status
