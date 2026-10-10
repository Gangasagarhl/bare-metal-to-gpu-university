#!/usr/bin/env bash
# F8-12 extra lab steps. MPI programs are .cc files so that run_lab.sh does not build them with
# plain g++; they are built here with mpic++ (the Open MPI wrapper around g++), without sanitizers.
#   dp_np1, dp_np2, dp_np4 - Listing 1 on 1, 2 and 4 processes (acceptance test of milestone F3)
#   slow_step              - forensic evidence: Listing 2 (dp_cost.cpp, built with the run_lab.sh flags)
#                            on slow_step.in, two traces of the communication stream
# The container runs as root, so mpirun needs --allow-run-as-root; --oversubscribe lets it start
# more processes than it counts slots. -DOMPI_SKIP_MPICXX skips Open MPI's old C++ binding header.
set -u
cd "$(dirname "$0")"
status=0
GXX="$(g++ --version | head -n 1)"
MPV="$(mpirun --version 2>&1 | head -n 1)"
FLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -DOMPI_SKIP_MPICXX"
RUN="mpirun --allow-run-as-root --oversubscribe"
BIN=".bin_F812_dp"
if ! mpic++ $FLAGS dp_train.cc -o "$BIN" > .build.txt 2>&1; then
    { echo "listing:   dp_train.cc"; echo "result:    BUILD FAILED"; cat .build.txt; } > dp_np1.log
    rm -f .build.txt; exit 1
fi
rm -f .build.txt
step() {  # step <name> <np> <argument or empty>
    local name="$1" np="$2" arg="$3"
    {
        echo "listing:   dp_train.cc"
        echo "toolchain: $MPV; $GXX"
        echo "command:   mpic++ $FLAGS dp_train.cc -o dp_train; $RUN -np $np ./dp_train $arg"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container, $(nproc) logical CPUs; MPI processes stand in for GPUs; no GPU)"
    } > "$name.log"
    timeout 60 $RUN -np "$np" "./$BIN" $arg > "$name.out" 2>&1
    local rc=$?
    echo "exit code: $rc" >> "$name.log"
    [ "$rc" = 0 ] || status=1
}
step dp_np1 1 ""
step dp_np2 2 ""
step dp_np4 4 ""
rm -f "$BIN"
CXXFLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
{
    echo "listing:   dp_cost.cpp"
    echo "toolchain: $GXX"
    echo "command:   g++ $CXXFLAGS dp_cost.cpp -o dp_cost; ./dp_cost < slow_step.in"
    echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
} > slow_step.log
if g++ $CXXFLAGS dp_cost.cpp -o .bin_F812_cost 2>> slow_step.log; then
    timeout 10 ./.bin_F812_cost < slow_step.in > slow_step.out 2>&1; rc=$?
    echo "exit code: $rc" >> slow_step.log; echo "stdin:     slow_step.in" >> slow_step.log
    [ "$rc" = 0 ] || status=1
else
    echo "result:    BUILD FAILED" >> slow_step.log; status=1
fi
rm -f .bin_F812_cost
exit $status
