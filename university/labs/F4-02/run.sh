#!/usr/bin/env bash
# F4-02 run.sh: milestone C1 on QEMU's q35 machine with the curriculum's fixed device set.
#   build    : the F4-02 kernel (F4-01 base + acpi.cc + pci.cc + f402_main.cc)
#   enum     : the kernel's PCI list, capability lists and driver binding (q35, ECAM)
#   enum_pc  : the same kernel on the older "pc" machine (no MCFG: legacy ports)
#   qemu_pci : QEMU's own listing ("info pci") of the same q35 configuration
#   compare  : acceptance test 1, the kernel's list against QEMU's query-pci
#   forensic : the kernel built with F402_BAR64_BUG, compared the same way
set -u -o pipefail
cd "$(dirname "$0")"
. ../F4-01/lablib.sh
status=0
SRC="../F4-01/boot.S ../F4-01/kbase.cc ../F4-01/driver.cc acpi.cc pci.cc f402_main.cc"
truncate -s 8M .vblk.img; truncate -s 8M .nvme.img
DEVS="-drive file=.vblk.img,if=none,id=vd0,format=raw -device virtio-blk-pci,drive=vd0 \
-netdev user,id=n0 -device e1000,netdev=n0 \
-drive file=.nvme.img,if=none,id=nv0,format=raw -device nvme,serial=DR301NVME,drive=nv0 \
-device qemu-xhci -audiodev none,id=snd0 -device intel-hda -device hda-output,audiodev=snd0 \
-device pcie-root-port,id=rp1,chassis=1,bus=pcie.0 -device virtio-rng-pci,bus=rp1"
DEVS_SHOWN="-drive file=vblk.img,if=none,id=vd0,format=raw -device virtio-blk-pci,drive=vd0 -netdev user,id=n0 -device e1000,netdev=n0 -drive file=nvme.img,if=none,id=nv0,format=raw -device nvme,serial=DR301NVME,drive=nv0 -device qemu-xhci -audiodev none,id=snd0 -device intel-hda -device hda-output,audiodev=snd0 -device pcie-root-port,id=rp1,chassis=1,bus=pcie.0 -device virtio-rng-pci,bus=rp1"

kbuild k402.elf $SRC > .kb.txt 2>&1; rc=$?
{ cat .kb.txt; size -A k402.elf | grep -E '^(section|\.text|\.rodata|\.data|\.bss|Total)'; } > build.out
rec build "$SRC kernel.ld" "$GXX_VER; $LD_VER" \
    "g++ $KFLAGS -c <each file>; ld -m elf_i386 -nostdlib -static -T kernel.ld -o k402.elf *.o" "$rc"
[ "$rc" = 0 ] || status=1

qboot enum.out 60 q35 k402.elf $DEVS; rc=$?
rec enum "f402_main.cc (kernel k402.elf)" "$QEMU_VER" \
    "$QEMU -machine q35 $QCOMMON -serial stdio -kernel k402.elf $DEVS_SHOWN" "$rc" \
    "note:      exit code 33 = pass (isa-debug-exit 0x10)" "$HW_NOTE"
[ "$rc" = 33 ] || status=1

# worked example: the raw sizing reads of the e1000 and the NVMe controller, decoded on the host
awk '/^PCI 00:0[23]\.0/{n=($2=="00:02.0")?"e1000":"nvme"} /raw BAR/{print n, $2, $4, $6}' enum.out \
    > .raw.txt
python3 - .raw.txt > bar_math.in <<'PY'
import sys
rows = [l.split() for l in open(sys.argv[1])]
out, i = [], 0
while i < len(rows):
    name, bar, orig, probe = rows[i]
    orig_v = int(orig, 16)
    if not (orig_v & 1) and ((orig_v >> 1) & 3) == 2:      # 64-bit: join with the next row
        out.append("%s %s %s %s %s" % (name, orig[2:], probe[2:], rows[i + 1][2][2:], rows[i + 1][3][2:]))
        i += 2
    else:
        out.append("%s %s %s" % (name, orig[2:], probe[2:]))
        i += 1
print("\n".join(out))
PY
hostbuild .bar_math bar_math.cc > .bm.txt 2>&1 && { ./.bar_math < bar_math.in > bar_math.out 2>&1; rc=$?; } || { cat .bm.txt > bar_math.out; rc=99; }
rec bar_math "bar_math.cc (input bar_math.in: the raw reads printed in enum.out)" "$GXX_VER" \
    "g++ $HOSTFLAGS bar_math.cc -o bar_math; ./bar_math < bar_math.in" "$rc"
[ "$rc" = 0 ] || status=1

qboot enum_pc.out 60 pc k402.elf -device virtio-rng-pci; rc=$?
rec enum_pc "f402_main.cc (kernel k402.elf)" "$QEMU_VER" \
    "$QEMU -machine pc $QCOMMON -serial stdio -kernel k402.elf -device virtio-rng-pci" "$rc" "$HW_NOTE"
[ "$rc" = 33 ] || status=1

rm -f .ser.txt
timeout 120 python3 -I qmp_pci.py .ser.txt .query.json qemu_pci.out $QEMU -machine q35 -m 64M -nodefaults \
    -display none -no-reboot -monitor none -serial file:.ser.txt -kernel k402.elf -append hold $DEVS > .qmp.txt 2>&1; rc=$?
cat .qmp.txt >> qemu_pci.out
rec qemu_pci "qmp_pci.py" "$QEMU_VER; $(python3 --version)" \
    "python3 -I qmp_pci.py ser.txt query.json qemu_pci.out $QEMU -machine q35 -m 64M -nodefaults -display none -no-reboot -monitor none -serial file:ser.txt -kernel k402.elf -append hold $DEVS_SHOWN" "$rc" \
    "note:      the kernel ran with 'hold' and stayed alive; QEMU was asked after it printed 'F4-02 done'"
[ "$rc" = 0 ] || status=1

python3 -I compare.py .ser.txt .query.json > compare.out; rc=$?
rec compare "compare.py" "$(python3 --version)" "python3 -I compare.py ser.txt query.json" "$rc"
[ "$rc" = 0 ] || status=1

KEXTRA="-DF402_BAR64_BUG" kbuild k402f.elf $SRC > .kbf.txt 2>&1 || status=1
rm -f .serf.txt
timeout 120 python3 -I qmp_pci.py .serf.txt .queryf.json .infof.txt $QEMU -machine q35 -m 64M -nodefaults \
    -display none -no-reboot -monitor none -serial file:.serf.txt -kernel k402f.elf -append hold $DEVS > /dev/null 2>&1
{ echo "== kernel log (excerpt: functions with BARs) =="; sed 's/\r$//' .serf.txt | grep -E '^PCI|BAR' ;
  echo "== compare.py against QEMU's query-pci =="; python3 -I compare.py .serf.txt .queryf.json; } > forensic.out
rc=$?
rec forensic "f402_main.cc and pci.cc built with -DF402_BAR64_BUG; compare.py" "$QEMU_VER; $(python3 --version)" \
    "(as qemu_pci, kernel k402f.elf); python3 -I compare.py serf.txt queryf.json" "$rc" \
    "note:      exit code 1 is expected: compare.py reports differences"
[ "$rc" = 1 ] || status=1

rm -f .raw.txt .bar_math .bm.txt k402.elf k402f.elf .kb.txt .kbf.txt .vblk.img .nvme.img .ser.txt .serf.txt .query.json .queryf.json .infof.txt .qmp.txt
exit $status
