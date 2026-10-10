#!/usr/bin/env bash
# BR-10 extra lab steps (run by run_lab.sh after the .cpp listings):
#   bench_copy      bench.hpp is byte-identical to F2-51's harness (what carries over)
#   compare_small   cluster.cpp optimised (-O2): laptop versus three nodes, small job
#   compare_large   the same, a job ten times wider
#   numa            what this machine reports about sockets and NUMA nodes
#   job_mpi         job_mpi.cc as three MPI ranks started by mpirun
#   job_mpi_stall   the same with one rank stalled: no timeout, so the time limit stops it
#   slurm_script    cluster_job.sh run by plain bash (no Slurm in the build container)
set -u
cd "$(dirname "$0")"
status=0
step() {  # step <name> <listing> <toolchain> <note or -> <command> [allow-nonzero]
    local name="$1" listing="$2" tc="$3" note="$4" cmd="$5" allow="${6:-no}"
    {
        echo "listing:   $listing"
        echo "toolchain: $tc"
        echo "command:   $cmd"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container), $(nproc) CPUs visible"
        if [ "$note" != "-" ]; then echo "note:      $note"; fi
    } > "$name.log"
    bash -c "$cmd" > "$name.out" 2>&1
    local rc=$?
    sed -i "s#$(pwd)/##g; s#\./\.bin_#./#g; s#\.bin_##g" "$name.out"
    if [ "$rc" = 124 ]; then
        echo "exit code: 124 (stopped by the time limit of the command)" >> "$name.log"
    else
        echo "exit code: $rc" >> "$name.log"
    fi
    if [ "$rc" -ne 0 ] && [ "$allow" != "allow-nonzero" ]; then status=1; fi
}
GXX="$(g++ --version | head -n 1)"
MPI="$(mpirun --version 2>&1 | head -n 1) with $GXX"
W="-std=c++20 -Wall -Wextra -Wpedantic -Werror"
MW="$W -O2 -DOMPI_SKIP_MPICXX"
ROOTOK="--allow-run-as-root --oversubscribe"
SHARED="measured on the build container (a shared virtual machine); times change from run to run"

step bench_copy "bench.hpp" "$(cmp --version | head -n 1)" "-" \
  "cmp bench.hpp ../F2-51/bench.hpp && echo 'bench.hpp is byte-identical to F2-51/bench.hpp'"

step compare_small "cluster.cpp (optimised build, no sanitizers)" "$GXX" "$SHARED" \
  "g++ $W -O2 cluster.cpp -o .bin_cluster && ./.bin_cluster 300000 21 | sed -n '5,14p'"

step compare_large "cluster.cpp (optimised build, no sanitizers)" "$GXX" "$SHARED" \
  "g++ $W -O2 cluster.cpp -o .bin_cluster && ./.bin_cluster 3000000 11 | sed -n '5,14p'"

step numa "-" "$(lscpu --version)" "the container's view; a real two-socket server reports two NUMA nodes" \
  "lscpu | grep -E '^(CPU\(s\)|Thread|Core|Socket|NUMA)'; ls -d /sys/devices/system/node/node*"

step job_mpi "job_mpi.cc" "$MPI" "three ranks on one machine; lines sorted because ranks print independently" \
  "mpic++ $MW job_mpi.cc -o job_mpi && mpirun $ROOTOK -np 3 ./job_mpi | sort"

step job_mpi_stall "job_mpi.cc (argument stall)" "$MPI" \
  "rank 1 stalls on purpose; nothing in the program has a deadline, so timeout 8 stops mpirun (exit 124 expected)" \
  "mpic++ $MW job_mpi.cc -o job_mpi && timeout 8 mpirun $ROOTOK -np 3 ./job_mpi stall | sort; exit \${PIPESTATUS[0]}" \
  allow-nonzero

step slurm_script "cluster_job.sh" "$(bash --version | head -n 1); $MPI" \
  "untested under Slurm: the build container has no Slurm, so bash ran the script and its fallback branch" \
  "mpic++ $MW job_mpi.cc -o job_mpi && MPIRUN_EXTRA='$ROOTOK' bash cluster_job.sh"
echo "hardware:  untested under Slurm and on a real multi-node cluster (AH-26)" >> slurm_script.log

rm -f .bin_cluster job_mpi
exit $status
