#!/bin/bash
# HW203-P: (1) the practical's timing half (transpose_time.cc, -O2, a measurement); (2) the
# reference solution on a second input (a marking variant for re-sits); (3) the exam papers'
# traces run through the course's own models from the chapter lab folders (F1-33 cachesim,
# F1-35 writepolicy, F1-36 mesi, F1-38 tlb_sim and vaddr_split), so that every number in the
# answer keys comes from a real run. Logs are in the run_lab.sh format.
set -u
cd "$(dirname "$0")" || exit 2
CXX="${CXX:-g++}"
TC="$($CXX --version | head -n 1)"
MACH="$(uname -s) $(uname -m) (cloud build container, $(nproc) CPUs); CPU model: $(grep -m1 'model name' /proc/cpuinfo | cut -d: -f2 | sed 's/^ //')"
SAN="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
OPT="-std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror"
status=0
rec() {   # rec <name> <listing> <command> <exit code> [extra lines...]
    {
        echo "listing:   $2"
        echo "toolchain: $TC"
        echo "command:   $3"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $MACH"
        echo "exit code: $4"
        shift 4
        for line in "$@"; do echo "$line"; done
    } > "$1.log"
}
# (1) the measurement
$CXX $OPT transpose_time.cc -o ./.bin_tt || exit 1
timeout 300 ./.bin_tt > transpose_time.out 2>&1; rc=$?; [ "$rc" = 0 ] || status=1
rec transpose_time transpose_time.cc "$CXX $OPT transpose_time.cc -o transpose_time && ./transpose_time" $rc \
    "note:      measured on the build container (a shared cloud virtual machine), one run; not a specification (AH-23)"
rm -f ./.bin_tt
# ratios of the measured times, computed from the file just written (so a rerun keeps them consistent)
awk '/^sum by rows/ {r=$4} /^sum by columns/ {c=$4} /^transpose naive/ {t=$3} /^transpose blocked 16/ {b16=$4} /^transpose blocked 64/ {b64=$4}
     END {printf "columns / rows = %.1f x\ntranspose naive / blocked 16 = %.1f x\ntranspose naive / blocked 64 = %.1f x\ncolumns / transpose naive = %.2f x\n", c/r, t/b16, t/b64, c/t}' \
    transpose_time.out > transpose_time_ratios.out 2>&1; rc=$?; [ "$rc" = 0 ] || status=1
rec transpose_time_ratios "transpose_time.out (ratios computed by run.sh with awk)" "awk ... transpose_time.out > transpose_time_ratios.out" $rc \
    "note:      derived from the one measured run recorded in transpose_time.log"
# (2) the reference solution on the re-sit input
$CXX $SAN cache_exam.cpp -o ./.bin_ref || exit 1
timeout 60 ./.bin_ref < cache_exam_case2.in > cache_exam_case2.out 2>&1; rc=$?; [ "$rc" = 0 ] || status=1
rec cache_exam_case2 "cache_exam.cpp (run on cache_exam_case2.in)" \
    "$CXX $SAN cache_exam.cpp -o cache_exam && ./cache_exam < cache_exam_case2.in" $rc "stdin:     cache_exam_case2.in"
rm -f ./.bin_ref
# (3) exam traces through the chapter models (sources unchanged, in their own lab folders)
model() {   # model <name> <source path relative to labs/> <input>
    local name="$1" src="$2" in="$3"
    local bin="./.bin_model"
    if ! $CXX $SAN "../$src" -o "$bin" > "$name.build.txt" 2>&1; then
        rec "$name" "$src" "$CXX $SAN $src -o model" 1 "result:    BUILD FAILED"; cat "$name.build.txt" >> "$name.log"
        rm -f "$name.build.txt"; status=1; return
    fi
    rm -f "$name.build.txt"
    timeout 60 "$bin" < "$in" > "$name.out" 2>&1; rc=$?; [ "$rc" = 0 ] || status=1
    rec "$name" "$src (run on $in)" "$CXX $SAN $src -o $(basename "${src%.cpp}") && ./$(basename "${src%.cpp}") < $in" $rc "stdin:     $in" \
        "note:      the model's source is the chapter listing in university/labs/$(dirname "$src")/; only the input is the exam's"
    rm -f "$bin"
}
model cachesim_exam_dm   F1-33/cachesim.cpp    cachesim_exam_dm.in
model cachesim_exam_2way F1-33/cachesim.cpp    cachesim_exam_2way.in
model cachesim_exam_full F1-33/cachesim.cpp    cachesim_exam_full.in
model writepolicy_mid    F1-35/writepolicy.cpp writepolicy_mid.in
model writepolicy_final  F1-35/writepolicy.cpp writepolicy_final.in
model mesi_exam          F1-36/mesi.cpp        mesi_exam.in
model tlb_exam           F1-38/tlb_sim.cpp     tlb_exam.in
model vaddr_exam         F1-38/vaddr_split.cpp vaddr_exam.in
exit $status
