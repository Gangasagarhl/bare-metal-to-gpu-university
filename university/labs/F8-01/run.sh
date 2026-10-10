#!/usr/bin/env bash
# F8-01 extra lab steps (MPI programs are .cc files so that run_lab.sh does not build them
# with plain g++; they are built here with mpic++, the Open MPI wrapper around g++).
#   mpi_collectives - Listing 2: six collectives through Open MPI, 4 processes
#   skip            - forensic evidence: one rank skips MPI_Allreduce (expected to hang;
#                     stopped by a 10 s time limit, exit code 124 is the expected result)
# The container runs as root, so mpirun needs --allow-run-as-root; --oversubscribe lets it
# start more processes than it counts slots. -DOMPI_SKIP_MPICXX skips Open MPI's old C++
# binding header, whose casts g++ 13 rejects under -Werror (the programs use only the C API).
set -u
cd "$(dirname "$0")"
status=0
GXX="$(g++ --version | head -n 1)"
MPV="$(mpirun --version 2>&1 | head -n 1)"
FLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -DOMPI_SKIP_MPICXX"
RUN="mpirun --allow-run-as-root --oversubscribe"

mpistep() {  # mpistep <name> <np> <expected exit code> <note>
    local name="$1" np="$2" expect="$3" note="$4"
    {
        echo "listing:   $name.cc"
        echo "toolchain: $MPV; $GXX"
        echo "command:   mpic++ $FLAGS $name.cc -o $name; timeout 10 $RUN -np $np ./$name"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container, $(nproc) logical CPUs; MPI processes on one machine, no GPU)"
    } > "$name.log"
    if ! mpic++ $FLAGS "$name.cc" -o ".bin_F801_$name" > ".build_$name.txt" 2>&1; then
        echo "result:    BUILD FAILED" >> "$name.log"; cat ".build_$name.txt" >> "$name.log"
        rm -f ".build_$name.txt"; status=1; return
    fi
    rm -f ".build_$name.txt"
    timeout 10 $RUN -np "$np" "./.bin_F801_$name" > ".run_$name.txt" 2>&1
    local rc=$?
    # mpirun does not always stop its ranks when the time limit kills it: stop leftovers
    pkill -9 -f "[.]bin_F801_$name" 2>/dev/null
    # ranks print concurrently; the lines of each rank keep their order, so sort only by rank
    # when the program does not order its own output (note says which)
    if [ "$note" = "sorted" ]; then
        sort -s -k2,2n ".run_$name.txt" | grep -E '^rank ' > "$name.out"
        grep -vE '^rank ' ".run_$name.txt" >> "$name.out"
    else
        cat ".run_$name.txt" > "$name.out"
    fi
    rm -f ".run_$name.txt" ".bin_F801_$name"
    if [ "$rc" = 124 ]; then
        echo "exit code: 124 (stopped by the 10 s time limit)" >> "$name.log"
    else
        echo "exit code: $rc" >> "$name.log"
    fi
    if [ "$rc" != "$expect" ]; then
        echo "result:    UNEXPECTED (expected exit code $expect)" >> "$name.log"; status=1
    elif [ "$expect" != 0 ]; then
        echo "result:    expected: this evidence program is broken on purpose (forensic lab)" >> "$name.log"
    fi
}

mpistep mpi_collectives 4 0 plain
mpistep skip 4 124 sorted
exit $status
