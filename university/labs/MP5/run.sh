#!/usr/bin/env bash
# MP5 starter lab, extra steps (run_lab.sh has already built and run mp5_suite.cpp,
# mp5_mutants.cpp and mp5_faults.cpp with sanitizers, and mp5_kernel.cu with nvcc):
#   mpi_ring - Listing 4: the same ring over MPI on 2, 3 and 4 processes, against MPI_Allreduce
#   bench    - Listing 5 built with -O2 (no sanitizers): the CPU-thread bandwidth curve
#   fit      - Listing 6: alpha and beta fitted to bench.out, model versus measurement
#   ptx      - the PTX of reduceInto (Listing 7), produced by nvcc -ptx: real compiler output
# Timing outputs (bench, fit) change on every run: the machine is shared with other jobs.
set -u
cd "$(dirname "$0")"
status=0
GXX="$(g++ --version | head -n 1)"
MPV="$(mpirun --version 2>&1 | head -n 1)"
PYV="$(python3 --version 2>&1); numpy $(python3 -I -c 'import numpy; print(numpy.__version__)')"
NVV="$(nvcc --version | tail -n 2 | head -n 1)"
MACHINE="$(uname -s) $(uname -m) (cloud build container, $(nproc) logical CPUs shared with other jobs; no GPU, no RDMA device, one machine)"
RUN="mpirun --allow-run-as-root --oversubscribe"
FLAGS="-std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror"

header() {  # header <name> <listing> <toolchain> <command>
    {
        echo "listing:   $2"
        echo "toolchain: $3"
        echo "command:   $4"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $MACHINE"
    } > "$1.log"
}

# --- mpi_ring
header mpi_ring mp5_ring_mpi.cc "$MPV; $GXX" "mpic++ $FLAGS -DOMPI_SKIP_MPICXX mp5_ring_mpi.cc -o mp5_ring_mpi; $RUN -np 2|3|4 ./mp5_ring_mpi"
if mpic++ $FLAGS -DOMPI_SKIP_MPICXX mp5_ring_mpi.cc -o .bin_MP5_mpi > .build.txt 2>&1; then
    : > mpi_ring.out
    rc_all=0
    for np in 2 3 4; do
        timeout -k 5 300 $RUN -np "$np" ./.bin_MP5_mpi >> mpi_ring.out 2>&1 || rc_all=1
    done
    echo "exit code: $rc_all" >> mpi_ring.log; [ "$rc_all" = 0 ] || status=1
else
    echo "result:    BUILD FAILED" >> mpi_ring.log; cat .build.txt >> mpi_ring.log; status=1
fi

# --- bench and fit
header bench mp5_bench.cc "$GXX" "g++ $FLAGS -pthread mp5_bench.cc -o mp5_bench; ./mp5_bench"
if g++ $FLAGS -pthread mp5_bench.cc -o .bin_MP5_bench > .build.txt 2>&1; then
    timeout 600 ./.bin_MP5_bench > bench.out 2>&1; rc=$?
    echo "exit code: $rc" >> bench.log; [ "$rc" = 0 ] || status=1
else
    echo "result:    BUILD FAILED" >> bench.log; cat .build.txt >> bench.log; status=1
fi
header fit mp5_fit.py "$PYV" "python3 -I mp5_fit.py bench.out"
python3 -I mp5_fit.py bench.out > fit.out 2>&1; rc=$?
echo "exit code: $rc" >> fit.log; [ "$rc" = 0 ] || status=1

# --- ptx
header ptx mp5_kernel.cu "$NVV" "nvcc -std=c++17 -ptx mp5_kernel.cu -o - | grep -E target/entry/ld.global/add.f32/st.global lines"
if nvcc -std=c++17 -ptx mp5_kernel.cu -o .MP5.ptx > .build.txt 2>&1; then
    grep -E '^\.target|^\.visible \.entry|ld\.global|add\.f32|st\.global' .MP5.ptx > ptx.out
    echo "hardware:  untested on hardware: compiler output only; no GPU ran this code" >> ptx.log
    echo "exit code: 0" >> ptx.log
else
    echo "result:    BUILD FAILED" >> ptx.log; cat .build.txt >> ptx.log; status=1
fi
rm -f .build.txt .bin_MP5_* .MP5.ptx
exit $status
