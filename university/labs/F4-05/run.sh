#!/usr/bin/env bash
# F4-05 run.sh: milestone C4 in QEMU (q35, modern-only virtio devices).
#   build    : the F4-05 kernel
#   virtio   : random I/O over a 1 GiB disk at queue depths 1 and 32; ARP through virtio-net
#   image    : the host checks the disk image the kernel wrote (verify_image.py)
#   capture  : the packet capture of the network device, read with tshark
#   forensic : the block driver with one flag missing on read requests
set -u -o pipefail
cd "$(dirname "$0")"
. ../F4-01/lablib.sh
status=0
SRC="../F4-01/boot.S ../F4-01/kbase.cc ../F4-01/div64.cc ../F4-02/pci.cc ../F4-03/isr.S ../F4-03/irq.cc \
virtio.cc virtio_blk.cc virtio_net.cc iotest.cc f405_main.cc"
DEVS="-drive file=.disk.img,if=none,id=vd0,format=raw -device virtio-blk-pci,drive=vd0,disable-legacy=on \
-netdev user,id=n0 -device virtio-net-pci,netdev=n0,disable-legacy=on,mac=52:54:00:12:34:56 \
-object filter-dump,id=f0,netdev=n0,file=.cap.pcap"
DEVS_SHOWN="-drive file=disk.img,if=none,id=vd0,format=raw -device virtio-blk-pci,drive=vd0,disable-legacy=on -netdev user,id=n0 -device virtio-net-pci,netdev=n0,disable-legacy=on,mac=52:54:00:12:34:56 -object filter-dump,id=f0,netdev=n0,file=cap.pcap"

kbuild k405.elf $SRC > .kb.txt 2>&1; rc=$?
{ cat .kb.txt; size -A k405.elf | grep -E '^(section|\.text|\.rodata|\.data|\.bss|Total)'; } > build.out
rec build "$SRC kernel.ld" "$GXX_VER; $LD_VER" \
    "g++ $KFLAGS -c <each file>; ld -m elf_i386 -nostdlib -static -T kernel.ld -o k405.elf *.o" "$rc"
[ "$rc" = 0 ] || status=1

rm -f .disk.img .cap.pcap; truncate -s 1G .disk.img
qboot virtio.out 300 q35 k405.elf $DEVS; rc=$?
rec virtio "f405_main.cc (kernel k405.elf)" "$QEMU_VER" \
    "truncate -s 1G disk.img; $QEMU -machine q35 $QCOMMON -serial stdio -kernel k405.elf $DEVS_SHOWN" "$rc" \
    "note:      exit code 33 = pass (isa-debug-exit 0x10); the disk image is a sparse file of 1 GiB" "$HW_NOTE"
[ "$rc" = 33 ] || status=1

python3 -I verify_image.py .disk.img 0x2F4A0001,0x2F4A0032 4000 > image.out; rc=$?
rec image "verify_image.py" "$(python3 --version)" "python3 -I verify_image.py disk.img 0x2F4A0001,0x2F4A0032 4000" "$rc"
[ "$rc" = 0 ] || status=1

tshark -r .cap.pcap > capture.out 2>/dev/null; rc=$?
tshark -r .cap.pcap -V -Y arp 2>/dev/null | grep -E 'Opcode|Sender (MAC|IP)|Target (MAC|IP)' >> capture.out
rec capture "(none: tshark reads the capture QEMU's filter-dump wrote)" "$(tshark --version 2>/dev/null | head -n 1)" \
    "tshark -r cap.pcap; tshark -r cap.pcap -V -Y arp | grep -E 'Opcode|Sender|Target'" "$rc"
[ "$rc" = 0 ] && grep -q 'is at' capture.out || status=1

# the layouts of virtio.h against Linux's UAPI headers (a C program prints Linux's values)
gcc -std=c11 -Wall -Wextra -Werror linux_values.c -o .linux_values && ./.linux_values > .lx.txt \
  && hostbuild .vcheck virtio_check.cc && { ./.vcheck < .lx.txt > virtio_check.out; rc=$?; } || rc=99
rec virtio_check "virtio_check.cc, linux_values.c" "$GXX_VER; $(gcc --version | head -n 1); linux-libc-dev $(dpkg-query -W -f='${Version}' linux-libc-dev 2>/dev/null)" \
    "gcc -std=c11 -Wall -Wextra -Werror linux_values.c -o linux_values; g++ $HOSTFLAGS virtio_check.cc -o virtio_check; ./linux_values | ./virtio_check" "$rc"
[ "$rc" = 0 ] || status=1

KEXTRA="-DF405_READ_WITHOUT_WRITE_FLAG" kbuild k405f.elf $SRC > .kbf.txt 2>&1 || status=1
rm -f .disk.img .cap.pcap; truncate -s 1G .disk.img
qboot forensic.out 300 q35 k405f.elf $DEVS -trace virtio_blk_rw_complete -trace virtio_blk_req_complete -D .trace.txt; rc=$?
{ echo "== QEMU trace (virtio_blk_req_complete), first 4 events of $(grep -c virtio_blk_req_complete .trace.txt) =="
  grep virtio_blk_req_complete .trace.txt | head -n 4 | sed 's/^[0-9]*@[0-9.]*://'; } >> forensic.out
rec forensic "f405_main.cc, virtio_blk.cc built with -DF405_READ_WITHOUT_WRITE_FLAG" "$QEMU_VER" \
    "$QEMU -machine q35 $QCOMMON -serial stdio -kernel k405f.elf $DEVS_SHOWN -trace virtio_blk_rw_complete -trace virtio_blk_req_complete -D trace.txt" "$rc" \
    "note:      exit code 3 is expected here: the kernel reports the failure through isa-debug-exit 0x01" "$HW_NOTE"
[ "$rc" = 3 ] || status=1

rm -f .linux_values .lx.txt .vcheck k405.elf k405f.elf .kb.txt .kbf.txt .disk.img .cap.pcap .trace.txt
exit $status
