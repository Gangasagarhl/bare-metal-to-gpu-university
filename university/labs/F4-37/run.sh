#!/usr/bin/env bash
# F4-37 run.sh: one kernel image (the F4-36 build), byte-identical, booted on two QEMU machines
# with different SoC layouts: virt (GIC, PSCI, QEMU's own devicetree) and raspi3b (BCM2837
# layout, no GIC, our devicetree). Plus: how each loader changed or supplied the devicetree, the
# relocation mistake, and the forensic evidence (a kernel that is silent on the second board).
set -u -o pipefail
cd "$(dirname "$0")"
. ../F4-31/lablib.sh
status=0
B=.build
rm -rf "$B"; mkdir -p "$B"
ok() { [ "$1" = "$2" ] || { echo "F4-37 step $3: exit $1, expected $2" >&2; status=1; }; }
SRC="../F4-35/kmain.cc ../F4-35/dm.cc ../F4-35/drivers.cc ../F4-35/bcm2835_gpio.cc ../F4-36/sdhci.cc"
VIRT="qemu-system-aarch64 -M virt -cpu cortex-a53 -smp 2 -m 512M -nographic -semihosting -device virtio-rng-device"
RPI="qemu-system-aarch64 -M raspi3b -nographic -semihosting"

# 1. one build
{ kbuild $B/kernel.bin $B/o $SRC && python3 ../F4-31/minidtc.py ../F4-36/raspi3b.dts $B/raspi3b.dtb > /dev/null &&
  python3 ../F4-36/mksd.py $B/card.img 64 > /dev/null &&
  echo "kernel.bin: $(stat -c %s $B/kernel.bin) bytes, SHA-256 $(sha256sum < $B/kernel.bin | cut -c1-64)" &&
  echo "relocations applied at start-up: $(aarch64-linux-gnu-readelf -r $B/kernel.elf | grep -c R_AARCH64_RELATIV) (all R_AARCH64_RELATIVE)" &&
  echo "drivers in the image: $(aarch64-linux-gnu-nm $B/kernel.elf | grep -c '_driverE$')"; } > kbuild.out 2>&1; rc=$?
rec kbuild "the F4-35 and F4-36 kernel files (no board option exists)" "$XGXX_VER; $XLD_VER" \
    "aarch64-linux-gnu-g++ $KFLAGS -fpie -c <file>; ld -pie -T ../F4-31/kernel.ld; objcopy -O binary; sha256sum" "$rc"
ok "$rc" 0 kbuild

# 2. the same file on both machines; the hash is taken again right before each run
sha_v="$(sha256sum < $B/kernel.bin | cut -c1-64)"
qrun 60 -- $VIRT -kernel $B/kernel.bin > two_virt.out; rc=$?
sed -i 's/RTCDR [0-9]* then [0-9]*/RTCDR <t> then <t+1>/' two_virt.out
rec two_virt "kernel.bin (SHA-256 $sha_v)" "$QEMU_VER" "$VIRT -kernel kernel.bin" "$rc" \
    "note:      exit code 0 = PSCI SYSTEM_OFF; RTC readings replaced by <t> and <t+1>" "$EMU_NOTE"
ok "$rc" 0 two_virt
sha_r="$(sha256sum < $B/kernel.bin | cut -c1-64)"
qrun 60 -- $RPI -kernel $B/kernel.bin -dtb $B/raspi3b.dtb -drive if=sd,format=raw,file=$B/card.img > two_rpi.out; rc=$?
rec two_rpi "kernel.bin (SHA-256 $sha_r)" "$QEMU_VER" "$RPI -kernel kernel.bin -dtb raspi3b.dtb -drive if=sd,format=raw,file=card.img" "$rc" \
    "note:      exit code 0 = semihosting exit (this machine's tree has no PSCI node)" "$EMU_NOTE"
ok "$rc" 0 two_rpi
python3 d10check.py "$sha_v" "$sha_r" two_virt.out 512 2 two_rpi.out 960 4 > d10_check.out; rc=$?
rec d10_check d10check.py "$PY_VER" "python3 d10check.py <sha> <sha> two_virt.out 512 2 two_rpi.out 960 4" "$rc"
ok "$rc" 0 d10_check

# 3. what each loader handed over: QEMU's raspi3b loader edits our tree (dumpdtb shows the result)
$RPI -kernel $B/kernel.bin -dtb $B/raspi3b.dtb -machine dumpdtb=$B/handed.dtb > /dev/null 2>&1
hostbuild $B/fdtdump ../F4-31/fdtdump.cc > /dev/null 2>&1
{ echo "== memory node and /chosen in OUR raspi3b.dtb =="
  $B/fdtdump $B/raspi3b.dtb | grep -A3 -E '^    (memory@0|chosen) \{' | grep -v -- '--'
  echo "== the same nodes in the tree QEMU's loader handed to the kernel =="
  $B/fdtdump $B/handed.dtb | grep -A4 -E '^    (memory@0|chosen) \{' | grep -v -- '--'
  echo "== total sizes: ours $(stat -c %s $B/raspi3b.dtb) bytes; handed over: $($B/fdtdump $B/handed.dtb | head -n 1 | sed 's/.*totalsize \([0-9]*\).*/\1/') bytes"; } > rpi_dtb.out 2>&1
rec rpi_dtb "../F4-31/fdtdump.cc" "$QEMU_VER; $GXX_VER" "$RPI ... -dtb raspi3b.dtb -machine dumpdtb=handed.dtb; fdtdump both; grep memory and chosen" "0"

# 4. the raspi3b memory map in QEMU's two views: the VideoCore bus and the CPU
printf 'info mtree -f\nquit\n' | hmp qemu-system-aarch64 -M raspi3b > $B/rm.txt
{ echo "== bus view (QEMU's flat view of the address space rooted at bcm2835-gpu) =="
  awk '/^FlatView/{k=0} /root: bcm2835-gpu/{k=1} k' $B/rm.txt | grep -E ': (pl011|sdhci|bcm2835_gpio)$' | sed 's/^ *//' | head -n 3
  echo "== CPU view (address space \"memory\") =="
  memory_view < $B/rm.txt | grep -E ': (pl011|sdhci|bcm2835_gpio|bcm2836-control)$|0000000000000000-' | sed 's/^ *//' | head -n 5; } > rpi_mtree.out
rec rpi_mtree "QEMU monitor" "$QEMU_VER" "qemu-system-aarch64 -M raspi3b -S -monitor stdio; (qemu) info mtree -f; filtered" "0"

# 5. common mistake: the linker script from an earlier draft (relocation table bounds inside it)
sed 's/^    .rela.dyn : ALIGN(8) { \*(.rela .rela.\*) }/    .rela.dyn : ALIGN(8) { __rela_start = .; *(.rela .rela.*) __rela_end = .; }/; /^    __rela_start = ADDR/d; /^    __rela_end = ADDR/d' \
    ../F4-31/kernel.ld > $B/old.ld
cp ../F4-31/kernel.ld $B/kernel.ld.keep
{ mkdir -p $B/ol && for f in ../F4-31/start.S ../F4-31/kbase.cc $SRC; do
      $XGXX $KFLAGS -fpie -c $f -o $B/ol/$(basename ${f%.*}).o || exit 1; done
  $XLD -pie --no-dynamic-linker -T $B/old.ld -o $B/old.elf $B/ol/*.o && aarch64-linux-gnu-objcopy -O binary $B/old.elf $B/old.bin; } > $B/ob.txt 2>&1
{ echo "relocations in the image: $(aarch64-linux-gnu-readelf -r $B/old.elf | grep -c R_AARCH64_RELATIV)"
  echo "table bounds the start code uses: __rela_start..__rela_end = $(( ( 0x$(aarch64-linux-gnu-nm $B/old.elf | awk '/ __rela_end$/{print $1}') - 0x$(aarch64-linux-gnu-nm $B/old.elf | awk '/ __rela_start$/{print $1}') ) / 24 )) entries"
  echo "== boot on virt =="
  qrun 30 -- $VIRT -kernel $B/old.bin | sed 's/RTCDR [0-9]* then [0-9]*/RTCDR <t> then <t+1>/' | head -n 8; } > reloc_mistake.out
rec reloc_mistake "kernel.ld with __rela_start/__rela_end defined inside the .rela.dyn section" "$XLD_VER; $QEMU_VER" \
    "ld -pie -T old.ld ...; readelf -r; nm; $VIRT -kernel old.bin | head -n 8" "0" "$EMU_NOTE"

# 6. forensic evidence: the bring-up branch's kernel (see the answer key)
sed 's/    if (!early_console(t)) {/    g_early_uart = 0x09000000;   \/\/ bring-up shortcut\n    k::set_console(early_putc);\n    if (false \&\& !early_console(t)) {/' \
    ../F4-35/kmain.cc | sed 's#include "dm.h"#include "../F4-35/dm.h"#' > .kmain_bringup.cc
kbuild $B/bringup.bin $B/ob .kmain_bringup.cc ../F4-35/dm.cc ../F4-35/drivers.cc ../F4-35/bcm2835_gpio.cc ../F4-36/sdhci.cc > $B/fb.txt 2>&1
qrun 60 -- $VIRT -kernel $B/bringup.bin | head -n 3 > forensic_virt.out; rc=${PIPESTATUS[0]}
rec forensic_virt "the bring-up branch kernel (evidence pack)" "$QEMU_VER" "$VIRT -kernel bringup.bin | head -n 3" "$rc" "$EMU_NOTE"
python3 ../F4-36/mksd.py $B/card2.img 64 > /dev/null
qrun 60 -- $RPI -kernel $B/bringup.bin -dtb $B/raspi3b.dtb -drive if=sd,format=raw,file=$B/card2.img > forensic_rpi.out; rc=$?
{ echo "serial output: $(wc -c < forensic_rpi.out) bytes"; } >> forensic_rpi.out
rec forensic_rpi "the bring-up branch kernel (evidence pack)" "$QEMU_VER" "$RPI -kernel bringup.bin -dtb raspi3b.dtb -drive if=sd,..." "$rc" \
    "note:      exit code 0: the run ended normally (semihosting exit)" "$EMU_NOTE"
ok "$rc" 0 forensic_rpi
python3 ../F4-36/verify_image.py $B/card2.img 2048 32 > forensic_card.out; rc=$?
rec forensic_card ../F4-36/verify_image.py "$PY_VER" "python3 ../F4-36/verify_image.py card2.img 2048 32 (the card used by the silent run)" "$rc"
ok "$rc" 0 forensic_card

rm -rf "$B" .kmain_bringup.cc
exit $status
