#!/usr/bin/env bash
# Extra runs for F5-26. (1) gpu_visible.cu again, with CUDA_VISIBLE_DEVICES set the way a
# scheduler would set it for a job that was given GPUs 1 and 3. There is no GPU in the
# build container, so the runtime's own error is the output (untested on hardware).
# (2) gpupack.cpp on idle.in: the forensic lab's evidence.
# (3) gpu_job.sh run by plain bash (no Slurm), with the program from (1): the job script
#     stops with the GPU check's exit code (untested under Slurm and on hardware).
set -u
NV="$(nvcc --version | tail -n 2 | head -n 1)"
{
    echo "listing:   gpu_visible.cu"
    echo "toolchain: $NV"
    echo "command:   nvcc -std=c++17 -O2 -lineinfo -Werror all-warnings gpu_visible.cu -o gpu_visible; CUDA_VISIBLE_DEVICES=1,3 ./gpu_visible"
    echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
    echo "hardware:  untested on hardware: the build container has no NVIDIA GPU (AH-26); the build is real, the run shows the runtime's own error"
} > gpu_visible_job.log
if ! nvcc -std=c++17 -O2 -lineinfo -Werror all-warnings gpu_visible.cu -o .bin_gv > .build.txt 2>&1; then
    echo "result:    BUILD FAILED" >> gpu_visible_job.log; cat .build.txt >> gpu_visible_job.log; rm -f .build.txt; exit 1
fi
rm -f .build.txt
CUDA_VISIBLE_DEVICES=1,3 timeout 20 ./.bin_gv > gpu_visible_job.out 2>&1; rc=$?
echo "exit code: $rc" >> gpu_visible_job.log
{
    echo "listing:   gpu_job.sh"
    echo "toolchain: $(bash --version | head -n 1)"
    echo "command:   GPU_PROGRAM=./.bin_gv bash gpu_job.sh   (no Slurm installed in the build container)"
    echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
    echo "hardware:  untested under Slurm and untested on hardware: no Slurm and no NVIDIA GPU in the build container (AH-26)"
} > gpu_job_bash.log
GPU_PROGRAM=./.bin_gv timeout 20 bash gpu_job.sh 2>&1 | sed 's/^running on: .*/running on:           (build container host name)/' > gpu_job_bash.out
rc=${PIPESTATUS[0]}
echo "exit code: $rc" >> gpu_job_bash.log
rm -f .bin_gv
# forensic evidence: gpupack.cpp with the course flags on idle.in (spread policy only)
GXX="$(g++ --version | head -n 1)"
CXXFLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
{
    echo "listing:   gpupack.cpp"
    echo "toolchain: $GXX"
    echo "command:   g++ $CXXFLAGS gpupack.cpp -o gpupack; ./gpupack < idle.in"
    echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
} > idle.log
status=0
if g++ $CXXFLAGS gpupack.cpp -o .bin_gp > .build.txt 2>&1; then
    timeout 10 ./.bin_gp < idle.in > idle.out 2>&1; rc=$?
    echo "exit code: $rc" >> idle.log; echo "stdin:     idle.in" >> idle.log; [ "$rc" = 0 ] || status=1
else
    echo "result:    BUILD FAILED" >> idle.log; cat .build.txt >> idle.log; status=1
fi
rm -f .build.txt .bin_gp
exit $status
