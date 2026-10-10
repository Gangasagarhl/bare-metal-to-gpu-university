#!/usr/bin/env bash
# Extra runs for F5-23:
#   pending     batchsim.cpp (same course flags as run_lab.sh) on pending.in: forensic evidence
#   job_bash    job.sh run by plain bash, outside any scheduler: the #SBATCH lines are comments
set -u
GXX="$(g++ --version | head -n 1)"
CXXFLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
hdr() {   # hdr <name> <listing> <toolchain> <command>
    {
        echo "listing:   $2"
        echo "toolchain: $3"
        echo "command:   $4"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
    } > "$1.log"
}
status=0
hdr pending batchsim.cpp "$GXX" "g++ $CXXFLAGS batchsim.cpp -o batchsim; ./batchsim < pending.in"
if g++ $CXXFLAGS batchsim.cpp -o .bin_bs > .build.txt 2>&1; then
    timeout 10 ./.bin_bs < pending.in > pending.out 2>&1; rc=$?
    echo "exit code: $rc" >> pending.log; echo "stdin:     pending.in" >> pending.log
    [ "$rc" = 0 ] || status=1
else
    echo "result:    BUILD FAILED" >> pending.log; cat .build.txt >> pending.log; status=1
fi
rm -f .build.txt .bin_bs
BV="$(bash --version | head -n 1)"
hdr job_bash job.sh "$BV" "bash job.sh   (no Slurm installed in the build container)"
timeout 10 bash job.sh > job_bash.out 2>&1; rc=$?
echo "exit code: $rc" >> job_bash.log
echo "hardware:  untested under Slurm: the build container has no Slurm; only bash ran this script" >> job_bash.log
[ "$rc" = 0 ] || status=1
exit $status
