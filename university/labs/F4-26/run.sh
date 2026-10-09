#!/usr/bin/env bash
# F4-26 run.sh: build the D3 kernel (PSCI SMP, per-CPU data, lock, SGIs, virtio-mmio), boot it
# with 1, 2 (entered at EL2), 4 and 8 CPUs, show the legacy virtio-mmio default, and compile
# publish.cc for three architectures to compare the ordering instructions they get.
# (reorder_sim.cpp, the forensic model, is built and run by run_lab.sh itself.)
set -u -o pipefail
cd "$(dirname "$0")"
. ../F4-23/dlab.sh
status=0
SRC="../F4-24/boot.S ../F4-24/vectors.S ../F4-24/trap.cc ../F4-24/pl011.cc ../F4-24/psci.cc \
../F4-25/mmu.cc ../F4-25/gic.cc smp.S virtio_mmio.cc d3_main.cc"

# 1. build (F4-25's page-aligned linker script; F4-25 headers on the include path)
KLDS=../F4-25/linker.ld kbuild aarch64 k_d3 "-I../F4-25" $SRC; rc=$?
aarch64-linux-gnu-nm k_d3.elf | grep -E ' (secondary_entry|secondary_main|kmain)$' > build.out 2>&1
rec build "dlab.sh kbuild; F4-24 + F4-25 files + smp.S virtio_mmio.cc d3_main.cc + F3-18 files" "$A64_VER" \
    "KLDS=../F4-25/linker.ld kbuild aarch64 k_d3 \"-I../F4-25\" $SRC" "$rc"
[ "$rc" = 0 ] || status=1

# 2. a 1 MiB disk image whose first sector starts with a known sentence
printf 'DR402 F4-26: sector 0, read through virtio-mmio' > .disk.img
truncate -s 1M .disk.img
DISK="-global virtio-mmio.force-legacy=false -drive if=none,file=.disk.img,format=raw,id=d0 -device virtio-blk-device,drive=d0"
boot() {   # boot <name> <timeout> <machine options> <extra QEMU options>
    local name="$1" t="$2" m="$3" extra="$4"
    local q="qemu-system-aarch64 -M $m -cpu cortex-a57 -m 256M -display none -monitor none -serial stdio -no-reboot"
    local start end rc
    start=$(date +%s%N)
    timeout "$t" $q $extra -kernel k_d3.bin < /dev/null > "$name.out" 2>&1; rc=$?
    end=$(date +%s%N)
    rec "$name" "d3_main.cc" "$QEMU_A64_VER" "$q $extra -kernel k_d3.bin" "$rc" \
        "note:      wall time $(( (end - start) / 1000000 )) ms on a host with $(nproc) CPUs (TCG, multi-threaded)" "$HW_NOTE"
    return $rc
}
boot smp1 60 "virt,gic-version=3 -smp 1" "$DISK"; grep -q '^D3 ok' smp1.out || status=1
boot smp2_el2 60 "virt,gic-version=3,virtualization=on -smp 2" "$DISK"; grep -q '^D3 ok' smp2_el2.out || status=1
boot smp4 60 "virt,gic-version=3 -smp 4" "$DISK"; grep -q '^D3 ok' smp4.out || status=1
boot smp8 60 "virt,gic-version=3 -smp 8" "$DISK -append rounds=1000"; grep -q '^D3 ok' smp8.out || status=1
# an observation, not a pass: 8 vCPUs on a 4-CPU host with 4000 rounds is slow and varies (one
# run of this build did not finish in 30 s, another took about 23 s); the chapter discusses why
boot smp8_slow 30 "virt,gic-version=3 -smp 8" "$DISK -append rounds=4000"
[ "$?" = 124 ] || echo "note: smp8_slow finished this time (see smp8_slow.out)"
# QEMU's default virtio-mmio transport is the legacy one: the driver must refuse it clearly
boot legacy 60 "virt,gic-version=3 -smp 1" "-drive if=none,file=.disk.img,format=raw,id=d0 -device virtio-blk-device,drive=d0"
grep -q 'legacy): unsupported' legacy.out || status=1

# 3. the same publish code compiled for three CPUs
{
    for cxx in g++ aarch64-linux-gnu-g++ riscv64-linux-gnu-g++; do
        dump=objdump; [ "$cxx" != g++ ] && dump="${cxx%-g++}-objdump"
        echo "== $($cxx --version | head -n 1): $cxx -std=c++20 -O2 -c publish.cc =="
        $cxx -std=c++20 -O2 -Wall -Wextra -Werror -c publish.cc -o .publish.o
        $dump -d --no-show-raw-insn -C .publish.o | grep -E '^ +[0-9a-f]+:|^[0-9a-f]+ <' | sed 's/(Ring\*, unsigned long, unsigned int)//'
    done
} > publish.out 2>&1; rc=$?
rec publish "publish.cc (compiled only)" "$HOST_VER; $A64_VER; $RV_VER" \
    "<cxx> -std=c++20 -O2 -c publish.cc; objdump -d --no-show-raw-insn -C" "$rc"
rm -f .publish.o .disk.img k_d3.elf k_d3.bin
exit $status
