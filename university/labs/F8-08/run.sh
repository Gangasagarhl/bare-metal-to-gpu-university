#!/usr/bin/env bash
# F8-08 lab steps:
#   aware_probe      - Listing 1 on 2 processes: does this Open MPI accept device pointers?
#   ompi_build       - ompi_info and the UCX module folder: how this Open MPI was built
#   force_cuda       - Listing 1 again, with CUDA support requested by an MCA parameter
#                      (--mca mpi_cuda_support 1) on a library built without it: the real
#                      failure is recorded (expected non-zero exit code)
#   pingpong_staging - Listing 2 on 2 processes: direct versus staged ping-pong (CPU model)
#   halo_device      - Listing 3: CUDA + MPI, built for real with nvcc; run on 2 processes;
#                      untested on hardware (the container has no GPU)
# MPI notes as in F8-07/run.sh (--allow-run-as-root, --oversubscribe, -DOMPI_SKIP_MPICXX).
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

# --- Listing 1 and the forced-CUDA run
header aware_probe aware_probe.cc "$MPV; $GXX" "mpic++ $FLAGS aware_probe.cc -o aware_probe; $RUN -np 2 ./aware_probe"
if mpic++ $FLAGS aware_probe.cc -o .bin_F808_probe > .build.txt 2>&1; then
    timeout -k 5 60 $RUN -np 2 ./.bin_F808_probe > aware_probe.out 2>&1
    rc=$?; echo "exit code: $rc" >> aware_probe.log; [ "$rc" = 0 ] || status=1
    header force_cuda aware_probe.cc "$MPV; $GXX" "$RUN --mca mpi_cuda_support 1 -np 2 ./aware_probe"
    timeout -k 5 60 $RUN --mca mpi_cuda_support 1 -np 2 ./.bin_F808_probe > force_cuda.out 2>&1
    echo "exit code: $? (non-zero expected: CUDA support requested from a library built without it)" >> force_cuda.log
else
    echo "result:    BUILD FAILED" >> aware_probe.log; cat .build.txt >> aware_probe.log; status=1
fi
rm -f .build.txt

# --- how this Open MPI was built
header ompi_build "(tool run, no listing)" "$MPV" "ompi_info --parsable --all | grep cuda_support:value; ompi_info | grep 'Configure command line' (options starting --with or --enable); ls /usr/lib/x86_64-linux-gnu/ucx/"
{
    echo "## ompi_info --parsable --all | grep cuda_support:value"
    ompi_info --parsable --all | grep "cuda_support:value"
    echo "## ompi_info --parsable --all | grep 'opal_built_with_cuda_support:help'"
    ompi_info --parsable --all | grep "opal_built_with_cuda_support:help"
    echo "## configure options of this build that start with --with or --enable"
    ompi_info | grep "Configure command line" | head -n 1 | tr ' ' '\n' | grep -o -- "--\(with\|enable\)[a-z0-9-]*" | sort -u
    echo "## UCX transport modules installed (ls /usr/lib/x86_64-linux-gnu/ucx/, *.so.0 only)"
    ls /usr/lib/x86_64-linux-gnu/ucx/ | grep '\.so\.0$'
} > ompi_build.out 2>&1
echo "exit code: 0" >> ompi_build.log

# --- Listing 2: measurement build (-O2, no sanitizers: sanitizers change timing)
header pingpong_staging pingpong_staging.cc "$MPV; $GXX" "mpic++ $FLAGS pingpong_staging.cc -o pingpong_staging; $RUN -np 2 ./pingpong_staging"
if mpic++ $FLAGS pingpong_staging.cc -o .bin_F808_pp > .build.txt 2>&1; then
    timeout -k 5 120 $RUN -np 2 ./.bin_F808_pp > pingpong_staging.out 2>&1
    rc=$?; echo "exit code: $rc" >> pingpong_staging.log; [ "$rc" = 0 ] || status=1
else
    echo "result:    BUILD FAILED" >> pingpong_staging.log; cat .build.txt >> pingpong_staging.log; status=1
fi
rm -f .build.txt

# --- Listing 3: CUDA + MPI
MPIC="$(mpicxx --showme:compile)"
NVFLAGS="-x cu -std=c++17 -O2 -lineinfo -Werror all-warnings -DOMPI_SKIP_MPICXX"
header halo_device halo_device.cc "$NVV; $MPV" "nvcc $NVFLAGS \$(mpicxx --showme:compile) halo_device.cc -o halo_device -L/usr/lib/x86_64-linux-gnu/openmpi/lib -lmpi; $RUN -np 2 ./halo_device"
echo "hardware:  untested on hardware: the build container has no NVIDIA GPU (AH-26); the build is real, the run shows the runtime's own error" >> halo_device.log
if nvcc $NVFLAGS $MPIC halo_device.cc -o .bin_F808_halo -L/usr/lib/x86_64-linux-gnu/openmpi/lib -lmpi > .build.txt 2>&1; then
    timeout -k 5 60 $RUN -np 2 ./.bin_F808_halo > halo_device.out 2>&1
    echo "exit code: $?" >> halo_device.log
else
    echo "result:    BUILD FAILED" >> halo_device.log; cat .build.txt >> halo_device.log; status=1
fi
rm -f .build.txt .bin_F808_*
exit $status
