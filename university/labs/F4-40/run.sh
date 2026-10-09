#!/usr/bin/env bash
# F4-40 lab: the tiny hypervisor (AMD-V/SVM) under QEMU TCG with the svm CPU feature.
#   build      the hypervisor kernel          hello    a 64-bit guest prints; first exits traced
#   triple     a guest triple-faults          invalid  four invalid guest states, then a good one
#   hlt        a guest that waits with HLT    forensic_busy  the same wait by polling the PIT
# (exitdecode.cpp is built and run by run_lab.sh itself, with exitdecode.in.)
set -u
cd "$(dirname "$0")"
. ../F4-39/dr404lib.sh
status=0
L=../F4-39
SRC="$L/boot.S $L/kio.cc $L/intr.S $L/intr.cc vmrun.S hv.cc guests.cc f440_main.cc"
if KEXTRA="-I$PWD" kbuild hv.elf $SRC > build.out 2>&1; then
    size hv.elf >> build.out
    rec build "$SRC" "$GXX_VER" "KEXTRA=-I. ../F4-39/build_kernel.sh hv.elf $SRC" 0
else
    rec build "$SRC" "$GXX_VER" "../F4-39/build_kernel.sh hv.elf $SRC" 1 "result:    BUILD FAILED"
    exit 1
fi
CPU=qemu64,+svm,+npt
qrun hello "hv.cc, guests.cc, f440_main.cc (kernel hv.elf)" 1 hv32.elf $CPU 64M "test=hello trace=6" || status=1
qrun triple "hv.cc, guests.cc, f440_main.cc (kernel hv.elf)" 1 hv32.elf $CPU 64M "test=triple" || status=1
qrun invalid "hv.cc, guests.cc, f440_main.cc (kernel hv.elf)" 1 hv32.elf $CPU 64M "test=invalid" || status=1
qrun hlt "hv.cc, guests.cc (guest_hlt), cputime.py" 1 hv32.elf $CPU 64M "test=hlt" cputime || status=1
qrun forensic_busy "hv.cc, guests.cc (guest_busy), cputime.py" 1 hv32.elf $CPU 64M "test=busy" cputime || status=1
rm -f hv.elf hv32.elf
exit $status
