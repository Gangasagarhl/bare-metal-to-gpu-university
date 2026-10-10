#!/usr/bin/env bash
# F8-09 lab steps (the two-node stand-in: two MPI processes on ONE machine):
#   pingpong_shm      - Listing 1 over the default transports (shared memory between processes)
#   pingpong_tcp      - Listing 1 forced onto TCP (--mca btl tcp,self): the path a two-node
#                       job takes when no RDMA transport is usable
#   transport_default - which transport ("btl") Open MPI chose, from its own verbose log
#   transport_tcp     - the same with TCP forced
#   rdma_devices      - Listing 2, built against libibverbs; no RDMA device here (exit code 2
#                       expected)
#   environment       - what the container has for RDMA and GPUs (sysfs, device files, tools)
# Listing 3 (gdr_probe.cu) and Listing 4 (two_node_model.cpp) are built and run by
# run_lab.sh itself. MPI notes as in F8-07/run.sh.
set -u
cd "$(dirname "$0")"
status=0
GXX="$(g++ --version | head -n 1)"
MPV="$(mpirun --version 2>&1 | head -n 1)"
MACHINE="$(uname -s) $(uname -m) (cloud build container, $(nproc) logical CPUs shared with other jobs; all MPI processes on this one machine, no GPU, no RDMA device)"
RUN="mpirun --allow-run-as-root --oversubscribe"
FLAGS="-std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror -DOMPI_SKIP_MPICXX"

header() {  # header <name> <listing> <toolchain> <command>
    {
        echo "listing:   $2"
        echo "toolchain: $3"
        echo "command:   $4"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $MACHINE"
    } > "$1.log"
}

if mpic++ $FLAGS pingpong_net.cc -o .bin_F809_pp > .build.txt 2>&1; then
    for t in shm tcp; do
        opt=""; [ "$t" = tcp ] && opt="--mca btl tcp,self "
        header "pingpong_$t" pingpong_net.cc "$MPV; $GXX" "mpic++ $FLAGS pingpong_net.cc -o pingpong_net; $RUN ${opt}-np 2 ./pingpong_net"
        timeout -k 5 120 $RUN $opt -np 2 ./.bin_F809_pp > "pingpong_$t.out" 2>&1
        rc=$?; echo "exit code: $rc" >> "pingpong_$t.log"; [ "$rc" = 0 ] || status=1

        header "transport_$t" pingpong_net.cc "$MPV" "$RUN ${opt}--mca btl_base_verbose 30 -np 2 ./pingpong_net 2>&1 | grep -E 'Using .* btl|select: init|Using interface'"
        timeout -k 5 120 $RUN $opt --mca btl_base_verbose 30 -np 2 ./.bin_F809_pp 2>&1 \
            | grep -E "Using .* btl|select: init|Using interface" | sed -E 's/^\[[a-z0-9]+:[0-9]+\]/[host:pid]/' \
            | sort > "transport_$t.out"
        echo "exit code: ${PIPESTATUS[0]} (lines filtered with grep and sorted; host name and process id replaced by [host:pid])" >> "transport_$t.log"
    done
else
    echo "BUILD FAILED"; cat .build.txt; status=1
fi
rm -f .build.txt

header rdma_devices rdma_devices.cc "$GXX; libibverbs $(dpkg-query -W -f='${Version}' libibverbs-dev 2>/dev/null)" "g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror rdma_devices.cc -o rdma_devices -libverbs; ./rdma_devices"
echo "hardware:  untested on hardware: the build container has no RDMA device; exit code 2 means 'no device found' and is the expected result here" >> rdma_devices.log
if g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror rdma_devices.cc -o .bin_F809_rd -libverbs > .build.txt 2>&1; then
    ./.bin_F809_rd > rdma_devices.out 2>&1
    echo "exit code: $?" >> rdma_devices.log
else
    echo "result:    BUILD FAILED" >> rdma_devices.log; cat .build.txt >> rdma_devices.log; status=1
fi
rm -f .build.txt

header environment "(tool run, no listing)" "bash $(bash --version | head -n 1 | sed 's/GNU bash, version //')" "ls /sys/class/infiniband /dev/infiniband; command -v ib_write_bw ibv_devinfo nvidia-smi"
{
    echo "## ls /sys/class/infiniband"; ls /sys/class/infiniband 2>&1
    echo "## ls /dev/infiniband"; ls /dev/infiniband 2>&1
    for tool in ib_write_bw ibv_devinfo nvidia-smi; do
        echo "## command -v $tool"; command -v "$tool" || echo "(not installed)"
    done
} > environment.out
echo "exit code: 0" >> environment.log

rm -f .bin_F809_*
exit $status
