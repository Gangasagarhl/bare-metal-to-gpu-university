#!/usr/bin/env bash
# F8-13 extra lab steps. The MPI program is a .cc file so that run_lab.sh does not build it with
# plain g++; it is built here with mpic++ (the Open MPI wrapper around g++), without sanitizers.
#   tp_np1, tp_np2, tp_np4 - Listing 1 on 1, 2 and 4 processes (acceptance test of milestone F7, part 1)
#   tp_refuse              - ffn=30 on 4 processes: the divisibility check refuses (exit code 2 expected)
#   tp_ffn30_np2, tp_ffn30_np4 - forensic evidence: ffn=30 with the check switched off
# --allow-run-as-root: the container runs as root; --oversubscribe: more processes than counted slots;
# -DOMPI_SKIP_MPICXX skips Open MPI's old C++ binding header.
set -u
cd "$(dirname "$0")"
status=0
GXX="$(g++ --version | head -n 1)"
MPV="$(mpirun --version 2>&1 | head -n 1)"
FLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -DOMPI_SKIP_MPICXX"
RUN="mpirun --allow-run-as-root --oversubscribe"
BIN=".bin_F813_tp"
if ! mpic++ $FLAGS tp_mlp.cc -o "$BIN" > .build.txt 2>&1; then
    { echo "listing:   tp_mlp.cc"; echo "result:    BUILD FAILED"; cat .build.txt; } > tp_np1.log
    rm -f .build.txt; exit 1
fi
rm -f .build.txt
step() {  # step <name> <np> <expected exit code> <arguments...>
    local name="$1" np="$2" expect="$3"; shift 3
    {
        echo "listing:   tp_mlp.cc"
        echo "toolchain: $MPV; $GXX"
        echo "command:   mpic++ $FLAGS tp_mlp.cc -o tp_mlp; $RUN -np $np ./tp_mlp $*"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container, $(nproc) logical CPUs; MPI processes stand in for GPUs; no GPU)"
    } > "$name.log"
    timeout 60 $RUN -np "$np" "./$BIN" "$@" > "$name.out" 2>&1
    local rc=$?
    sed -i -E 's/\[\[[0-9]+,1\]/[[<job id>,1]/' "$name.out"   # the job id changes every run
    echo "exit code: $rc" >> "$name.log"
    if [ "$rc" != "$expect" ]; then
        echo "result:    UNEXPECTED (expected exit code $expect)" >> "$name.log"; status=1
    elif [ "$expect" != 0 ]; then
        echo "result:    expected: the program refuses a split it cannot do (exit code $expect)" >> "$name.log"
    fi
}
step tp_np1 1 0
step tp_np2 2 0
step tp_np4 4 0
step tp_refuse 4 2 ffn=30
step tp_ffn30_np2 2 0 ffn=30 check=off
step tp_ffn30_np4 4 0 ffn=30 check=off
rm -f "$BIN"
exit $status
