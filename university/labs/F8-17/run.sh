#!/usr/bin/env bash
# F8-17 extra lab steps. Built here: local_rank.cc with mpic++ (MPI programs are .cc files so that
# run_lab.sh does not build them with plain g++) and worker.cpp with g++ (run_lab.sh has already
# built and run it once with no arguments). Each step works in an empty scratch folder.
#   local_rank    - Listing 5 on 8 processes, gpus=4 per node (one machine: all 8 share one "node")
#   job_preempt   - Listing 3 under bash: SIGTERM after 0.8 s, then the "requeued" second run
#   restart_zero  - forensic evidence: the same, but the second run lands on another "node"
#                   whose node-local temporary folder is different (NODE and TMPDIR set by run.sh)
set -u
cd "$(dirname "$0")"
here="$(pwd)"
status=0
GXX="$(g++ --version | head -n 1)"
MPV="$(mpirun --version 2>&1 | head -n 1)"
BASHV="$(bash --version | head -n 1)"
FLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g"
header() {  # header <name> <listing> <toolchain> <command text>
    {
        echo "listing:   $2"
        echo "toolchain: $3"
        echo "command:   $4"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container, $(nproc) logical CPUs; no GPU; no scheduler)"
    } > "$1.log"
}

header local_rank local_rank.cc "$MPV; $GXX" \
    "mpic++ $FLAGS -DOMPI_SKIP_MPICXX local_rank.cc -o local_rank; mpirun --allow-run-as-root --oversubscribe -np 8 ./local_rank gpus=4"
if mpic++ $FLAGS -DOMPI_SKIP_MPICXX local_rank.cc -o .bin_F817_lr 2>> local_rank.log; then
    timeout 60 mpirun --allow-run-as-root --oversubscribe -np 8 ./.bin_F817_lr gpus=4 > .lr.txt 2>&1; rc=$?
    sort -t' ' -k2,2n .lr.txt > local_rank.out; rm -f .lr.txt
    echo "exit code: $rc" >> local_rank.log; [ "$rc" = 0 ] || status=1
else
    echo "result:    BUILD FAILED" >> local_rank.log; status=1
fi
rm -f .bin_F817_lr

if ! g++ $FLAGS worker.cpp -o .bin_F817_worker 2> .build.txt; then
    { echo "listing:   worker.cpp"; echo "result:    BUILD FAILED"; cat .build.txt; } > job_preempt.log
    rm -f .build.txt; exit 1
fi
rm -f .build.txt
WORKER="$here/.bin_F817_worker"

# preempt <name> <node of run 1> <node of run 2>
preempt() {
    local name="$1" n1="$2" n2="$3"
    header "$name" "train_job.sh, worker.cpp" "$BASHV; $GXX" \
        "g++ $FLAGS worker.cpp -o worker; NODE=$n1 TMPDIR=./$n1-local-tmp bash train_job.sh & (SIGTERM after 0.8 s); NODE=$n2 TMPDIR=./$n2-local-tmp bash train_job.sh"
    rm -rf .scratch_F817; mkdir .scratch_F817; cd .scratch_F817 || exit 2
    {
        echo "=== run 1 (node $n1) ==="
        NODE="$n1" TMPDIR="./$n1-local-tmp" WORKER="$WORKER" bash "$here/train_job.sh" &
        local pid=$!
        sleep 0.8
        kill -TERM "$pid"
        wait "$pid"; echo "=== run 1 exit code: $? ==="
        echo "=== run 2 (node $n2, after requeue) ==="
        NODE="$n2" TMPDIR="./$n2-local-tmp" WORKER="$WORKER" bash "$here/train_job.sh"
        echo "=== run 2 exit code: $? ==="
    } > "$here/$name.out" 2>&1
    cd "$here"; rm -rf .scratch_F817
    sed -i -E 's/\(pid [0-9]+\)/(pid <pid>)/' "$name.out"     # the process id changes every run
    echo "exit code: 0" >> "$name.log"
}
preempt job_preempt node-a node-a
preempt restart_zero node-a node-b
if ! grep -q 'resumed from' job_preempt.out; then
    echo "result:    UNEXPECTED (the second run did not resume)" >> job_preempt.log; status=1
fi
rm -f .bin_F817_worker
exit $status
