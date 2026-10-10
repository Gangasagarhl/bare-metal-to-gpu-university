#!/usr/bin/env bash
# F8-15 extra lab steps. The MPI program is a .cc file so that run_lab.sh does not build it with
# plain g++; it is built here with mpic++ (the Open MPI wrapper around g++), without sanitizers.
#   zero_np1, zero_np2, zero_np4 - Listing 2 on 1, 2 and 4 processes
#   zero_ckpt                    - 4 processes, save and reload the sharded state after step 25
#   zero_bug                     - forensic evidence: the same with bug=rank0ckpt
# --allow-run-as-root: the container runs as root; --oversubscribe: more processes than counted slots;
# -DOMPI_SKIP_MPICXX skips Open MPI's old C++ binding header.
set -u
cd "$(dirname "$0")"
status=0
GXX="$(g++ --version | head -n 1)"
MPV="$(mpirun --version 2>&1 | head -n 1)"
FLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -DOMPI_SKIP_MPICXX"
RUN="mpirun --allow-run-as-root --oversubscribe"
BIN=".bin_F815_zero"
if ! mpic++ $FLAGS zero_os.cc -o "$BIN" > .build.txt 2>&1; then
    { echo "listing:   zero_os.cc"; echo "result:    BUILD FAILED"; cat .build.txt; } > zero_np1.log
    rm -f .build.txt; exit 1
fi
rm -f .build.txt
step() {  # step <name> <np> <arguments...>
    local name="$1" np="$2"; shift 2
    {
        echo "listing:   zero_os.cc"
        echo "toolchain: $MPV; $GXX"
        echo "command:   mpic++ $FLAGS zero_os.cc -o zero_os; $RUN -np $np ./zero_os $*"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container, $(nproc) logical CPUs; MPI processes stand in for GPUs; no GPU)"
    } > "$name.log"
    timeout 60 $RUN -np "$np" "./$BIN" "$@" > "$name.out" 2>&1
    local rc=$?
    echo "exit code: $rc" >> "$name.log"
    [ "$rc" = 0 ] || status=1
}
step zero_np1 1
step zero_np2 2
step zero_np4 4
step zero_ckpt 4 ckpt=25
step zero_bug 4 ckpt=25 bug=rank0ckpt
rm -f "$BIN" os_rank*.bin
exit $status
