#!/usr/bin/env bash
# BR-03 extra lab steps (run by run_lab.sh after the .cpp/.cu listings):
#   hist_threads_timing  Listing 1 built with -O2 and no sanitizers, run with "time" (CPU measurements)
#   racy_plain           Listing 3 (.cc, a deliberate data race) built with -O2, five runs
#   racy_tsan            Listing 3 under ThreadSanitizer (first lines of the report and its summary)
#   hist_resources       ptxas resource report for the five kernels of Listing 2 (sm_80)
#   hist_profiler        Listing 2 run under the profiler that ships with this toolkit (no GPU here)
#   spin_sass            the machine code of countSpinLock (Listing 4) for sm_60 and sm_80
#   diverge_sass         the control-flow lines of weigh (Listing 6), sm_80
set -u
cd "$(dirname "$0")"
status=0
step() {  # step <name> <listing> <toolchain line> <hardware note or -> <command> [allow-nonzero]
    local name="$1" listing="$2" tc="$3" hw="$4" cmd="$5" allow="${6:-no}"
    {
        echo "listing:   $listing"
        echo "toolchain: $tc"
        echo "command:   $cmd"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container), $(nproc) CPUs visible"
        if [ "$hw" != "-" ]; then echo "hardware:  $hw"; fi
    } > "$name.log"
    bash -c "$cmd" > "$name.out" 2>&1
    local rc=$?
    sed -i "s#$(pwd)/##g; s# (BuildId: [0-9a-f]*)##g; s#\./\.bin_#./#g; s#\.bin_##g" "$name.out"
    echo "exit code: $rc" >> "$name.log"
    if [ "$rc" -ne 0 ] && [ "$allow" != "allow-nonzero" ]; then status=1; fi
}
GXX="$(g++ --version | head -n 1)"
NVCC="$(nvcc --version | tail -n 2 | head -n 1)"
W="-std=c++20 -Wall -Wextra -Wpedantic -Werror"
COMPILED="compiled only: no GPU is needed to read generated code"
SASSF="tr -d '{}' | grep -E '^[[:space:]]+/\*[0-9a-f]{4}\*/' | sed -E 's#^[[:space:]]+/\*([0-9a-f]+)\*/[[:space:]]+([^;]*[^;[:space:]])[[:space:]]*;?.*#\1  \2#' | grep -v -E '  NOP\$' | sed -E '/^0*([0-9a-f]+)  BRA 0x\1\$/d'"

step hist_threads_timing "hist_threads.cpp (timing build, no sanitizers)" "$GXX" "-" \
  "g++ $W -O2 hist_threads.cpp -o .bin_hist_threads && ./.bin_hist_threads time"
echo "note:      measured on the build container (a shared virtual machine); times change from run to run" >> hist_threads_timing.log

step racy_plain "hist_racy.cc (deliberate data race)" "$GXX" "-" \
  "g++ $W -O2 hist_racy.cc -o .bin_racy && for r in 1 2 3 4 5; do ./.bin_racy; done"
echo "note:      the number lost changes from run to run; this file holds one build's five runs" >> racy_plain.log

step racy_tsan "hist_racy.cc under ThreadSanitizer" "$GXX" "-" \
  "g++ $W -O1 -g -fsanitize=thread hist_racy.cc -o .bin_racy_tsan && ./.bin_racy_tsan > .tsan.txt 2>&1; rc=\$?; grep -m 1 -A 12 'WARNING: ThreadSanitizer' .tsan.txt | grep -v -E '^ +#[0-9]+ .*(libstdc|/usr/include|clone|start_thread)'; echo '...'; grep -E '^SUMMARY|^ThreadSanitizer: reported|^counted' .tsan.txt; exit \$rc" allow-nonzero
echo "note:      exit code 66 is ThreadSanitizer's own after it reported a race; the program itself returns 0" >> racy_tsan.log

step hist_resources "hist_cuda.cu" "$NVCC" "$COMPILED" \
  "nvcc -arch=sm_80 -cubin -Xptxas -v hist_cuda.cu -o .bin_hist.cubin 2>&1 | awk '/Compiling entry function/{print} /Used/{print}' | sed -E 's/ptxas info *: //; s/Compiling entry function/entry/'"

step hist_profiler "hist_cuda.cu under nvprof" "$NVCC; $(nvprof --version 2>&1 | grep -m 1 Release)" \
  "untested on hardware: no NVIDIA GPU in the build container; the output shows what the profiler itself reports" \
  "nvcc -std=c++17 -O2 -lineinfo hist_cuda.cu -o .bin_hist_prof && nvprof --print-gpu-trace ./.bin_hist_prof; echo \"(program exit status \$?)\"; true"

step spin_sass "spin_warp.cu, kernel countSpinLock" "$NVCC; $(cuobjdump --version | tail -n 2 | head -n 1) (cuobjdump)" "$COMPILED" \
  "for a in sm_60 sm_80; do nvcc -arch=\$a -cubin spin_warp.cu -o .bin_spin.cubin && echo \"--- \$a\" && cuobjdump -sass -fun _Z13countSpinLockv .bin_spin.cubin | $SASSF; done"

step diverge_sass "diverge.cu, kernel weigh (control-flow lines, the byte load and the atomic)" "$NVCC; $(cuobjdump --version | tail -n 2 | head -n 1) (cuobjdump)" "$COMPILED" \
  "nvcc -arch=sm_80 -cubin diverge.cu -o .bin_div.cubin && cuobjdump -sass .bin_div.cubin | $SASSF > .div.txt && echo \"(\$(wc -l < .div.txt) instructions in total; lines shown:)\" && grep -E 'BSSY|BSYNC|BRA|EXIT|LDG|RED|ISETP\.LT\.U32\.OR' .div.txt"

rm -f .bin_hist_threads .bin_racy .bin_racy_tsan .tsan.txt .bin_hist.cubin .bin_hist_prof .bin_spin.cubin .bin_div.cubin .div.txt
exit $status
