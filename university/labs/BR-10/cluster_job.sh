#!/bin/bash
#SBATCH --job-name=br10-primes
#SBATCH --nodes=3
#SBATCH --ntasks=3
#SBATCH --time=00:05:00
#SBATCH --output=br10-%j.out
# BR-10 Listing 10: the same job as a Slurm batch script. To bash, the #SBATCH lines
# are comments, so the script can be tested anywhere. Option names, srun and the
# SLURM_* variables are unverified in this build: see F5-23's unverified table.

set -euo pipefail
echo "job id:      ${SLURM_JOB_ID:-not set (not inside a Slurm job)}"
echo "node list:   ${SLURM_JOB_NODELIST:-not set}"
echo "submitted on $(hostname), $(nproc) CPUs visible, $(date -u +%Y-%m-%dT%H:%M:%SZ)"
if [ -n "${SLURM_JOB_ID:-}" ]; then
    srun ./job_mpi                                   # one rank per task Slurm allocated
else
    mpirun ${MPIRUN_EXTRA:-} -np 3 ./job_mpi | sort  # fallback: three ranks on this machine
fi
