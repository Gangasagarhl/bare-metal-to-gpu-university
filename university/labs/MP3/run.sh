#!/usr/bin/env bash
# MP3 starter lab, extra steps (run by run_lab.sh after the .cpp and .cu listings):
#   suite_square     - Listing 2 rebuilt with -DMP3_SQUARE_ONLY (square tile-multiple shapes only)
#   synthetic_check  - regenerate the SYNTHETIC raw records and confirm they are byte-identical
#   targets_example  - Listing 3 on Leila's signed R1 targets (an example, synthetic machine)
#   forensic_diff    - the target lines of the R1 file against those used by the final report
#   forensic_report  - Listing 4 on the final report's input (lowered targets, wall-clock reference)
#   env_label        - what this container can say about GPU, driver and toolkit
#   resources        - ptxas report of the kernel mp3_gemm times, for sm_80 (compiled only)
#   gemm_cublas      - Listing 5 built for sm_80 with the cuBLAS reference and run (no GPU here)
set -u
cd "$(dirname "$0")"
status=0
step() {  # step <name> <listing> <toolchain line> <hardware note or -> <command> [may-fail]
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
GXX="$(g++ --version | head -n 1)"
NVCC="$(nvcc --version | tail -n 2 | head -n 1)"
PY="$(python3 --version)"
FLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
COMPILED="compiled only: no GPU is needed to read generated code"
NOGPU="untested on hardware: the build container has no NVIDIA GPU (AH-26); the build is real, the run shows the runtime's own error"

step suite_square "mp3_suite.cpp with -DMP3_SQUARE_ONLY" "$GXX" - \
  "g++ $FLAGS -DMP3_SQUARE_ONLY mp3_suite.cpp -o .tmp_sq && timeout 300 ./.tmp_sq" may-fail
step synthetic_check "synthetic_raw.py" "$PY" - \
  "mkdir -p .tmp_syn && cp targets_example.txt .tmp_syn/ && python3 synthetic_raw.py .tmp_syn && cmp .tmp_syn/mp3_report.in mp3_report.in && cmp .tmp_syn/forensic_final.in forensic_final.in && echo 'regenerated files are byte-identical to mp3_report.in and forensic_final.in'"
step targets_example "targets_lock.cpp on targets_example.txt" "$GXX" - \
  "g++ $FLAGS targets_lock.cpp -o .tmp_tl && ./.tmp_tl < targets_example.txt"
step forensic_diff "targets_example.txt, forensic_final.in" "$(diff --version | head -n 1)" - \
  "diff <(grep '^target' targets_example.txt) <(grep '^target' forensic_final.in)" may-fail
step forensic_report "mp3_report.cpp on forensic_final.in" "$GXX" - \
  "g++ $FLAGS mp3_report.cpp -o .tmp_rep && ./.tmp_rep < forensic_final.in" may-fail
step env_label "-" "$NVCC" "untested on hardware: no NVIDIA GPU and no NVIDIA driver tools in the build container" \
  "echo \"toolkit: \$(nvcc --version | tail -n 2 | head -n 1)\"; echo \"host compiler: \$(g++ --version | head -n 1)\"; command -v nvidia-smi || echo 'nvidia-smi: not found in this container (no GPU driver tools)'" may-fail
step resources "mp3_gemm.cu (kernel sgemmRegTile<128,128,8,8,8,true>)" "$NVCC" "$COMPILED" \
  "nvcc -std=c++17 -arch=sm_80 -Xptxas -v -cubin mp3_gemm.cu -o .tmp.cubin 2>&1 | grep -A2 'sgemmRegTileILi128ELi128ELi8ELi8ELi8ELb1ELi1E' | grep -E 'Used|spill' | sed -E 's/^ptxas info +: //'"
step gemm_cublas "mp3_gemm.cu" "$NVCC" "$NOGPU" \
  "nvcc -std=c++17 -O2 -lineinfo -arch=sm_80 -DUSE_CUBLAS mp3_gemm.cu -lcublas -o .tmp_gemm && ./.tmp_gemm" may-fail
rm -rf .tmp_sq .tmp_syn .tmp_tl .tmp_rep .tmp.cubin .tmp_gemm
exit $status
