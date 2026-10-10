#!/usr/bin/env bash
# F8-14 extra lab steps. The MPI program is a .cc file so that run_lab.sh does not build it with
# plain g++; it is built here with mpic++ (the Open MPI wrapper around g++), without sanitizers.
#   pipe_M1, pipe_M4, pipe_M8, pipe_M16 - Listing 2 with 4 stages and 1, 4, 8, 16 micro-batches
#                                         (acceptance test of milestone F7, part 2)
#   pipe_slow                            - forensic evidence: stage 2 is three times slower
# Timings are measured with sleeps that stand in for GPU work; they change a little on every run.
# --allow-run-as-root: the container runs as root; --oversubscribe: more processes than counted slots;
# -DOMPI_SKIP_MPICXX skips Open MPI's old C++ binding header.
set -u
cd "$(dirname "$0")"
status=0
GXX="$(g++ --version | head -n 1)"
MPV="$(mpirun --version 2>&1 | head -n 1)"
FLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -DOMPI_SKIP_MPICXX"
RUN="mpirun --allow-run-as-root --oversubscribe"
BIN=".bin_F814_pipe"
if ! mpic++ $FLAGS pipe_mpi.cc -o "$BIN" > .build.txt 2>&1; then
    { echo "listing:   pipe_mpi.cc"; echo "result:    BUILD FAILED"; cat .build.txt; } > pipe_M1.log
    rm -f .build.txt; exit 1
fi
rm -f .build.txt
step() {  # step <name> <np> <arguments...>
    local name="$1" np="$2"; shift 2
    {
        echo "listing:   pipe_mpi.cc"
        echo "toolchain: $MPV; $GXX"
        echo "command:   mpic++ $FLAGS pipe_mpi.cc -o pipe_mpi; $RUN -np $np ./pipe_mpi $*"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container, $(nproc) logical CPUs; MPI processes stand in for GPUs, sleeps stand in for GPU work; no GPU)"
    } > "$name.log"
    timeout 60 $RUN -np "$np" "./$BIN" "$@" > "$name.out" 2>&1
    local rc=$?
    echo "exit code: $rc" >> "$name.log"
    [ "$rc" = 0 ] || status=1
}
step pipe_M1 4 M=1
step pipe_M4 4 M=4
step pipe_M8 4 M=8
step pipe_M16 4 M=16
step pipe_slow 4 M=8 slow=2:3
rm -f "$BIN"
exit $status
