#!/usr/bin/env bash
# F4-15 run.sh: stage (b), "find documentation".
#   build       the lab kernel with the driver match table (bind4.cc, f415_main.cc)
#   boottrace   boot it on the F4-14 machine: which functions a driver claims, which not
#   docs_local  which documents about these devices exist on this machine, offline
#   forensic    the "unknown device" exhibits: Windows-style IDs for the unbound function,
#               a constructed INF excerpt, and QEMU's own description of the device (qtree)
# tier.cpp (the tier decision procedure, input tier.in) is run by run_lab.sh itself.
set -u -o pipefail
cd "$(dirname "$0")"
. ../F4-14/dr401lib.sh
status=0
B=../F4-14
SRC="$B/boot.S $B/k4.cc $B/pci4.cc bind4.cc f415_main.cc"
DEV="-netdev user,id=n0 -device e1000,netdev=n0 -device edu -device nvme,serial=dr401,drive=nv \
-drive if=none,id=nv,file=/dev/null,format=raw -device qemu-xhci -device intel-hda \
-audiodev none,id=snd0 -device hda-output,audiodev=snd0 -device virtio-rng-pci"

kbuild k415.elf $SRC > build.out 2>&1; rc=$?
[ -s build.out ] || echo "(no messages: no warnings, no errors)" > build.out
rec build "boot.S k4.cc pci4.cc (from F4-14), bind4.cc, f415_main.cc" "$GXX_VER; $LD_VER" \
    "g++ $KFLAGS -c <each file>; ld -m elf_i386 -nostdlib -static -T kernel.ld -o k415.elf *.o" "$rc"
[ "$rc" = 0 ] || status=1

qboot boottrace.out 60 k415.elf $DEV; rc=$?
rec boottrace "f415_main.cc (kernel k415.elf)" "$QEMU_VER" \
    "$QEMU -machine q35 $QCOMMON -serial stdio -kernel k415.elf $DEV" "$rc" \
    "note:      exit code 33 = pass (isa-debug-exit 0x10); tsc+N = time-stamp counter ticks since kmain started" "$HW_NOTE"
[ "$rc" = 33 ] || status=1

{
    echo "== ID databases (the files lspci and similar tools use) =="
    for f in /usr/share/misc/pci.ids /usr/share/misc/usb.ids /usr/share/hwdata/pnp.ids; do
        printf '%s: %s lines; %s\n' "$f" "$(wc -l < "$f")" "$(grep -m1 -E '^#\s*Version' "$f" | sed 's/^#[[:space:]]*//' || true)"
    done
    echo "== Linux UAPI headers (linux-libc-dev $(dpkg-query -W -f='${Version}' linux-libc-dev)): register and protocol names =="
    for h in pci_regs.h serial_reg.h virtio_pci.h virtio_ring.h hid.h i2c.h input-event-codes.h; do
        printf '%-22s %5s lines  e.g. %s\n' "$h" "$(wc -l < /usr/include/linux/$h)" \
            "$(grep -m1 -E '^#define[[:space:]]+(PCI_CLASS_REVISION|UART_LSR|VIRTIO_PCI_CAP_COMMON_CFG|VRING_DESC_F_NEXT|USB_INTERFACE_CLASS_HID|I2C_M_RD|BTN_TOUCH)\b' /usr/include/linux/$h | tr -s '\t ' ' ')"
    done
    echo "== what pci.ids knows about the unbound function 00:02.0 (vendor 1234) =="
    grep -c -E '^1234 ' /usr/share/misc/pci.ids | sed 's/^/lines starting with vendor 1234: /'
    echo "== QEMU's own documentation installed on this machine =="
    ls /usr/share/doc | grep -i -E '^qemu' | tr '\n' ' '; echo
    echo "regular files under /usr/share/doc/qemu-system-x86: $(find /usr/share/doc/qemu-system-x86/ -type f | sed "s#.*/##" | tr "\n" " ")"
} > docs_local.out 2>&1; rc=$?
rec docs_local "run.sh step docs_local" "$(bash --version | head -n 1)" "ls, wc, grep over /usr/share/misc, /usr/share/hwdata, /usr/include/linux, /usr/share/doc" "$rc"
[ "$rc" = 0 ] || status=1

python3 mk_evidence.py hwids boottrace.out 00:02.0 > forensic_hwids.out 2>&1; rc=$?
rec forensic_hwids "mk_evidence.py" "$PY_VER" "python3 mk_evidence.py hwids boottrace.out 00:02.0" "$rc" \
    "note:      formatted from the real IDs in boottrace.out; the Windows ID scheme is this lab's assumption (see the chapter)"
[ "$rc" = 0 ] || status=1
python3 mk_evidence.py inf > forensic_inf.out 2>&1; rc=$?
rec forensic_inf "mk_evidence.py" "$PY_VER" "python3 mk_evidence.py inf" "$rc" \
    "note:      CONSTRUCTED exhibit: written for this lab, not taken from any vendor"
[ "$rc" = 0 ] || status=1
printf '{"execute":"qmp_capabilities"}\n{"execute":"human-monitor-command","arguments":{"command-line":"info qtree"}}\n{"execute":"quit"}\n' |
    timeout 30 $QEMU -machine q35 -m 128M -nodefaults -display none -S -qmp stdio $DEV 2>/dev/null |
    python3 -c 'import sys, json
for l in sys.stdin:
    try: j = json.loads(l)
    except ValueError: continue
    if isinstance(j.get("return"), str): print(j["return"])' > .qtree.txt
python3 mk_evidence.py qtree .qtree.txt 1234:11e8 > forensic_qtree.out 2>&1; rc=$?
rec forensic_qtree "mk_evidence.py" "$PY_VER; $QEMU_VER" \
    "QMP human-monitor-command \"info qtree\" on the paused machine; python3 mk_evidence.py qtree qtree.txt 1234:11e8" "$rc" "$HW_NOTE"
[ "$rc" = 0 ] || status=1

rm -f k415.elf .qtree.txt
exit $status
