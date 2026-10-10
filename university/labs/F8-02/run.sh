#!/usr/bin/env bash
# F8-02 extra lab steps:
#   ompi_algos - Open MPI's own description of its all-reduce algorithm choices (ompi_info)
#   forced     - Listing 2 run once per forced all-reduce algorithm, with 4 and with 3 processes
# MPI notes as in F8-01/run.sh: --allow-run-as-root (the container runs as root),
# --oversubscribe, and -DOMPI_SKIP_MPICXX (Open MPI's old C++ binding header fails -Werror).
set -u
cd "$(dirname "$0")"
status=0
GXX="$(g++ --version | head -n 1)"
MPV="$(mpirun --version 2>&1 | head -n 1)"
MACHINE="$(uname -s) $(uname -m) (cloud build container, $(nproc) logical CPUs shared with other jobs; MPI processes on one machine, no GPU)"
RUN="mpirun --allow-run-as-root --oversubscribe"
FLAGS="-std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror -DOMPI_SKIP_MPICXX"

header() {  # header <name> <listing> <toolchain> <command>
    {
        echo "listing:   $2"
        echo "toolchain: $3"
        echo "command:   $4"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $MACHINE"
    } > "$1.log"
}

# --- ompi_algos
OI="$(ompi_info --version 2>&1 | head -n 1)"
header ompi_algos "(tool) ompi_info" "$OI" \
  "ompi_info --param coll tuned --level 9 | grep -A2 -E 'coll_tuned_(use_dynamic_rules|allreduce_algorithm)\"'"
ompi_info --param coll tuned --level 9 2>&1 \
  | grep -A2 -E 'coll_tuned_(use_dynamic_rules|allreduce_algorithm)"' \
  | sed -E 's/^[[:space:]]+//' > ompi_algos.out
rc=${PIPESTATUS[0]}
echo "exit code: $rc" >> ompi_algos.log
[ "$rc" = 0 ] || status=1

# --- forced
header forced "allreduce_check.cc" "$MPV; $GXX" \
  "mpic++ $FLAGS allreduce_check.cc -o allreduce_check; for np in 4 3; for k in 1..6: timeout 60 $RUN -np \$np --mca coll_tuned_use_dynamic_rules 1 --mca coll_tuned_allreduce_algorithm \$k ./allreduce_check 262144"
if ! mpic++ $FLAGS allreduce_check.cc -o .bin_F802_check > .build_check.txt 2>&1; then
    echo "result:    BUILD FAILED" >> forced.log; cat .build_check.txt >> forced.log; status=1
else
    : > forced.out
    frc=0
    names=(ignore basic_linear nonoverlapping recursive_doubling ring segmented_ring rabenseifner)
    for np in 4 3; do
        for k in 1 2 3 4 5 6; do
            line="$(timeout 60 $RUN -np "$np" --mca coll_tuned_use_dynamic_rules 1 \
                     --mca coll_tuned_allreduce_algorithm "$k" ./.bin_F802_check 262144 2>&1)" || frc=1
            echo "algorithm $k (${names[$k]}): $line" >> forced.out
        done
    done
    pkill -9 -f "[.]bin_F802_check" 2>/dev/null
    echo "timing note: medians measured on this shared container with 1 MiB messages; they vary from run to run" >> forced.out
    echo "exit code: $frc" >> forced.log
    [ "$frc" = 0 ] || status=1
fi
rm -f .build_check.txt .bin_F802_check
exit $status
