#!/usr/bin/env bash
# Extra run for F5-22: pi_mpi.cc is an MPI program, so it is built with mpic++ (the Open MPI
# wrapper around g++) and started by mpirun with 1, 2 and 4 processes on this one machine.
# The build container runs as root, so mpirun needs --allow-run-as-root (its own error
# message names that option); --oversubscribe lets it start more processes than it counts slots.
# -DOMPI_SKIP_MPICXX skips Open MPI's old C++ binding header (mpi.h line 2909 tests this macro),
# whose casts g++ 13 rejects under -Werror; the program uses only the C API.
set -u
GXX="$(g++ --version | head -n 1)"
MPV="$(mpirun --version 2>&1 | head -n 1)"
FLAGS="-std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror -DOMPI_SKIP_MPICXX"
{
    echo "listing:   pi_mpi.cc"
    echo "toolchain: $MPV; $GXX"
    echo "command:   mpic++ $FLAGS pi_mpi.cc -o pi_mpi; for n in 1 2 4: mpirun --allow-run-as-root --oversubscribe -np \$n ./pi_mpi"
    echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "machine:   $(uname -s) $(uname -m) (cloud build container, $(nproc) logical CPUs, shared with other jobs)"
} > pi_mpi.log
if ! mpic++ $FLAGS pi_mpi.cc -o .bin_pi_mpi > .build_pi.txt 2>&1; then
    echo "result:    BUILD FAILED" >> pi_mpi.log; cat .build_pi.txt >> pi_mpi.log; rm -f .build_pi.txt; exit 1
fi
rm -f .build_pi.txt
status=0
: > pi_mpi.out
for n in 1 2 4; do
    echo "--- mpirun -np $n" >> pi_mpi.out
    t0=$(date +%s.%N)
    timeout 60 mpirun --allow-run-as-root --oversubscribe -np "$n" ./.bin_pi_mpi > .pi_run.txt 2>&1 || status=1
    t1=$(date +%s.%N)
    sort .pi_run.txt >> pi_mpi.out   # ranks print in any order; sorted for reading
    python3 -c "print('wall time of the whole mpirun: %.2f s (measured on this container)' % ($t1 - $t0))" >> pi_mpi.out
done
rm -f .pi_run.txt .bin_pi_mpi
echo "exit code: $status" >> pi_mpi.log
exit $status
