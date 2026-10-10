#!/usr/bin/env bash
# F8-10 lab steps:
#   hier_ok   - Listing 1 (checked) on 4 ranks as 2 x 2, 8 ranks as 2 x 4 and as 4 x 2, and
#               6 ranks with ppn 4 (unequal nodes: 4 + 2, so the fallback is used)
#   hier_bug  - Listing 1 with "nocheck" on 6 ranks, ppn 4: the forensic lab's evidence
#   han_info  - ompi_info: Open MPI's own hierarchical collective component (coll han)
# Listing 2 (hier_model.cpp) is built and run by run_lab.sh itself.
# All "nodes" are simulated on one machine. MPI notes as in F8-07/run.sh.
set -u
cd "$(dirname "$0")"
status=0
GXX="$(g++ --version | head -n 1)"
MPV="$(mpirun --version 2>&1 | head -n 1)"
MACHINE="$(uname -s) $(uname -m) (cloud build container, $(nproc) logical CPUs shared with other jobs; all MPI processes on this one machine, no GPU, no RDMA device)"
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

header hier_ok hier_allreduce.cc "$MPV; $GXX" "mpic++ $FLAGS hier_allreduce.cc -o hier_allreduce; $RUN -np 4 ./hier_allreduce 2; -np 8 ... 4; -np 8 ... 2; -np 6 ... 4"
header hier_bug hier_allreduce.cc "$MPV; $GXX" "$RUN -np 6 ./hier_allreduce 4 nocheck"
if mpic++ $FLAGS hier_allreduce.cc -o .bin_F810_ha > .build.txt 2>&1; then
    : > hier_ok.out
    rc_all=0
    for cfg in "4 2" "8 4" "8 2" "6 4"; do
        set -- $cfg
        echo "--- mpirun -np $1 ./hier_allreduce $2" >> hier_ok.out
        timeout -k 5 120 $RUN -np "$1" ./.bin_F810_ha "$2" >> hier_ok.out 2>&1 || rc_all=1
    done
    echo "exit code: $rc_all" >> hier_ok.log; [ "$rc_all" = 0 ] || status=1
    grep -q "FAIL" hier_ok.out && status=1
    timeout -k 5 120 $RUN -np 6 ./.bin_F810_ha 4 nocheck > hier_bug.out 2>&1
    echo "exit code: $? (the program exits 0 and prints FAIL lines: the wrong results are the evidence)" >> hier_bug.log
else
    echo "result:    BUILD FAILED" >> hier_ok.log; cat .build.txt >> hier_ok.log; status=1
fi
rm -f .build.txt .bin_F810_*

header han_info "(tool run, no listing)" "$MPV" "ompi_info | grep 'MCA coll'; ompi_info --param coll han --level 9 (priority and reproducible lines with their help text)"
{
    echo "## ompi_info | grep 'MCA coll'"
    ompi_info | grep "MCA coll" | sed 's/^ *//'
    echo "## ompi_info --param coll han --level 9 (two parameters)"
    ompi_info --param coll han --level 9 | grep -A1 -E '"coll_han_priority"|"coll_han_reproducible"' | sed 's/^ *//'
} > han_info.out 2>&1
echo "exit code: 0" >> han_info.log
exit $status
