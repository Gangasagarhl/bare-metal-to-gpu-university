#!/usr/bin/env bash
# F4-09 run.sh: milestone C8 in QEMU (q35, qemu-xhci with a USB keyboard and a USB stick).
#   usbcheck  : host program, constants checked against <linux/usb/ch9.h>, <scsi/scsi.h> (run_lab.sh)
#   build     : the F4-09 kernel (default) and the forensic variant
#   usb       : enumeration, keyboard line (typed through QMP), FAT32 file, write test, hot-plug
#   image     : the host reads back the block the kernel wrote
#   capture   : the keyboard's enumeration, from QEMU's USB packet capture, decoded by tshark
#   usb_iommu : the same run with intel-iommu enabled (every xHCI buffer mapped by F4-08's DMA API)
#   forensic  : the stick driver built with -DF409_LBA_LE; forensic_capture = tshark on its capture
set -u -o pipefail
cd "$(dirname "$0")"
. ../F4-01/lablib.sh
status=0
SRC="../F4-01/boot.S ../F4-01/kbase.cc ../F4-01/div64.cc ../F4-02/pci.cc ../F4-02/acpi.cc \
../F4-08/isr256.S ../F4-08/intr.cc ../F4-08/msi.cc ../F4-08/vtd.cc ../F4-08/dma.cc ../F4-08/xhci_core.cc \
usbh.cc hid_kbd.cc bot.cc f409_main.cc"
TEXT="hello usb 42"
TSHARK_VER="$(tshark --version 2>/dev/null | head -n 1)"

mkimage() {
    rm -f .stick.img; truncate -s 64M .stick.img
    mkfs.fat -F 32 -n DR302STICK .stick.img > /dev/null
    printf 'Hello from the host, read through USB mass storage.\n' > .hello.txt
    mcopy -i .stick.img .hello.txt ::HELLO.TXT
}
devs() {  # $1 = capture prefix
    echo "-device qemu-xhci,id=xhci -device usb-kbd,id=kbd0,pcap=.$1kbd.pcap \
-drive if=none,id=stick,file=.stick.img,format=raw -device usb-storage,drive=stick,id=stick0,pcap=.$1stick.pcap \
-qmp unix:.qmp.sock,server=on,wait=off"
}
SHOWN="-device qemu-xhci,id=xhci -device usb-kbd,id=kbd0,pcap=kbd.pcap -drive if=none,id=stick,file=stick.img,format=raw -device usb-storage,drive=stick,id=stick0,pcap=stick.pcap -qmp unix:qmp.sock,server=on,wait=off"
boot() {  # $1 out file, $2 kernel, $3 capture prefix, $4 extra QEMU options
    rm -f .qmp.sock "$1"
    python3 qmp_usb.py .qmp.sock "$1" "$TEXT" > ".$1.qmp" 2>&1 &
    local py=$!
    qboot "$1" 90 q35 "$2" $4 $(devs "$3"); local rc=$?
    wait $py
    { echo "== QMP driver (host side) =="; cat ".$1.qmp"; } >> "$1"
    rm -f ".$1.qmp" .qmp.sock
    return $rc
}

kbuild k409.elf $SRC > .kb.txt 2>&1; rc=$?
KEXTRA="-DF409_LBA_LE" kbuild k409f.elf $SRC >> .kb.txt 2>&1 || rc=1
{ cat .kb.txt; size -A k409.elf | grep -E '^(section|\.text|\.rodata|\.data|\.bss|Total)'; } > build.out
rec build "$SRC kernel.ld" "$GXX_VER; $LD_VER" \
    "g++ $KFLAGS -c <each file>; ld -m elf_i386 -nostdlib -static -T kernel.ld -o k409.elf *.o (and again with -DF409_LBA_LE)" "$rc"
[ "$rc" = 0 ] || status=1

mkimage
boot usb.out k409.elf "" ""; rc=$?
rec usb "f409_main.cc (kernel k409.elf); keys typed by qmp_usb.py" "$QEMU_VER" \
    "mkfs.fat -F 32 -n DR302STICK stick.img (64 MiB); mcopy HELLO.TXT; $QEMU -machine q35 $QCOMMON -serial stdio -kernel k409.elf $SHOWN" "$rc" \
    "note:      exit code 33 = pass (isa-debug-exit 0x10); the last lines come from the host-side QMP script" "$HW_NOTE"
[ "$rc" = 33 ] || status=1

python3 - > image.out <<'PY'
d = open(".stick.img", "rb").read()
last = d[-512:]
h = 2166136261
for b in last:
    h = ((h ^ b) * 16777619) & 0xFFFFFFFF
print("image: %d bytes, last block starts %r" % (len(d), last[:28].decode()))
print("image: last block fnv1a 0x%08x" % h)
PY
rec image "run.sh (inline Python reading stick.img)" "$(python3 --version)" "python3 (reads the last 512 bytes of stick.img)" "$?"

{ printf 'frame\tURB\ttype\tendpoint\tbmRequestType\tbRequest\tdescriptor\twIndex\twLength\tURB length\tstatus\n'
  tshark -r .kbd.pcap -T fields -e frame.number -e usb.urb_type -e usb.transfer_type -e usb.endpoint_address \
    -e usb.bmRequestType -e usb.setup.bRequest -e usb.bDescriptorType -e usb.setup.wIndex -e usb.setup.wLength \
    -e usb.urb_len -e usb.urb_status 2>/dev/null | head -n 44
  printf '\n== frame 18 in detail (device descriptor) ==\n'
  tshark -r .kbd.pcap -V -Y 'frame.number == 18' 2>/dev/null | sed -n '/DEVICE DESCRIPTOR/,$p' | sed 's/^ *//'
} > capture.out; rc=$?
rec capture "tshark on kbd.pcap (QEMU usb-kbd pcap property)" "$TSHARK_VER" \
    "tshark -r kbd.pcap -T fields -e frame.number -e usb.urb_type -e usb.transfer_type -e usb.endpoint_address -e usb.bmRequestType -e usb.setup.bRequest -e usb.bDescriptorType -e usb.setup.wIndex -e usb.setup.wLength -e usb.urb_len -e usb.urb_status | head -n 44; tshark -r kbd.pcap -V -Y 'frame.number == 18'" "$rc"

mkimage
boot usb_iommu.out k409.elf "i" "-device intel-iommu"; rc=$?
rec usb_iommu "f409_main.cc (kernel k409.elf) with intel-iommu" "$QEMU_VER" \
    "$QEMU -machine q35 $QCOMMON -serial stdio -kernel k409.elf -device intel-iommu $SHOWN" "$rc" \
    "note:      exit code 33 = pass" "$HW_NOTE"
[ "$rc" = 33 ] || status=1

mkimage
boot forensic.out k409f.elf "f" ""; rc=$?
rec forensic "f409_main.cc and bot.cc built with -DF409_LBA_LE (kernel k409f.elf)" "$QEMU_VER" \
    "$QEMU -machine q35 $QCOMMON -serial stdio -kernel k409f.elf $SHOWN" "$rc" \
    "note:      exit code 3 = the kernel reported FAILED (isa-debug-exit 0x01): expected for this forensic build" "$HW_NOTE"
[ "$rc" = 3 ] || status=1
{ printf 'frame\tCBW tag\tSCSI opcode\tREAD/WRITE(10) LBA\ttransfer length\n'
  tshark -r .fstick.pcap -Y 'usbms.dCBWSignature' -T fields -e frame.number -e usbms.dCBWTag -e scsi_sbc.opcode \
    -e scsi_sbc.rdwr10.lba -e scsi_sbc.rdwr10.xferlen 2>/dev/null | head -n 40; } > forensic_capture.out; rc=$?
rec forensic_capture "tshark on stick.pcap of the forensic run" "$TSHARK_VER" \
    "tshark -r stick.pcap -Y usbms.dCBWSignature -T fields -e frame.number -e usbms.dCBWTag -e scsi_sbc.opcode -e scsi_sbc.rdwr10.lba -e scsi_sbc.rdwr10.xferlen" "$rc"

rm -f k409.elf k409f.elf .kb.txt .stick.img .hello.txt .kbd.pcap .stick.pcap .ikbd.pcap .istick.pcap .fkbd.pcap .fstick.pcap
exit $status
