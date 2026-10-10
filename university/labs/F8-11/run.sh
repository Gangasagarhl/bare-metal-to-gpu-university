#!/usr/bin/env bash
# F8-11 lab steps:
#   overlap_mpi     - Listing 1 on 2 processes: serial, overlap and notest schedules with a
#                     per-bucket start/complete trace (measurement build: -O2, no sanitizers)
#   overlap_streams - Listing 2: CUDA + MPI, built for real with nvcc; run on 2 processes;
#                     untested on hardware (the container has no GPU)
# Listing 3 (timeline_model.cpp) is built and run by run_lab.sh itself.
# MPI notes as in F8-07/run.sh.
set -u
cd "$(dirname "$0")"
status=0
GXX="$(g++ --version | head -n 1)"
MPV="$(mpirun --version 2>&1 | head -n 1)"
NVV="$(nvcc --version | tail -n 2 | head -n 1)"
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

header overlap_mpi overlap_mpi.cc "$MPV; $GXX" "mpic++ $FLAGS overlap_mpi.cc -o overlap_mpi; $RUN -np 2 ./overlap_mpi"
if mpic++ $FLAGS overlap_mpi.cc -o .bin_F811_ov > .build.txt 2>&1; then
    timeout -k 5 120 $RUN -np 2 ./.bin_F811_ov > overlap_mpi.out 2>&1
    rc=$?; echo "exit code: $rc" >> overlap_mpi.log; [ "$rc" = 0 ] || status=1
else
    echo "result:    BUILD FAILED" >> overlap_mpi.log; cat .build.txt >> overlap_mpi.log; status=1
fi
rm -f .build.txt

MPIC="$(mpicxx --showme:compile)"
NVFLAGS="-x cu -std=c++17 -O2 -lineinfo -Werror all-warnings -DOMPI_SKIP_MPICXX"
header overlap_streams overlap_streams.cc "$NVV; $MPV" "nvcc $NVFLAGS \$(mpicxx --showme:compile) overlap_streams.cc -o overlap_streams -L/usr/lib/x86_64-linux-gnu/openmpi/lib -lmpi; $RUN -np 2 ./overlap_streams"
echo "hardware:  untested on hardware: the build container has no NVIDIA GPU (AH-26); the build is real, the run shows the runtime's own error" >> overlap_streams.log
if nvcc $NVFLAGS $MPIC overlap_streams.cc -o .bin_F811_os -L/usr/lib/x86_64-linux-gnu/openmpi/lib -lmpi > .build.txt 2>&1; then
    timeout -k 5 60 $RUN -np 2 ./.bin_F811_os > overlap_streams.out 2>&1
    echo "exit code: $?" >> overlap_streams.log
else
    echo "result:    BUILD FAILED" >> overlap_streams.log; cat .build.txt >> overlap_streams.log; status=1
fi
rm -f .build.txt .bin_F811_*
exit $status
