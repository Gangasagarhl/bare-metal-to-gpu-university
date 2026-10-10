#!/usr/bin/env bash
# F4-06 run.sh: milestone C5 in QEMU (q35's built-in ICH9 AHCI controller).
#   build     : the F4-06 kernel
#   ahci      : IDENTIFY against QEMU's configuration, random I/O on AHCI and virtio-blk,
#               an error and its recovery
#   image     : the host checks the AHCI disk image (verify_image.py of F4-05)
#   ahci_trace: QEMU's own trace of the AHCI register accesses during initialisation (excerpt)
#   boottrace : healthy and slow boot timelines -> boottrace.csv (the forensic evidence)
set -u -o pipefail
cd "$(dirname "$0")"
. ../F4-01/lablib.sh
status=0
SRC="../F4-01/boot.S ../F4-01/kbase.cc ../F4-01/div64.cc ../F4-02/pci.cc ../F4-03/isr.S ../F4-03/irq.cc \
../F4-03/i8042.cc ../F4-04/rtc.cc ../F4-05/virtio.cc ../F4-05/virtio_blk.cc ../F4-05/iotest.cc ahci.cc f406_main.cc"
KEXTRA="-I../F4-03"
SATA="-drive file=.sata.img,if=none,id=sd0,format=raw -device ide-hd,drive=sd0,bus=ide.0,model=DR301 SATA DISK,serial=DR301SATA0001"
SATA_SHOWN="-drive file=sata.img,if=none,id=sd0,format=raw -device ide-hd,drive=sd0,bus=ide.0,model=DR301 SATA DISK,serial=DR301SATA0001"
VBLK="-drive file=.vblk.img,if=none,id=vd0,format=raw -device virtio-blk-pci,drive=vd0,disable-legacy=on"
VBLK_SHOWN="-drive file=vblk.img,if=none,id=vd0,format=raw -device virtio-blk-pci,drive=vd0,disable-legacy=on"

kbuild k406.elf $SRC > .kb.txt 2>&1; rc=$?
{ cat .kb.txt; size -A k406.elf | grep -E '^(section|\.text|\.rodata|\.data|\.bss|Total)'; } > build.out
rec build "$SRC kernel.ld" "$GXX_VER; $LD_VER" \
    "g++ $KFLAGS -I../F4-03 -c <each file>; ld -m elf_i386 -nostdlib -static -T kernel.ld -o k406.elf *.o" "$rc"
[ "$rc" = 0 ] || status=1

rm -f .sata.img .vblk.img; truncate -s 1G .sata.img; truncate -s 1G .vblk.img
qboot ahci.out 600 q35 k406.elf "-drive" "file=.sata.img,if=none,id=sd0,format=raw" "-device" \
    "ide-hd,drive=sd0,bus=ide.0,model=DR301 SATA DISK,serial=DR301SATA0001" $VBLK; rc=$?
rec ahci "f406_main.cc (kernel k406.elf)" "$QEMU_VER" \
    "truncate -s 1G sata.img vblk.img; $QEMU -machine q35 $QCOMMON -serial stdio -kernel k406.elf $SATA_SHOWN $VBLK_SHOWN" "$rc" \
    "note:      exit code 33 = pass (isa-debug-exit 0x10); KiB/s figures are emulated time in QEMU TCG, not disk speeds" "$HW_NOTE"
[ "$rc" = 33 ] && grep -q 'model "DR301 SATA DISK" serial "DR301SATA0001"' ahci.out || status=1

python3 -I ../F4-05/verify_image.py .sata.img 0x2F4A0006 4000 > image.out; rc=$?
rec image "../F4-05/verify_image.py" "$(python3 --version)" "python3 -I ../F4-05/verify_image.py sata.img 0x2F4A0006 4000" "$rc"
[ "$rc" = 0 ] || status=1

# QEMU's view of the same initialisation: which registers the driver touched, in order
rm -f .sata.img; truncate -s 64M .sata.img
qboot .t.txt 120 q35 k406.elf -append boottrace "-drive" "file=.sata.img,if=none,id=sd0,format=raw" "-device" \
    "ide-hd,drive=sd0,bus=ide.0,model=DR301 SATA DISK,serial=DR301SATA0001" -trace 'ahci_mem_write_host' \
    -trace 'ahci_port_write' -trace 'ahci_reset_port' -D .atrace.txt; rc=$?
# The firmware (SeaBIOS) drives the controller first; the kernel's part starts at its HBA
# reset, the last write to GHC with bit 0 (HR) set.
sed -i 's/^[0-9]*@[0-9.]*://' .atrace.txt
k=$(grep -n 'reg:GHC\] @ 0x4: 0x[0-9a-f]*[13579bdf]$' .atrace.txt | tail -n 1 | cut -d: -f1)
{ echo "QEMU trace events ahci_mem_write_host, ahci_port_write, ahci_reset_port during the boottrace run"
  echo "(the leading process id and time stamp removed; $(wc -l < .atrace.txt) lines in all)"
  echo "== lines 1-$(( ${k:-1} - 1 )): the firmware (SeaBIOS) before the kernel started; the first 8 =="
  head -n $(( ${k:-1} - 1 )) .atrace.txt | head -n 8
  echo "== from line ${k:-?}: the lab kernel, from its HBA reset to the end =="
  tail -n +"${k:-1}" .atrace.txt; } > ahci_trace.out
[ -n "$k" ] || rc=98
rec ahci_trace "f406_main.cc boottrace (kernel k406.elf)" "$QEMU_VER" \
    "$QEMU -machine q35 $QCOMMON -serial stdio -kernel k406.elf -append boottrace $SATA_SHOWN -trace ahci_mem_write_host -trace ahci_port_write -trace ahci_reset_port -D atrace.txt" "$rc" "$HW_NOTE"
[ "$rc" = 33 ] || status=1

# The forensic evidence: the same boot with the healthy and with the slow port scan.
BT="-drive file=.sata.img,if=none,id=sd0,format=raw -device ide-hd,drive=sd0,bus=ide.0 $VBLK"
qboot .healthy.txt 120 q35 k406.elf -append boottrace $BT; rc1=$?
qboot .slow.txt 120 q35 k406.elf -append "boottrace slowahci" $BT; rc2=$?
python3 -I trace_csv.py .healthy.txt .slow.txt boottrace.csv > .sum.txt; rc3=$?
{ echo "== boottrace.csv =="; cat boottrace.csv; echo "== summary (trace_csv.py) =="; cat .sum.txt
  echo "== kernel log of the slow boot, AHCI lines =="; grep '^ahci' .slow.txt; } > boottrace.out
rec boottrace "f406_main.cc boottrace; trace_csv.py" "$QEMU_VER; $(python3 --version)" \
    "$QEMU -machine q35 $QCOMMON -serial stdio -kernel k406.elf -append boottrace [slowahci] -drive file=sata.img,if=none,id=sd0,format=raw -device ide-hd,drive=sd0,bus=ide.0 $VBLK_SHOWN; python3 -I trace_csv.py healthy.txt slow.txt boottrace.csv" \
    "$(( (rc1 != 33) + (rc2 != 33) + rc3 ))" "note:      exit code 0 = both boots exited with 33 and the CSV was written" "$HW_NOTE"
[ "$rc1" = 33 ] && [ "$rc2" = 33 ] && [ "$rc3" = 0 ] || status=1

rm -f k406.elf .kb.txt .sata.img .vblk.img .t.txt .atrace.txt .healthy.txt .slow.txt .sum.txt
exit $status
