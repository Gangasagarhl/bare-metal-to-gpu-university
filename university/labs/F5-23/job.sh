#!/bin/bash
#SBATCH --job-name=pi-test
#SBATCH --nodes=1
#SBATCH --ntasks=4
#SBATCH --cpus-per-task=1
#SBATCH --mem=2G
#SBATCH --time=00:10:00
#SBATCH --output=pi-test-%j.out
# The #SBATCH lines above are read by Slurm's sbatch when the script is submitted.
# To bash they are ordinary comments. (Option names: see the unverified box in F5-23.)

set -euo pipefail
echo "job id:        ${SLURM_JOB_ID:-not set (this shell is not inside a Slurm job)}"
echo "node list:     ${SLURM_JOB_NODELIST:-not set}"
echo "tasks:         ${SLURM_NTASKS:-not set}"
echo "running on:    $(hostname)"
echo "CPUs visible:  $(nproc)"
echo "working dir:   $(basename "$PWD")"
# The real work would start here, for example: srun ./pi_mpi
echo "work step:     (placeholder) $(seq 1 4 | paste -sd+ | bc) = 1+2+3+4"
