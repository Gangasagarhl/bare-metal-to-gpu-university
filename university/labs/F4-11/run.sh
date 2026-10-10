#!/usr/bin/env bash
# F4-11 run.sh: milestone C10 in QEMU (q35).
#   amlcheck : host program, aml.cc checked on hand-made AML byte strings (run_lab.sh)
#   build    : the F4-11 kernel (default) and the forensic variant
#   acpi     : FADT, ACPI mode, PM timer, DSDT walk, \_S5, _PRT routing of the e1000,
#              power button through QMP, shutdown. Pass = QEMU exits by itself with code 0
#   dsdt     : the DSDT the kernel dumped, walked on the host by aml_walk (aml.cc again)
#   forensic : the same with -DF411_BYTE_WRITE: the machine does not switch off
set -u -o pipefail
cd "$(dirname "$0")"
. ../F4-01/lablib.sh
status=0
SRC="../F4-01/boot.S ../F4-01/kbase.cc ../F4-01/div64.cc ../F4-02/pci.cc ../F4-02/acpi.cc \
../F4-08/isr256.S ../F4-08/intr.cc aml.cc pm.cc f411_main.cc"
DEVS="-device e1000,netdev=n0 -netdev user,id=n0"
SHOWN="$DEVS -qmp unix:qmp.sock,server=on,wait=off"

powerboot() {  # $1 out, $2 kernel
    rm -f .qmp.sock "$1"
    python3 qmp_power.py .qmp.sock "$1" > ".$1.qmp" 2>&1 &
    local qp=$!
    qboot "$1" 60 q35 "$2" $DEVS -qmp unix:.qmp.sock,server=on,wait=off
    local rc=$?
    wait $qp
    { echo "== QMP (host side) =="; cat ".$1.qmp"; } >> "$1"
    rm -f ".$1.qmp" .qmp.sock
    return $rc
}

kbuild k411.elf $SRC > .kb.txt 2>&1; rc=$?
KEXTRA="-DF411_BYTE_WRITE" kbuild k411f.elf $SRC >> .kb.txt 2>&1 || rc=1
{ cat .kb.txt; size -A k411.elf | grep -E '^(section|\.text|\.rodata|\.data|\.bss|Total)'; } > build.out
rec build "$SRC kernel.ld" "$GXX_VER; $LD_VER" \
    "g++ $KFLAGS -c <each file>; ld -m elf_i386 -nostdlib -static -T kernel.ld -o k411.elf *.o (and again with -DF411_BYTE_WRITE)" "$rc"
[ "$rc" = 0 ] || status=1

powerboot acpi.out k411.elf; rc=$?
rec acpi "f411_main.cc (kernel k411.elf); power button pressed by qmp_power.py" "$QEMU_VER" \
    "$QEMU -machine q35 $QCOMMON -serial stdio -kernel k411.elf $SHOWN; python3 qmp_power.py qmp.sock acpi.out" "$rc" \
    "note:      exit code 0 = pass: the guest switched the machine off (S5) and QEMU ended by itself" "$HW_NOTE"
[ "$rc" = 0 ] || status=1
grep -q 'acpi: shutting down' acpi.out || status=1

# The DSDT itself: a third build (-DF411_DUMP) prints it in hex and stops; the host
# walker then runs on exactly the bytes the kernel saw.
hostbuild .aml_walk aml_walk.cc aml.cc > .hb.txt 2>&1 || { cat .hb.txt; status=1; }
KEXTRA="-DF411_DUMP" kbuild .k411d.elf $SRC >> .kb.txt 2>&1
qboot .dump.txt 30 q35 .k411d.elf $DEVS
python3 -c "
import sys
h=''.join(l.split(':',1)[1].strip() for l in open('.dump.txt') if l.startswith('dsdtdump:'))
open('.dsdt.aml','wb').write(bytes.fromhex(h))"
{ ./.aml_walk .dsdt.aml; echo "== the namespace tree =="; ./.aml_walk .dsdt.aml --tree | sed -n '2,$p'; } > dsdt.out 2>&1; rc=$?
rec dsdt "aml_walk.cc + aml.cc (host), DSDT dumped by the F4-11 kernel built with -DF411_DUMP" "$GXX_VER; $QEMU_VER" \
    "g++ $HOSTFLAGS aml_walk.cc aml.cc -o aml_walk; ./aml_walk dsdt.aml; ./aml_walk dsdt.aml --tree" "$rc"
[ "$rc" = 0 ] || status=1

powerboot forensic.out k411f.elf; rc=$?
rec forensic "f411_main.cc built with -DF411_BYTE_WRITE (kernel k411f.elf)" "$QEMU_VER" \
    "$QEMU -machine q35 $QCOMMON -serial stdio -kernel k411f.elf $SHOWN; python3 qmp_power.py qmp.sock forensic.out" "$rc" \
    "note:      exit code 3 = the kernel reported FAILED (still running after the S5 write): expected for this forensic build" "$HW_NOTE"
[ "$rc" = 3 ] || status=1

rm -f k411.elf k411f.elf .k411d.elf .kb.txt .hb.txt .aml_walk .dump.txt .dsdt.aml
exit $status
