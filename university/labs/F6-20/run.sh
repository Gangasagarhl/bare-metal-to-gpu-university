#!/usr/bin/env bash
# F6-20 extra lab steps (no GPU needed: they read what the compiler generates, or run models).
#   resources  - ptxas report (registers) of scaleIlp<1,2,4,8> for sm_80
#   sass_ilp   - loads, multiplies and stores of scaleIlp<1> and scaleIlp<4> in SASS for sm_80
#   build_sm80 - Listing 2 built for sm_80 and run (no GPU: the run shows the runtime's error)
#   forensic   - the scheduler model of Listing 3 on the forensic case (forensic.in)
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
    if [ "$rc" -ne 0 ] && [ "${6:-}" != "may-fail" ]; then status=1; fi
}
NVCC="$(nvcc --version | tail -n 2 | head -n 1)"
GXX="$(g++ --version | head -n 1)"
COMPILED="compiled only: no GPU is needed to read generated code"
NOGPU="untested on hardware: the build container has no NVIDIA GPU (AH-26); the build is real, the run shows the runtime's own error"
SASSF="grep -E '^[[:space:]]+/\*[0-9a-f]{4}\*/' | sed -E 's#^[[:space:]]+/\*([0-9a-f]+)\*/[[:space:]]+(.*[^[:space:]])[[:space:]]*;.*#\1  \2#'"

step resources "ilp.cu" "$NVCC" "$COMPILED" \
  "nvcc -arch=sm_80 -Xptxas -v -cubin ilp.cu -o .tmp.cubin 2>&1 | grep -E 'Compiling entry|Used' | sed -E 's/^ptxas info +: //'"
step sass_ilp "ilp.cu" "$NVCC" "$COMPILED" \
  "nvcc -arch=sm_80 -cubin ilp.cu -o .tmp.cubin && for f in _Z8scaleIlpILi1EEvPKfPffi _Z8scaleIlpILi4EEvPKfPffi; do echo \"--- \$f\"; cuobjdump -sass -fun \$f .tmp.cubin | $SASSF | grep -E 'LDG|FMUL|STG'; done"
step build_sm80 "ilp.cu" "$NVCC" "$NOGPU" \
  "nvcc -std=c++17 -O2 -lineinfo -arch=sm_80 ilp.cu -o .tmp_ilp && ./.tmp_ilp" may-fail
step forensic "latency_sim.cpp" "$GXX" "-" \
  "g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -O2 latency_sim.cpp -o .tmp_sim && ./.tmp_sim < forensic.in"
rm -f .tmp.cubin .tmp_ilp .tmp_sim
exit $status
