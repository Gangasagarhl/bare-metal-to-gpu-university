#!/usr/bin/env bash
# BR-04 lab steps that need mpirun (the .cpp and .cu listings are built and run by run_lab.sh).
#   peer_matrix_shm  - Listing 3, 4 processes, Open MPI shared-memory transport (btl self,vader)
#   peer_matrix_shm2 - the same run again, a few seconds later (is the matrix repeatable?)
#   peer_matrix_tcp  - Listing 3, 4 processes, TCP over the loopback device (btl self,tcp)
#   roofline_measured- Listing 1 fed with the two measured (alpha, beta) fits
#   ring_shm         - Listing 4: prediction from the shm fit printed first, then the run
#   ring_tcp         - Listing 4 with the TCP fit and the TCP transport
#   mismatch_ready   - Listing 5, rank-dependent bucket order (expected to hang: limit 10 s, exit 124)
#   mismatch_fixed   - Listing 5, agreed bucket order (expected exit 0)
#   fail_one         - Listing 6, rank 2 kills itself in step 4 (expected: the job stops, exit != 0)
#   errno14          - the name of error number 14, printed by mismatch_ready
#   ompi_yield       - Open MPI's own description of mpi_yield_when_idle
#   cuda_api         - the declarations of the CUDA calls of Listing 8, read from the installed header
# Measurement builds use -O2 without sanitizers (sanitizers change timing). MPI notes as in
# F8-01/run.sh, plus mpi_yield_when_idle 1: a waiting process gives its CPU away instead of
# spinning (the container's 4 CPUs are shared with other jobs; without it, runs in this build
# showed one-way times of milliseconds whenever a spinning process lost its CPU);
# --allow-run-as-root (the container runs as root), --oversubscribe,
# -DOMPI_SKIP_MPICXX (Open MPI's old C++ header does not compile under -Werror with g++ 13).
set -u
cd "$(dirname "$0")"
status=0
GXX="$(g++ --version | head -n 1)"
MPV="$(mpirun --version 2>&1 | head -n 1)"
MACHINE="$(uname -s) $(uname -m) (cloud build container, $(nproc) logical CPUs shared with other jobs; MPI processes on one machine, no GPU)"
RUN="mpirun --allow-run-as-root --oversubscribe"
FLAGS="-std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror -DOMPI_SKIP_MPICXX"
SHM="--mca btl self,vader --mca mpi_yield_when_idle 1"
TCP="--mca btl self,tcp --mca btl_tcp_if_include lo --mca mpi_yield_when_idle 1"

header() {  # header <out name> <listing> <toolchain> <command>
    {
        echo "listing:   $2"
        echo "toolchain: $3"
        echo "command:   $4"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $MACHINE"
    } > "$1.log"
}
build() {  # build <source base name>
    if ! mpic++ $FLAGS "$1.cc" -o ".bin_BR04_$1" > ".build_$1.txt" 2>&1; then
        cat ".build_$1.txt"; rm -f ".build_$1.txt"; return 1
    fi
    rm -f ".build_$1.txt"
}
# mpistep <out name> <source> <limit s> <expected: 0 | 124 | nonzero> <mpirun options> -- <program args>
mpistep() {
    local out="$1" src="$2" limit="$3" expect="$4"; shift 4
    local opts=(); while [ "$1" != "--" ]; do opts+=("$1"); shift; done; shift
    header "$out" "$src.cc" "$MPV; $GXX" \
        "mpic++ $FLAGS $src.cc -o $src; timeout $limit $RUN ${opts[*]} -np 4 ./$src $*"
    if [ ! -x ".bin_BR04_$src" ]; then
        echo "result:    BUILD FAILED" >> "$out.log"; status=1; return
    fi
    timeout "$limit" $RUN "${opts[@]}" -np 4 "./.bin_BR04_$src" "$@" > ".run_$out.txt" 2>&1
    local rc=$?
    pkill -9 -f "[.]bin_BR04_$src" 2>/dev/null
    # machine-specific words (host name, process ids) replaced by placeholders (AH-25)
    sed -e "s/$(hostname)/<host>/g" -e 's/PID [0-9]\+/PID <pid>/g' -e 's/pid [0-9]\+/pid <pid>/g' \
        ".run_$out.txt" > "$out.out"
    rm -f ".run_$out.txt"
    if [ "$rc" = 124 ]; then
        echo "exit code: 124 (stopped by the $limit s time limit)" >> "$out.log"
    else
        echo "exit code: $rc" >> "$out.log"
    fi
    case "$expect" in
        0)   [ "$rc" = 0 ] || { echo "result:    UNEXPECTED (expected exit code 0)" >> "$out.log"; status=1; } ;;
        124) if [ "$rc" = 124 ]; then
                 echo "result:    expected: this program is broken on purpose (trap 3), it hangs" >> "$out.log"
             else
                 echo "result:    UNEXPECTED (expected a hang, exit code 124)" >> "$out.log"; status=1
             fi ;;
        nonzero) if [ "$rc" != 0 ] && [ "$rc" != 124 ]; then
                 echo "result:    expected: one rank was killed on purpose; mpirun stopped the job" >> "$out.log"
             else
                 echo "result:    UNEXPECTED (expected mpirun to stop the job with a non-zero code)" >> "$out.log"; status=1
             fi ;;
    esac
}

for s in peer_matrix ring_allreduce mismatch fail_one; do
    build "$s" || status=1
done

# 1. the peer matrix on two transports, and the fits
mpistep peer_matrix_shm peer_matrix 300 0 $SHM --
mpistep peer_matrix_shm2 peer_matrix 300 0 $SHM --
mpistep peer_matrix_tcp peer_matrix 300 0 $TCP --
read -r a_shm b_shm < <(grep '^fit:' peer_matrix_shm.out | awk '{print $2, $3}')
read -r a_tcp b_tcp < <(grep '^fit:' peer_matrix_tcp.out | awk '{print $2, $3}')

# 2. the link roofline of the two measured links (Listing 1 again, measured input)
header roofline_measured "link_roofline.cpp (input: the two 'fit:' lines above)" "$GXX" \
  "g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined link_roofline.cpp -o link_roofline; printf 'shm(measured) <alpha> <beta>\\ntcp_loopback(measured) <alpha> <beta>\\n' | ./link_roofline"
if g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined link_roofline.cpp \
        -o .bin_BR04_roofline; then
    printf 'shared_memory_(measured) %s %s\nTCP_loopback_(measured) %s %s\n' "$a_shm" "$b_shm" "$a_tcp" "$b_tcp" \
        | ./.bin_BR04_roofline > roofline_measured.out 2>&1; rc=$?
    echo "exit code: $rc" >> roofline_measured.log; [ "$rc" = 0 ] || status=1
else
    echo "result:    BUILD FAILED" >> roofline_measured.log; status=1
fi
rm -f .bin_BR04_roofline

# 3. predict, then run: the ring all-reduce on each transport
mpistep ring_shm ring_allreduce 300 0 $SHM -- "$a_shm" "$b_shm" shared-memory-fit
mpistep ring_tcp ring_allreduce 300 0 $TCP -- "$a_tcp" "$b_tcp" TCP-loopback-fit

# 4. trap 3 and the fix; 5. one failing process
mpistep mismatch_ready mismatch 10 124 $SHM -- ready
mpistep mismatch_fixed mismatch 60 0 $SHM -- fixed
mpistep fail_one fail_one 60 nonzero $SHM --

rm -f .bin_BR04_peer_matrix .bin_BR04_ring_allreduce .bin_BR04_mismatch .bin_BR04_fail_one

# 6. the CUDA declarations used by Listing 8, from the installed header (not from memory)
H=/usr/include/cuda_runtime_api.h
header cuda_api "grep of $H" "$(nvcc --version | tail -n 2 | head -n 1); header $H" \
  "grep -E 'extern .*(cudaDeviceCanAccessPeer|cudaDeviceEnablePeerAccess|cudaMemcpyPeerAsync|cudaEventElapsedTime|cudaSetDevice)\\(' $H"
grep -E 'extern .*(cudaDeviceCanAccessPeer|cudaDeviceEnablePeerAccess|cudaMemcpyPeerAsync|cudaEventElapsedTime|cudaSetDevice)\(' "$H" \
    | sed 's/^extern __host__ \(__cudart_builtin__ \)\?//' > cuda_api.out; rc=$?
echo "exit code: $rc" >> cuda_api.log; [ "$rc" = 0 ] || status=1

# 7. what errno 14 (printed in mismatch_ready.out) means, asked from the C library via Python
header errno14 "python3 one-liner (errno module and os.strerror)" "$(python3 --version 2>&1)" \
  "python3 -c 'import errno, os; print(14, errno.errorcode[14], os.strerror(14))'"
python3 -c 'import errno, os; print(14, errno.errorcode[14], os.strerror(14))' > errno14.out 2>&1; rc=$?
echo "exit code: $rc" >> errno14.log; [ "$rc" = 0 ] || status=1
# 8. what Open MPI itself says about mpi_yield_when_idle (used for every run above)
header ompi_yield "ompi_info of the installed Open MPI" "$MPV" \
  "ompi_info --param mpi all --level 9 | grep -A 1 'mpi_yield_when_idle'"
ompi_info --param mpi all --level 9 2>&1 | grep -A 1 'mpi_yield_when_idle' | sed 's/^ *//' > ompi_yield.out; rc=$?
echo "exit code: $rc" >> ompi_yield.log; [ "$rc" = 0 ] || status=1
exit $status
