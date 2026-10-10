#!/usr/bin/env bash
# HW202-P: the steps the default runner does not do by itself.
#  1. the practical's reference solution on the re-sit variant (pipe_case2.in);
#  2. cross-check: the course's own pipeline model (F1-27 hazards.cpp) and the U16 computer
#     (F1-24 cpu.cpp) on the exam program; the summary lines must agree with pipe_exam.out;
#  3. the final exam's forensic evidence "The fast loop that lies": the clamp program as written,
#     Ravi's reordered version and the fixed version, each through the pipeline model (forwarding
#     diagram) and through the U16 computer (the outputs 236 / 85 / 236 are the key).
set -u -o pipefail
cd "$(dirname "$0")"
FLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
status=0
g++ $FLAGS pipe_exam.cpp -o .bin_ref || exit 1
g++ $FLAGS ../F1-27/hazards.cpp -o .bin_h || exit 1
g++ $FLAGS ../F1-24/cpu.cpp -o .bin_cpu || exit 1
step() {  # step <name> <listing text> <command text> <binary + args...> < input
    local name=$1 listing=$2 cmdtext=$3; shift 3
    "$@" > "$name.out" 2>&1; local rc=$?
    {
        echo "listing:   $listing"
        echo "toolchain: $(g++ --version | head -n 1)"
        echo "command:   $cmdtext"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
        echo "exit code: $rc"
    } > "$name.log"
    [ "$rc" = 0 ] || status=1
}
step pipe_case2 "pipe_exam.cpp (run on pipe_case2.in, the re-sit variant)" "./pipe_exam 16 < pipe_case2.in" ./.bin_ref 16 < pipe_case2.in
step case2_cpu "pipe_case2.in (run with ../F1-24/cpu.cpp)" "./cpu < pipe_case2.in" ./.bin_cpu < pipe_case2.in
step crosscheck_hazards "pipe_exam.in (run with ../F1-27/hazards.cpp, the course's model)" "./hazards 16 < pipe_exam.in" ./.bin_h 16 < pipe_exam.in
step crosscheck_cpu "pipe_exam.in (run with ../F1-24/cpu.cpp)" "./cpu < pipe_exam.in" ./.bin_cpu < pipe_exam.in
# The reference solution and the course model must report the same totals.
{
    echo "reference solution (pipe_exam.out) versus the course model (crosscheck_hazards.out), summary lines:"
    grep '^instructions' pipe_exam.out | sed 's/, load-use pairs.*//' > .ref_sum
    grep '^instructions' crosscheck_hazards.out > .model_sum
    if diff .ref_sum .model_sum > /dev/null; then
        echo "identical totals in both modes:"; cat .ref_sum
    else
        echo "DIFFERENT:"; diff .ref_sum .model_sum; status=1
    fi
    rm -f .ref_sum .model_sum
} > crosscheck_diff.out 2>&1
{
    echo "listing:   pipe_exam.out and crosscheck_hazards.out compared (GNU diffutils $(diff --version | head -n 1 | awk '{print $NF}'))"
    echo "toolchain: $(g++ --version | head -n 1)"
    echo "command:   grep '^instructions' on both outputs, then diff"
    echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
    echo "exit code: $status"
} > crosscheck_diff.log
for v in orig fast fixed; do
    step "forensic_${v}_pipe" "forensic_${v}.s (run with ../F1-27/hazards.cpp)" "./hazards 12 < forensic_${v}.s" ./.bin_h 12 < "forensic_${v}.s"
    step "forensic_${v}_cpu" "forensic_${v}.s (run with ../F1-24/cpu.cpp)" "./cpu < forensic_${v}.s" ./.bin_cpu < "forensic_${v}.s"
done
rm -f .bin_ref .bin_h .bin_cpu
exit $status
