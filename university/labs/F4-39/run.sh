#!/usr/bin/env bash
# F4-39 lab: the good-guest kernel under QEMU (TCG), as a guest that detects its hypervisor,
# idles correctly (or not: the forensic run) and keeps a monotonic clock.
#   build       the kernel                      detect / detect_nohv  hypervisor detection
#   idle_hlt    idle with HLT, host CPU time    forensic_poll         idle by polling the PIT
#   mono        10 million clock reads
# (hostguest.cpp and pvclock_model.cpp are built and run by run_lab.sh itself.)
set -u
cd "$(dirname "$0")"
. ./dr404lib.sh
status=0
SRC="boot.S kio.cc intr.S intr.cc hvdetect.cc goodguest.cc"
if kbuild goodguest.elf $SRC > build.out 2>&1; then
    size goodguest.elf >> build.out
    rec build "$SRC kernel.ld build_kernel.sh" "$GXX_VER" "./build_kernel.sh goodguest.elf $SRC" 0
else
    rec build "$SRC" "$GXX_VER" "./build_kernel.sh goodguest.elf $SRC" "1" "result:    BUILD FAILED"
    exit 1
fi
qrun detect "goodguest.cc (kernel goodguest.elf)" 1 goodguest32.elf qemu64 64M "test=detect" || status=1
qrun detect_nohv "goodguest.cc (kernel goodguest.elf)" 1 goodguest32.elf qemu64,-hypervisor 64M "test=detect" || status=1
qrun idle_hlt "goodguest.cc (kernel goodguest.elf), cputime.py" 1 goodguest32.elf qemu64 64M "test=idle idle=hlt seconds=3" cputime || status=1
qrun forensic_poll "goodguest.cc (kernel goodguest.elf), cputime.py" 1 goodguest32.elf qemu64 64M "test=idle idle=poll seconds=3" cputime || status=1
qrun mono "goodguest.cc (kernel goodguest.elf)" 1 goodguest32.elf qemu64 64M "test=mono reads=10000000" || status=1
rm -f goodguest.elf goodguest32.elf
exit $status
