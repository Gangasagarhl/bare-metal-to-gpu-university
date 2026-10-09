#!/usr/bin/env bash
# F2-54 steps: the same source built so that the compiler keeps or removes the branch,
# timings on sorted and shuffled data, the instructions, and cachegrind's branch simulation.
set -u -o pipefail
cd "$(dirname "$0")"
TC="$(g++ --version | head -n 1)"
OD="$(objdump --version | head -n 1)"
VG="$(valgrind --version)"
MACH="$(uname -s) $(uname -m) (cloud build container, $(nproc) CPUs); CPU model: $(grep -m1 'model name' /proc/cpuinfo | cut -d: -f2 | sed 's/^ //')"
NOTE="note:      measured on the build container (a shared cloud virtual machine, other jobs may run at the same time), one session; not a specification (AH-23)"
rec() {
    {
        echo "listing:   $2"
        echo "toolchain: $3"
        echo "command:   $4"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $MACH"
        echo "exit code: $5"
        if [ $# -ge 6 ]; then echo "$6"; fi
    } > "$1.log"
}
status=0
W="-std=c++20 -Wall -Wextra -Wpedantic -Werror"
for v in cmov jump; do
    if [ $v = cmov ]; then F="-O2"; else F="-O2 -fno-if-conversion -fno-if-conversion2"; fi
    g++ $W $F branches.cc -o .bin_$v || exit 1
    timeout 300 ./.bin_$v > branches_$v.out 2>&1; rc=$?; [ $rc = 0 ] || status=1
    rec branches_$v branches.cc "$TC" "g++ $W $F branches.cc -o branches && ./branches" $rc "$NOTE"
    objdump -d -C --no-show-raw-insn .bin_$v | awk '/^[0-9a-f]+ <sumBranchy/,/^$/' \
        | sed -E 's/<sumBranchy\(std::vector<unsigned char, std::allocator<unsigned char> > const&\)/<sumBranchy/' > asm_$v.out; rc=$?
    rec asm_$v branches.cc "$TC; $OD" "g++ $W $F branches.cc -o branches && objdump -d -C --no-show-raw-insn branches (sumBranchy only; long name shortened)" $rc
done
# Forensic evidence: the jump build, cachegrind with branch simulation, one input order per run.
for d in sorted shuffled; do
    CG="valgrind --tool=cachegrind --cache-sim=no --branch-sim=yes ./branches 1048576 1 $d"
    valgrind --tool=cachegrind --cache-sim=no --branch-sim=yes --cachegrind-out-file=.cg.tmp \
        ./.bin_jump 1048576 1 $d > .cg.stdout 2> .cg.stderr; rc=$?
    {
        echo "--- program output"; cat .cg.stdout
        echo "--- cachegrind summary (process id replaced by PID)"
        sed -E 's/^(==|--)[0-9]+(==|--)/\1PID\2/' .cg.stderr | grep -E 'refs|Branches|Mispred|Mispred rate'
        echo "--- cg_annotate: per-function counts"
        cg_annotate .cg.tmp 2>&1 | grep -E '^Events shown|^ +[0-9,]+ .*(sumBranchy|sumBranchless)\(' \
            | sed -E 's/\(std::vector<unsigned char, std::allocator<unsigned char> > const&\)//; s/ +/ /g'
    } > cg_$d.out 2>&1
    rec cg_$d branches.cc "$VG; $TC" "g++ $W -O2 -fno-if-conversion -fno-if-conversion2 branches.cc -o branches && $CG && cg_annotate" $rc \
        "note:      branch behaviour simulated by cachegrind's own simple predictor model, not measured by hardware counters"
done
rm -f .bin_cmov .bin_jump .cg.tmp .cg.stdout .cg.stderr
exit $status
