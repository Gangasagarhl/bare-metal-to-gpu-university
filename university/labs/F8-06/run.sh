#!/usr/bin/env bash
# F8-06 extra lab step:
#   dp_mpi - Listing 2 (Open MPI as the communication library) with 1, 2 and 4 processes.
# MPI notes as in F8-01/run.sh (--allow-run-as-root, --oversubscribe, -DOMPI_SKIP_MPICXX).
set -u
cd "$(dirname "$0")"
status=0
GXX="$(g++ --version | head -n 1)"
MPV="$(mpirun --version 2>&1 | head -n 1)"
FLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -DOMPI_SKIP_MPICXX"
RUN="mpirun --allow-run-as-root --oversubscribe"
{
    echo "listing:   dp_mpi.cc (with mlp.hpp)"
    echo "toolchain: $MPV; $GXX"
    echo "command:   mpic++ $FLAGS dp_mpi.cc -o dp_mpi; for np in 1 2 4: timeout 60 $RUN -np \$np ./dp_mpi"
    echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "machine:   $(uname -s) $(uname -m) (cloud build container, $(nproc) logical CPUs; MPI processes on one machine, no GPU)"
} > dp_mpi.log
if mpic++ $FLAGS dp_mpi.cc -o .bin_F806_dp >> dp_mpi.log 2>&1; then
    : > dp_mpi.out
    rc=0
    for np in 1 2 4; do
        timeout 60 $RUN -np "$np" ./.bin_F806_dp >> dp_mpi.out 2>&1 || rc=1
    done
    pkill -9 -f "[.]bin_F806_dp" 2>/dev/null
    echo "exit code: $rc" >> dp_mpi.log; [ "$rc" = 0 ] || status=1
else
    echo "result:    BUILD FAILED" >> dp_mpi.log; status=1
fi
rm -f .bin_F806_dp
exit $status
