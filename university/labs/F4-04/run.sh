#!/usr/bin/env bash
# F4-04 run.sh: milestone C3 in QEMU.
#   build    : the F4-04 kernel (F4-01 base, F4-02 acpi.cc, F4-03 interrupts, rtc.cc)
#   rtc_host : acceptance test 1 - RTC based on the host clock; kernel time vs host time
#   rtc_fixed: acceptance test 2 - a fixed start date just before a new year
#   forensic : the BCD bug, booted on two different dates
set -u -o pipefail
cd "$(dirname "$0")"
. ../F4-01/lablib.sh
status=0
SRC="../F4-01/boot.S ../F4-01/kbase.cc ../F4-01/div64.cc ../F4-02/acpi.cc ../F4-03/isr.S ../F4-03/irq.cc rtc.cc f404_main.cc"
KEXTRA="-I../F4-03"

kbuild k404.elf $SRC > .kb.txt 2>&1; rc=$?
{ cat .kb.txt; size -A k404.elf | grep -E '^(section|\.text|\.rodata|\.data|\.bss|Total)'; } > build.out
rec build "$SRC kernel.ld" "$GXX_VER; $LD_VER" \
    "g++ $KFLAGS -c <each file>; ld -m elf_i386 -nostdlib -static -T kernel.ld -o k404.elf *.o" "$rc"
[ "$rc" = 0 ] || status=1

# acceptance test 1: QEMU's RTC follows the host clock (UTC); compare with the host's time
# at the moment the kernel's reading arrives (rtc_stamp.py)
python3 -I rtc_stamp.py 60 $QEMU -machine pc $QCOMMON -serial stdio -kernel k404.elf -rtc base=utc > rtc_host.out 2>&1; rc=$?
rec rtc_host "f404_main.cc (kernel k404.elf); rtc_stamp.py" "$QEMU_VER; $(python3 --version)" \
    "python3 -I rtc_stamp.py 60 $QEMU -machine pc $QCOMMON -serial stdio -kernel k404.elf -rtc base=utc" "$rc" \
    "note:      exit code 33 = QEMU passed (isa-debug-exit 0x10) and the difference was within 2 s" "$HW_NOTE"
[ "$rc" = 33 ] && grep -q 'within 2 seconds: PASS' rtc_host.out || status=1

# acceptance test 2: a fixed start date two seconds before a new year
qboot rtc_fixed.out 60 pc k404.elf -rtc base=2026-12-31T23:59:58; rc=$?
rec rtc_fixed "f404_main.cc (kernel k404.elf)" "$QEMU_VER" \
    "$QEMU -machine pc $QCOMMON -serial stdio -kernel k404.elf -rtc base=2026-12-31T23:59:58" "$rc" "$HW_NOTE"
[ "$rc" = 33 ] && grep -q '^date: 2026-12-31 23:59:5' rtc_fixed.out && grep -q '2027-01-01' rtc_fixed.out || status=1

# forensic: the kernel that ignores the binary/BCD bit, booted on two dates
KEXTRA="-I../F4-03 -DF404_IGNORE_BINARY_BIT" kbuild k404f.elf $SRC > .kbf.txt 2>&1 || status=1
{ for d in 2026-03-05T04:05:06 2026-10-09T20:45:30; do
      echo "== boot with -rtc base=$d =="
      qboot .f.txt 60 pc k404f.elf -rtc base=$d; echo "(QEMU exit code $?)"
      grep -E '^(raw|date)' .f.txt | head -n 2
  done; } > forensic.out
rec forensic "f404_main.cc, rtc.cc built with -DF404_IGNORE_BINARY_BIT" "$QEMU_VER" \
    "$QEMU -machine pc $QCOMMON -serial stdio -kernel k404f.elf -rtc base=<date>, for two dates" "0" "$HW_NOTE"

rm -f k404.elf k404f.elf .kb.txt .kbf.txt .host.txt .f.txt
exit $status
