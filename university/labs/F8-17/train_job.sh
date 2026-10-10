#!/usr/bin/env bash
#SBATCH --job-name=f8-train
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=1
#SBATCH --gpus-per-node=4
#SBATCH --time=00:30:00
#SBATCH --signal=B:TERM@120
#SBATCH --requeue
#SBATCH --open-mode=append
#SBATCH --output=f8-train-%j.out
# F8-17 Listing 3: a batch script for a long training job that can be stopped and requeued.
# The #SBATCH lines are comments to bash; their option names are UNVERIFIED (see the chapter).
# Under bash alone (no scheduler) the script runs as a dry run, which is how this build ran it.
set -uo pipefail

NODE="${NODE:-$(hostname -s)}"
CKPT_DIR="${CKPT_DIR:-${TMPDIR:-/tmp}/f8-ckpt}"      # where the checkpoint lives (see the forensic lab)
WORKER="${WORKER:-./worker}"
echo "train_job.sh: job ${SLURM_JOB_ID:-(none: dry run)} on node $NODE, restart count ${SLURM_RESTART_COUNT:-0}"
echo "train_job.sh: checkpoint directory $CKPT_DIR"
mkdir -p "$CKPT_DIR" || exit 3

"$WORKER" steps="${STEPS:-40}" every=10 step_ms="${STEP_MS:-50}" ckpt="$CKPT_DIR/state.txt" &
child=$!
# The scheduler's warning signal reaches this script; pass it on to the worker.
trap 'echo "train_job.sh: SIGTERM received, forwarding it to the worker (pid $child)"; kill -TERM "$child"' TERM
wait "$child"
rc=$?
if [ "$rc" -gt 128 ]; then           # wait was interrupted by the trap: wait for the real exit code
    wait "$child"
    rc=$?
fi
case "$rc" in
    0)  echo "train_job.sh: training finished" ;;
    99) echo "train_job.sh: worker checkpointed and stopped early; asking to be requeued"
        if command -v scontrol > /dev/null 2>&1 && [ -n "${SLURM_JOB_ID:-}" ]; then
            scontrol requeue "$SLURM_JOB_ID"
        else
            echo "train_job.sh: (no scheduler here: dry run, nothing requeued)"
        fi ;;
    *)  echo "train_job.sh: worker failed with exit code $rc" ;;
esac
exit "$rc"
