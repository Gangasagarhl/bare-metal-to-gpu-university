#!/usr/bin/env bash
# F4-44 run.sh: milestone G3 (read-only probing) on QEMU's emulated display devices.
#   build    : the F4-44 kernel (DR301 base + gpuprobe.cc + romparse.h)
#   probe    : VGA (primary), bochs-display and virtio-gpu-pci, inspected read-only
#   ati      : QEMU's ati-vga model (vendor 0x1002) as the only display device
#   compare  : ROM images read by the kernel versus the ROM files on the host (romcheck.cpp)
#   forensic : the probe built without the ROM BAR enable write
set -u -o pipefail
cd "$(dirname "$0")"
. ../F4-01/lablib.sh
status=0
SRC="../F4-01/boot.S ../F4-01/kbase.cc ../F4-01/div64.cc ../F4-02/pci.cc gpuprobe.cc f444_main.cc"
DEVS="-device VGA -device bochs-display -device virtio-gpu-pci"

kbuild k444.elf $SRC > .kb.txt 2>&1; rc=$?
{ cat .kb.txt; size -A k444.elf | grep -E '^(section|\.text|\.rodata|\.data|\.bss|Total)'; } > build.out
rec build "$SRC kernel.ld" "$GXX_VER; $LD_VER" \
    "g++ $KFLAGS -c <each file>; ld -m elf_i386 -nostdlib -static -T kernel.ld -o k444.elf *.o" "$rc"
[ "$rc" = 0 ] || status=1

qboot probe.out 120 q35 k444.elf $DEVS; rc=$?
rec probe "f444_main.cc, gpuprobe.cc (kernel k444.elf)" "$QEMU_VER" \
    "$QEMU -machine q35 $QCOMMON -serial stdio -kernel k444.elf $DEVS" "$rc" \
    "note:      exit code 33 = pass (isa-debug-exit 0x10)" "$HW_NOTE"
[ "$rc" = 33 ] || status=1

qboot ati.out 120 q35 k444.elf -device ati-vga; rc=$?
rec ati "f444_main.cc, gpuprobe.cc (kernel k444.elf)" "$QEMU_VER" \
    "$QEMU -machine q35 $QCOMMON -serial stdio -kernel k444.elf -device ati-vga" "$rc" \
    "note:      exit code 33 = pass; ati-vga is QEMU's model of an ATI card, with SeaBIOS's VGA BIOS as its ROM" "$HW_NOTE"
[ "$rc" = 33 ] || status=1

# compare: the hash of each image read through a ROM BAR against the hash of the ROM file
hostbuild .romcheck romcheck.cpp && ./.romcheck > .rc.txt; rc=$?
{ for pair in "vgabios-stdvga.bin:00:01.0:probe.out" "vgabios-bochs-display.bin:00:02.0:probe.out" "vgabios-ati.bin:00:01.0:ati.out"; do
    f="${pair%%:*}"; rest="${pair#*:}"; dev="${rest%:*}"; out="${rest##*:}"
    hf="$(grep "^$f " .rc.txt | grep -o 'fnv1a(image) 0x[0-9a-f]*' | awk '{print $2}')"
    hk="$(sed -n "/^$dev /,/^[0-9a-f][0-9a-f]:/p" "$out" | grep 'ROM via ROM BAR' | grep -o 'fnv1a 0x[0-9a-f]*' | awk '{print $2}')"
    printf '%-26s file %s  %s %-8s kernel %s  %s\n' "$f" "$hf" "$out" "$dev" "$hk" "$([ "$hf" = "$hk" ] && echo SAME || echo DIFFERENT)"
  done; } > compare.out
rec compare "romcheck.cpp, romparse.h; probe.out and ati.out" "$GXX_VER" \
    "g++ $HOSTFLAGS romcheck.cpp -o romcheck; ./romcheck; then the fnv1a values are compared line by line" "$rc"
[ "$rc" = 0 ] && [ "$(grep -c SAME compare.out)" = 2 ] || status=1

KEXTRA="-DF444_FORGET_ROM_ENABLE" kbuild k444f.elf $SRC > .kbf.txt 2>&1 || status=1
qboot forensic.out 120 q35 k444f.elf -device VGA -device bochs-display; rc=$?
rec forensic "f444_main.cc, gpuprobe.cc built with -DF444_FORGET_ROM_ENABLE" "$QEMU_VER" \
    "$QEMU -machine q35 $QCOMMON -serial stdio -kernel k444f.elf -device VGA -device bochs-display" "$rc" \
    "note:      exit code 33: the tool itself reports success" "$HW_NOTE"
[ "$rc" = 33 ] || status=1

rm -f k444.elf k444f.elf .kb.txt .kbf.txt .romcheck .rc.txt
exit $status
