#!/usr/bin/env bash
# F8-07 lab steps (MPI programs, so they are built with mpic++ and started by mpirun):
#   mpi_basics      - Listing 1 on 1, 2 and 4 processes
#   halo1d          - Listing 2 (milestone F4 stencil, host buffers) on 1, 2 and 4 processes
#   eager_limits    - ompi_info: the eager limits of the shared-memory (vader) and tcp transports
#   h2h_vader       - Listing 3, naive mode, default transports (shared memory): expected to hang;
#                     stopped by a 15 s time limit (exit code 124 recorded honestly)
#   h2h_tcp         - Listing 3, naive mode, forced onto the tcp transport (--mca btl tcp,self)
#   h2h_fixed       - Listing 3, MPI_Sendrecv mode: completes
# The container runs as root, so mpirun needs --allow-run-as-root; --oversubscribe lets it
# start more processes than it counts slots. -DOMPI_SKIP_MPICXX skips Open MPI's old C++
# binding header (the programs use only the C API), as in F5-22 and F8-01.
set -u
cd "$(dirname "$0")"
status=0
GXX="$(g++ --version | head -n 1)"
MPV="$(mpirun --version 2>&1 | head -n 1)"
MACHINE="$(uname -s) $(uname -m) (cloud build container, $(nproc) logical CPUs shared with other jobs; all MPI processes on this one machine, no GPU, no RDMA device)"
RUN="mpirun --allow-run-as-root --oversubscribe"
FLAGS="-std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror -DOMPI_SKIP_MPICXX"

header() {  # header <name> <listing> <command>
    {
        echo "listing:   $2"
        echo "toolchain: $MPV; $GXX"
        echo "command:   $3"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $MACHINE"
    } > "$1.log"
}
build() {  # build <source base name>
    if ! mpic++ $FLAGS "$1.cc" -o ".bin_F807_$1" > ".build_$1.txt" 2>&1; then
        cat ".build_$1.txt"; rm -f ".build_$1.txt"; return 1
    fi
    rm -f ".build_$1.txt"
}

for prog in mpi_basics halo1d; do
    header "$prog" "$prog.cc" "mpic++ $FLAGS $prog.cc -o $prog; for n in 1 2 4: $RUN -np \$n ./$prog"
    if ! build "$prog" >> "$prog.log"; then
        echo "result:    BUILD FAILED" >> "$prog.log"; status=1; continue
    fi
    : > "$prog.out"
    rc_all=0
    for n in 1 2 4; do
        echo "--- mpirun -np $n" >> "$prog.out"
        timeout -k 5 60 $RUN -np "$n" "./.bin_F807_$prog" >> "$prog.out" 2>&1 || rc_all=1
    done
    echo "exit code: $rc_all" >> "$prog.log"
    [ "$rc_all" = 0 ] || status=1
done

header eager_limits "(tool run, no listing)" "ompi_info --param btl vader --level 9; ompi_info --param btl tcp --level 9 (lines with eager_limit)"
{
    ompi_info --param btl vader --level 9 | grep -A1 '"btl_vader_eager_limit"'
    ompi_info --param btl tcp --level 9 | grep -A1 '"btl_tcp_eager_limit"'
} | sed 's/^ *//' > eager_limits.out 2>&1
echo "exit code: $?" >> eager_limits.log

build headtohead || status=1
run_h2h() {  # run_h2h <name> <mpirun extra options> <program argument>
    header "$1" "headtohead.cc" "mpic++ $FLAGS headtohead.cc -o headtohead; timeout -k 5 15 $RUN ${2:+$2 }-np 2 ./headtohead $3"
    timeout -k 5 15 $RUN ${2:+$2 }-np 2 ./.bin_F807_headtohead $3 > "$1.out" 2>&1
    rc=$?
    if [ "$rc" = 124 ]; then
        echo "exit code: 124 (stopped by the 15 s time limit: the program hung, as this forensic step expects)" >> "$1.log"
    else
        echo "exit code: $rc" >> "$1.log"
    fi
}
run_h2h h2h_vader "" naive
run_h2h h2h_tcp "--mca btl tcp,self" naive
run_h2h h2h_fixed "" fixed
grep -q "exit code: 0" h2h_fixed.log || status=1

rm -f .bin_F807_*
exit $status
