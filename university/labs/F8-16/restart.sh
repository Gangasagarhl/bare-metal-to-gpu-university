#!/usr/bin/env bash
# F8-16 Listing 2: run a job and, when it fails, start it again; the job resumes from its own
# last checkpoint. Usage: restart.sh <max attempts> <time limit s> <program> [arguments...]
# Environment: NP (processes, default 4); FAULT (optional) is added to the arguments of the
# FIRST attempt only, because the injected failure is meant to be transient.
set -u
max="$1"; limit="$2"; shift 2
np="${NP:-4}"
attempt=1
while true; do
    extra=()
    if [ "$attempt" = 1 ] && [ -n "${FAULT:-}" ]; then extra=("$FAULT"); fi
    echo "restart.sh: attempt $attempt of $max, $np processes, time limit $limit s"
    timeout -s KILL "$limit" mpirun --allow-run-as-root --oversubscribe -np "$np" "$@" "${extra[@]}"
    rc=$?
    if [ "$rc" = 0 ]; then
        echo "restart.sh: attempt $attempt succeeded"
        exit 0
    fi
    echo "restart.sh: attempt $attempt failed with exit code $rc"
    if [ "$attempt" -ge "$max" ]; then
        echo "restart.sh: giving up after $attempt attempts"
        exit "$rc"
    fi
    attempt=$((attempt + 1))
    sleep 1                                   # let the failed processes disappear
done
