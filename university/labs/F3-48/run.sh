#!/usr/bin/env bash
# F3-48 run.sh: milestone FS5 in the build container. Build an ISO with Rock Ridge, Joliet
# and El Torito (BIOS + UEFI entries) from a host tree; read it back three ways and compare
# with the host; ask libblkid and file(1); extract the UEFI FAT image for mtools; boot the
# BIOS entry under SeaBIOS in QEMU; and the forensic lab's broken build.
set -u -o pipefail
cd "$(dirname "$0")"
. ../F3-43/lablib.sh
status=0
PY_VER="$(python3 --version)"
QEMU_VER="$(qemu-system-x86_64 --version | head -n 1)"
NASM_VER="$(nasm -v)"
MT_VER="$(mtools --version | head -n 1)"
HW="hardware:  untested on hardware: QEMU 8.2.2 with its bundled SeaBIOS (TCG, no KVM); no real optical drive or USB stick"
ok() { [ "$1" = "$2" ] || { echo "step $3: exit $1, expected $2" >&2; status=1; }; }
rm -rf .w && mkdir -p .w
QEMU="qemu-system-x86_64 -machine pc -m 32M -nodefaults -display none -no-reboot -monitor none -serial stdio -boot d -device isa-debug-exit,iobase=0xf4,iosize=0x04"

# 1. build the writer, the reader, the forensic writer and the boot image
{ hostbuild .w/mkiso mkiso.cc; echo "mkiso.cc: exit $?"
  hostbuild .w/isoread isoread.cc; echo "isoread.cc: exit $?"
  hostbuild .w/mkiso_bug -DFORENSIC_RBA_UNITS mkiso.cc; echo "mkiso.cc -DFORENSIC_RBA_UNITS: exit $?"
  nasm -f bin boot.asm -o .w/boot.bin; echo "nasm boot.asm: exit $?, $(stat -c %s .w/boot.bin) bytes"; } > build.out 2>&1
grep -q 'exit [1-9]' build.out; rc=$((1 - $?))
rec build "mkiso.cc isoread.cc boot.asm" "$GXX_VER; $NASM_VER" "g++ $HOSTFLAGS mkiso.cc|isoread.cc (and -DFORENSIC_RBA_UNITS); nasm -f bin boot.asm -o boot.bin" "$rc"
ok "$rc" 0 build

# 2. the tree, a FAT image for the UEFI entry, and the ISO
python3 -I mktree.py .w/tree
{ mkfs.fat -C -n EFIBOOT -i 0401F348 .w/efi.img 1440 && mmd -i .w/efi.img ::/EFI ::/EFI/BOOT \
  && printf 'placeholder: not an EFI program\n' > .w/note.txt && mcopy -i .w/efi.img .w/note.txt ::/EFI/BOOT/README.TXT
  .w/mkiso .w/os401.iso .w/tree OS401_DEMO .w/boot.bin .w/efi.img; } > mkiso.out 2>&1; rc=$?
sed -i 's#\.w/##g' mkiso.out
rec mkiso "mktree.py mkiso.cc" "$GXX_VER; $PY_VER; $(mkfs.fat --help 2>&1 | grep -m1 "^mkfs.fat [0-9]"); $MT_VER" \
    "python3 -I mktree.py tree; mkfs.fat -C -n EFIBOOT efi.img 1440; mmd; mcopy; ./mkiso os401.iso tree OS401_DEMO boot.bin efi.img" "$rc"
ok "$rc" 0 mkiso

# 3. read back: descriptors, then the Rock Ridge, Joliet and plain ISO 9660 views
{ .w/isoread info .w/os401.iso
  echo "== isoread ls rr";     .w/isoread ls .w/os401.iso rr | tee .w/rr.txt
  echo "== isoread ls joliet"; .w/isoread ls .w/os401.iso joliet | tee .w/jo.txt
  echo "== isoread ls plain";  .w/isoread ls .w/os401.iso plain
  python3 -I hostlist.py .w/tree rr > .w/hrr.txt; python3 -I hostlist.py .w/tree joliet > .w/hjo.txt
  diff <(sort .w/rr.txt) <(sort .w/hrr.txt) && echo "Rock Ridge view = host (names, modes, sizes, CRC-32, link targets)"
  diff <(sort .w/jo.txt) <(sort .w/hjo.txt) && echo "Joliet view = host (names, sizes, CRC-32)"; } > read.out 2>&1; rc=$?
rec read "isoread.cc; hostlist.py" "$GXX_VER; $PY_VER" "./isoread info|ls rr|ls joliet|ls plain os401.iso; python3 -I hostlist.py tree rr|joliet; diff" "$rc"
ok "$rc" 0 read

# 4. what independent code sees: libblkid's iso9660 probe and file(1)
{ blkid -p -o export .w/os401.iso; file .w/os401.iso; } > probe.out 2>&1; rc=$?
sed -i 's#\.w/##g' probe.out
rec probe "run.sh step 4" "$BLKID_VER; $(file --version | head -n 1)" "blkid -p -o export os401.iso; file os401.iso" "$rc"
ok "$rc" 0 probe

# 5. the boot catalog, and the UEFI entry's FAT image read by mtools
{ .w/isoread boot .w/os401.iso; .w/isoread extract .w/os401.iso 1 .w/esp.img
  cmp .w/esp.img .w/efi.img && echo "extracted image = the FAT image given to mkiso"
  MTOOLS_SKIP_CHECK=1 mdir -i .w/esp.img ::/EFI/BOOT; } > boot.out 2>&1; rc=$?
sed -i 's#\.w/##g' boot.out
rec boot "isoread.cc boot / extract" "$GXX_VER; $MT_VER" "./isoread boot os401.iso; ./isoread extract os401.iso 1 esp.img; cmp; mdir -i esp.img ::/EFI/BOOT" "$rc"
ok "$rc" 0 boot

# 6. boot the BIOS entry: SeaBIOS reads the catalog, loads the image, the image prints and exits
timeout 30 $QEMU -cdrom .w/os401.iso -chardev file,id=dbg,path=.w/seabios.log -device isa-debugcon,iobase=0x402,chardev=dbg > qemu_boot.out 2>&1; rc=$?
{ echo "== last lines of the SeaBIOS debug log (port 0x402)"; tail -n 3 .w/seabios.log; } >> qemu_boot.out
rec qemu_boot "boot.asm in os401.iso" "$QEMU_VER" "$QEMU -cdrom os401.iso -chardev file,id=dbg,path=seabios.log -device isa-debugcon,iobase=0x402,chardev=dbg" "$rc" \
    "result:    exit code 33 = (0x10 << 1) | 1, the value boot.asm writes to isa-debug-exit: success" "$HW"
ok "$rc" 33 qemu_boot

# 7. an observation: this SeaBIOS boots even when the validation entry's checksum is wrong
cp .w/os401.iso .w/sum.iso
printf '\x00\x00' | dd of=.w/sum.iso bs=1 seek=$((20 * 2048 + 28)) conv=notrunc status=none
{ .w/isoread boot .w/sum.iso | head -n 1; timeout 30 $QEMU -cdrom .w/sum.iso; echo "qemu exit code $?"; } > checksum_tolerance.out 2>&1
grep -q 'qemu exit code 33' checksum_tolerance.out; rc=$?
rec checksum_tolerance "run.sh step 7" "$QEMU_VER" "dd (zero the checksum word of the validation entry); isoread boot sum.iso; $QEMU -cdrom sum.iso" "$rc" "$HW"
ok "$rc" 0 checksum_tolerance

# 8. forensic evidence: the colleague's build of mkiso
.w/mkiso_bug .w/bad.iso .w/tree OS401_DEMO .w/boot.bin .w/efi.img > /dev/null 2>&1
{ .w/isoread boot .w/bad.iso; echo "== first 16 bytes of sector 73 and of sector 292"
  dd if=.w/bad.iso bs=2048 skip=73 count=1 status=none | od -A d -t x1 | head -n 1
  dd if=.w/bad.iso bs=2048 skip=292 count=1 status=none | od -A d -t x1 | head -n 2; } > forensic_catalog.out 2>&1
rec forensic_catalog "mkiso.cc built for the forensic lab (see the answer key)" "$GXX_VER" "./isoread boot bad.iso; dd | od (sectors 73 and 292)" "$?"
timeout 10 $QEMU -cdrom .w/bad.iso -chardev file,id=dbg,path=.w/seabios_bad.log -device isa-debugcon,iobase=0x402,chardev=dbg > forensic_qemu.out 2>&1; rc=$?
{ echo "(serial output above this line, if any)"; echo "== last lines of the SeaBIOS debug log (port 0x402)"; tail -n 3 .w/seabios_bad.log; } >> forensic_qemu.out
rec forensic_qemu "the forensic lab's bad.iso" "$QEMU_VER" "timeout 10 $QEMU -cdrom bad.iso (debugcon to seabios_bad.log)" "$rc" \
    "result:    exit code 124 (stopped by the 10 s time limit; nothing printed) is the expected outcome" "$HW"
ok "$rc" 124 forensic_qemu

rm -rf .w
exit $status
