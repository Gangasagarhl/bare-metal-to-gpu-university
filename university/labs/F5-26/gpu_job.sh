#!/bin/bash
#SBATCH --job-name=f8-gpu-check
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=1
#SBATCH --gpus-per-node=2
#SBATCH --cpus-per-task=8
#SBATCH --mem=32G
#SBATCH --time=00:05:00
#SBATCH --output=f8-gpu-check-%j.out
# Seed of the DS303 course project: the first step of every F8 GPU job.
# The #SBATCH lines are comments to bash; Slurm's sbatch reads them. Every option name
# is in F5-26's unverified box and must be checked in the Slurm documentation.
# Outside Slurm, set GPU_PROGRAM to the program to run (default ./gpu_visible).

set -uo pipefail
echo "job id:               ${SLURM_JOB_ID:-not set (not inside a Slurm job)}"
echo "node list:            ${SLURM_JOB_NODELIST:-not set}"
echo "CUDA_VISIBLE_DEVICES: ${CUDA_VISIBLE_DEVICES:-not set}"
echo "running on:           $(hostname)"
echo "CPUs visible:         $(nproc)"
if command -v nvidia-smi > /dev/null 2>&1; then
    echo "nvidia-smi:           found"
else
    echo "nvidia-smi:           not found on this machine"
fi
prog="${GPU_PROGRAM:-./gpu_visible}"
echo "--- $prog"
"$prog"
rc=$?
echo "--- exit code of the GPU check: $rc"
if [ "$rc" -ne 0 ]; then
    echo "GPU check failed: stop here instead of starting the F8 workload"
fi
exit "$rc"
