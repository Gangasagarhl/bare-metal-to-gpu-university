#!/usr/bin/env bash
# HW301 practical (P) extra steps. The runner (run_lab.sh) has already built and run
# kernel_map.cpp (reference) and kernel_map_start.cpp (starting file) on their .in files, and
# exam_checks.cpp. This script adds:
#   kernel_map_case2    the reference solution on a second sheet (EX-2, a wave64 exercise GPU) for re-sits
#   forensic_occupancy  F1-58's occupancy model (TG-1 limits) on the forensic kernel        } evidence of the
#   forensic_waves      F1-56's waves model: the forensic grid on GPU-A (4 SMs) and GPU-B (16)} final's forensic
#   forensic_copy       F1-62's copy model: the forensic job as profiled on both GPUs        } question
#   forensic_fix        F1-62's copy model on the three fixes proposed in the answer key
#   forensic_fix_waves  F1-56's waves model on the enlarged grids of the fix
#   project_passport    F1-63's passport checker on the project's reference passport
# Every model is the course's own listing, compiled from its chapter folder; nothing here needs a GPU.
set -u
cd "$(dirname "$0")"
status=0
CXXFLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
GXX="$(g++ --version | head -n 1)"
step() {  # step <name> <listing> <command>
    local name="$1" listing="$2" cmd="$3"
    {
        echo "listing:   $listing"
        echo "toolchain: $GXX"
        echo "command:   $cmd"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
    } > "$name.log"
    bash -c "$cmd" > "$name.out" 2>&1
    local rc=$?
    echo "exit code: $rc" >> "$name.log"
    echo "stdin:     $name.in" >> "$name.log"
    if [ "$rc" -ne 0 ]; then status=1; fi
}
step kernel_map_case2 "kernel_map.cpp (run on kernel_map_case2.in)" \
  "g++ $CXXFLAGS kernel_map.cpp -o .bin_ref && ./.bin_ref < kernel_map_case2.in"
step forensic_occupancy "../F1-58/occupancy.cpp (run on forensic_occupancy.in)" \
  "g++ $CXXFLAGS ../F1-58/occupancy.cpp -o .bin_occ && ./.bin_occ < forensic_occupancy.in"
step forensic_waves "../F1-56/waves.cpp (run on forensic_waves.in)" \
  "g++ $CXXFLAGS ../F1-56/waves.cpp -o .bin_waves && ./.bin_waves < forensic_waves.in"
step forensic_copy "../F1-62/copy_model.cpp (run on forensic_copy.in)" \
  "g++ $CXXFLAGS ../F1-62/copy_model.cpp -o .bin_copy && ./.bin_copy < forensic_copy.in"
step forensic_fix "../F1-62/copy_model.cpp (run on forensic_fix.in)" \
  "./.bin_copy < forensic_fix.in"
step forensic_fix_waves "../F1-56/waves.cpp (run on forensic_fix_waves.in)" \
  "./.bin_waves < forensic_fix_waves.in"
step project_passport "../F1-63/passport_check.cpp (run on project_passport_ref.txt)" \
  "g++ $CXXFLAGS ../F1-63/passport_check.cpp -o .bin_pp && ./.bin_pp < project_passport_ref.txt"
sed -i 's/^stdin:     project_passport.in$/stdin:     project_passport_ref.txt/' project_passport.log
rm -f .bin_ref .bin_occ .bin_waves .bin_copy .bin_pp
exit $status
