#!/usr/bin/env bash
# F2-53 steps: the vectoriser's own reports, the generated instructions, and timings.
set -u -o pipefail
cd "$(dirname "$0")"
TC="$(g++ --version | head -n 1)"
OD="$(objdump --version | head -n 1)"
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
# 1. What does -march=native mean on this machine? (GCC's own answer)
g++ -Q --help=target -march=native 2>&1 | grep -E '^ +-march=|^ +-mavx2 |^ +-mfma |^ +-mavx512f |^ +-mprefer-vector-width=' \
    | sed -E 's/[[:space:]]+/ /g' > native.out; rc=$?
rec native "(none)" "$TC" "g++ -Q --help=target -march=native | grep -E 'march=|mavx2|mfma|mavx512f|mprefer-vector-width'" $rc
# 2. Vectoriser reports for kernels.cc, baseline target and native target.
for v in base native; do
    if [ $v = base ]; then F="-O3"; else F="-O3 -march=native"; fi
    { echo "--- optimized:"; g++ $W $F -c kernels.cc -o /dev/null -fopt-info-vec-optimized 2>&1;
      echo "--- missed (first reason per loop):"; g++ $W $F -c kernels.cc -o /dev/null -fopt-info-vec-missed 2>&1 \
        | grep -E 'missed: (couldn.t vectorize|not vectorized:)' | awk '!seen[$0]++'; } > vec_$v.out; rc=$?
    rec vec_$v kernels.cc "$TC" "g++ $W $F -c kernels.cc -fopt-info-vec-optimized; same with -fopt-info-vec-missed" $rc
done
# 3. The instructions of sumFloat, strict IEEE order versus -ffast-math (forensic evidence).
for v in strict fast; do
    if [ $v = strict ]; then F="-O3 -march=native"; else F="-O3 -march=native -ffast-math"; fi
    g++ $W $F -c kernels.cc -o .k.o || exit 1
    objdump -d -C --no-show-raw-insn .k.o | awk '/^[0-9a-f]+ <sumFloat/,/^$/' | head -n 40 > sumfloat_$v.out; rc=$?
    rec sumfloat_$v kernels.cc "$TC; $OD" "g++ $W $F -c kernels.cc && objdump -d -C --no-show-raw-insn kernels.o (sumFloat only, first 40 lines)" $rc
done
# 4. The inner loop of saxpyPlain at -O3 -march=native.
g++ $W -O3 -march=native -c simd.cc -o .s.o || exit 1
objdump -d -C --no-show-raw-insn .s.o | awk '/^[0-9a-f]+ <saxpyPlain/,/^$/' | grep -E 'vfmadd|vmul|vadd|vmov|<saxpyPlain' | head -n 12 > saxpy_asm.out; rc=$?
rec saxpy_asm simd.cc "$TC; $OD" "g++ $W -O3 -march=native -c simd.cc && objdump -d (saxpyPlain: SIMD arithmetic and moves only, first 12 lines)" $rc
# 5. Timings, three builds of the same source.
i=0
for F in "-O2 -fno-tree-vectorize" "-O3" "-O3 -march=native"; do
    i=$((i+1))
    g++ $W $F simd.cc -o .bin_simd || exit 1
    timeout 300 ./.bin_simd > simd_$i.out 2>&1; rc=$?; [ $rc = 0 ] || status=1
    rec simd_$i simd.cc "$TC" "g++ $W $F simd.cc -o simd && ./simd" $rc "$NOTE"
done
rm -f .k.o .s.o .bin_simd
exit $status
