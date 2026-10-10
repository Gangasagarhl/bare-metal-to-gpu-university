#!/usr/bin/env bash
# F8-16 extra lab steps (milestone F8). The MPI program is a .cc file so that run_lab.sh does not
# build it with plain g++; it is built here with mpic++, without sanitizers. Each step runs in its
# own empty scratch folder (checkpoints and traces are files) that is deleted afterwards.
#   ft_clean   - restart.sh, no fault: the uninterrupted reference run
#   ft_hang    - rank 2 freezes (SIGSTOP) at step 37; the watchdog aborts after 2 s; restart resumes
#   ft_kill    - rank 1 is killed (SIGKILL) at step 23; Open MPI ends the job; restart resumes
#   ft_corrupt - a run stops after 45 steps, ckpt.bin is cut short, the next run falls back to ckpt.prev
#   hung       - forensic evidence: rank 3 freezes at step 41 with the watchdog OFF; the job is
#                killed by a 6 s time limit (exit code 137 expected); rank traces appended
# Open MPI prints its host name and process ids in some messages; they are replaced by <host>:<pid>.
set -u
cd "$(dirname "$0")"
here="$(pwd)"
status=0
GXX="$(g++ --version | head -n 1)"
MPV="$(mpirun --version 2>&1 | head -n 1)"
FLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -DOMPI_SKIP_MPICXX"
BIN="$here/.bin_F816_ft"
if ! mpic++ $FLAGS ft_train.cc -o "$BIN" > .build.txt 2>&1; then
    { echo "listing:   ft_train.cc"; echo "result:    BUILD FAILED"; cat .build.txt; } > ft_clean.log
    rm -f .build.txt; exit 1
fi
rm -f .build.txt
header() {  # header <name> <command text>
    {
        echo "listing:   ft_train.cc, restart.sh"
        echo "toolchain: $MPV; $GXX"
        echo "command:   mpic++ $FLAGS ft_train.cc -o ft_train; $2"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container, $(nproc) logical CPUs; MPI processes stand in for GPUs; no GPU)"
    } > "$1.log"
}
tidy() {  # remove machine-specific details from an output file
    sed -i -E -e 's/\[[A-Za-z0-9_.-]+:[0-9]+\]/[<host>:<pid>]/g' -e 's/on node [A-Za-z0-9_.-]+/on node <host>/' \
              -e 's/with PID [0-9]+/with PID <pid>/' -e 's/\[\[[0-9]+,([0-9]+)\]/[[<job id>,\1]/g' "$1"
}
scratch() { rm -rf "$here/.scratch_F816"; mkdir "$here/.scratch_F816"; cd "$here/.scratch_F816" || exit 2; }
leave() { pkill -9 -x .bin_F816_ft 2>/dev/null; cd "$here"; rm -rf "$here/.scratch_F816"; }

# reference: uninterrupted
header ft_clean "NP=4 ./restart.sh 3 60 ./ft_train watchdog=2"
scratch
NP=4 "$here/restart.sh" 3 60 "$BIN" watchdog=2 > "$here/ft_clean.out" 2>&1; rc=$?
leave
tidy ft_clean.out
echo "exit code: $rc" >> ft_clean.log; [ "$rc" = 0 ] || status=1
ref="$(grep -o 'checksum [0-9a-f]*' ft_clean.out)"

# faults followed by an automatic restart
faulty() {  # faulty <name> <fault>
    header "$1" "FAULT=fail=$2 NP=4 ./restart.sh 3 60 ./ft_train watchdog=2"
    scratch
    FAULT="fail=$2" NP=4 "$here/restart.sh" 3 60 "$BIN" watchdog=2 > "$here/$1.out" 2>&1; local rc=$?
    leave
    tidy "$1.out"
    local got; got="$(grep -o 'checksum [0-9a-f]*' "$1.out" | tail -n 1)"
    if [ -n "$ref" ] && [ "$got" = "$ref" ]; then
        echo "run.sh: final $got equals the uninterrupted run's $ref" >> "$1.out"
    else
        echo "run.sh: final '$got' DIFFERS from the uninterrupted run's '$ref'" >> "$1.out"; status=1
    fi
    echo "exit code: $rc" >> "$1.log"; [ "$rc" = 0 ] || status=1
}
faulty ft_hang 2:37:hang
faulty ft_kill 1:23:kill

# a damaged newest checkpoint
header ft_corrupt "mpirun -np 4 ./ft_train steps=45; truncate -s 100 ckpt.bin; mpirun -np 4 ./ft_train"
scratch
{
    echo "run.sh: first run, 45 steps"
    timeout -s KILL 60 mpirun --allow-run-as-root --oversubscribe -np 4 "$BIN" steps=45
    echo "run.sh: ckpt.bin cut to 100 bytes (as if the machine lost power while copying it)"
    truncate -s 100 ckpt.bin
    echo "run.sh: second run, 60 steps"
    timeout -s KILL 60 mpirun --allow-run-as-root --oversubscribe -np 4 "$BIN"
} > "$here/ft_corrupt.out" 2>&1; rc=$?
leave
tidy ft_corrupt.out
got="$(grep -o 'checksum [0-9a-f]*' ft_corrupt.out | tail -n 1)"
if [ -n "$ref" ] && [ "$got" = "$ref" ]; then
    echo "run.sh: final $got equals the uninterrupted run's $ref" >> ft_corrupt.out
else
    echo "run.sh: final '$got' DIFFERS from the uninterrupted run's '$ref'" >> ft_corrupt.out; status=1
fi
echo "exit code: $rc" >> ft_corrupt.log; [ "$rc" = 0 ] || status=1

# forensic evidence: the hung job
header hung "timeout -s KILL 6 mpirun -np 4 ./ft_train watchdog=0 warn=1 (fault injected: see the answer key)"
scratch
{ timeout -s KILL 6 mpirun --allow-run-as-root --oversubscribe -np 4 "$BIN" watchdog=0 warn=1 fail=3:41:hang \
    > "$here/hung.out" 2>&1; } 2> /dev/null; rc=$?      # (hides the shell's own "Killed" notice)
sleep 1
{
    echo "=== scheduler record: job killed by the 6 s time limit, exit code $rc ==="
    for r in 0 1 2 3; do
        echo "=== last 7 lines of rank$r.trace ==="
        tail -n 7 "rank$r.trace"
    done
} >> "$here/hung.out"
leave
tidy hung.out
echo "exit code: $rc" >> hung.log
if [ "$rc" = 137 ]; then
    echo "result:    expected: the job hangs and is killed by the time limit (forensic evidence)" >> hung.log
else
    echo "result:    UNEXPECTED (expected exit code 137)" >> hung.log; status=1
fi
rm -f "$BIN"
exit $status
