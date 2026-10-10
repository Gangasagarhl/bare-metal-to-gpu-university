#!/usr/bin/env bash
# F3-23 run.sh: milestone B6 (consoles, logging, panic screen). Steps:
#   screen        : boot with a framebuffer, screendump through QMP, pixel comparison (fbcheck)
#   smbios        : the banner with QEMU's own SMBIOS strings and with overridden strings
#   panic_lock    : panic while the log lock is held: must print and exit, not hang
#   panic_screen  : the same panic without the exit device, screendump of the panic screen
#   forensic_*    : the forensic kernel whose panic path waits for the lock (hangs)
set -u -o pipefail
cd "$(dirname "$0")"
. ../F3-18/oslab.sh
status=0
SRC="../F3-18/boot.S ../F3-18/cxxrt.cc ../F3-18/serial.cc ../F3-18/kprint.cc ../F3-18/panic.cc \
../F3-19/gdt.cc ../F3-19/isr.S ../F3-19/interrupts.cc ../F3-20/pmm.cc ../F3-21/paging.cc \
fb.cc log.cc smbios.cc b6_main.cc"
B=../F3-18/kbuild.sh
$B k_b6 "" $SRC && $B k_pt "-DB6_PANIC_TEST" $SRC && $B k_hang "-DB6_PANIC_TEST -DPANIC_KEEPS_LOCK" $SRC || exit 1
g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined fbcheck.cc -o .fbcheck || exit 1
QV="-machine pc -m 128M -nodefaults -vga std -display none -no-reboot"
strip_boot() { grep -v -e '^  0000' -e '^firmware memory map' -e '^usable ranges' -e '^gdt:' -e '^idt:' -e '^paging:' "$1"; }
NOTE="note:      boot lines already shown in F3-19 to F3-21 (memory map, gdt, idt, paging) are left out"

# 1. acceptance test 1: the screenshot test
python3 -I qmp_cmds.py .s1.txt "waiting for the screendump" "screendump $(pwd)/.screen.ppm" \
    $QEMU $QV -device isa-debug-exit,iobase=0xf4,iosize=0x04 -kernel k_b6.bin > .q1.txt 2>&1; rc=$?
./.fbcheck .screen.ppm .s1.txt > screen.out 2>&1; crc=$?
{ echo "== serial log =="; strip_boot .s1.txt; } > boot.out
rec boot "b6_main.cc, log.cc, fb.cc, fbconsole.h, font5x7.h, smbios.cc" "$QEMU_VER; $GXX_VER" \
    "python3 -I qmp_cmds.py serial.txt 'waiting for the screendump' 'screendump screen.ppm' $QEMU $QV -kernel k_b6.bin" "$rc" "$NOTE" "$HW_NOTE"
rec screen "fbcheck.cc" "$(g++ --version | head -n 1)" \
    "g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined fbcheck.cc -o fbcheck && ./fbcheck screen.ppm serial.txt" \
    "$crc" "$HW_NOTE"
[ "$rc" = 0 ] && [ "$crc" = 0 ] || status=1

# 2. acceptance test 2: SMBIOS strings, QEMU's own and overridden
qrun .s2.txt 30 k_pt.bin -vga std -smbios "type=1,manufacturer=OS302 Lab,product=Kernel I Test Board"; rc=$?
{
    echo "== default QEMU SMBIOS (from the screen test's serial log) =="
    grep -E 'smbios|Booted on|Firmware' .s1.txt
    echo "== with -smbios \"type=1,manufacturer=OS302 Lab,product=Kernel I Test Board\" =="
    grep -E 'smbios|Booted on|Firmware' .s2.txt
} > smbios.out
rec smbios "smbios.cc" "$QEMU_VER" "$QEMU $QBASE -vga std -smbios \"type=1,manufacturer=OS302 Lab,product=Kernel I Test Board\" -serial stdio -kernel k_pt.bin" \
    "$rc" "note:      the B6_PANIC_TEST kernel prints the banner first, then panics (exit 35 as designed)" "$HW_NOTE"
grep -q 'Booted on: OS302 Lab Kernel I Test Board' smbios.out || status=1

# 3. acceptance test 3: panic while the log lock is held
qrun .s3.txt 30 k_pt.bin -vga std; rc=$?
strip_boot .s3.txt > panic_lock.out
rec panic_lock "b6_main.cc built with -DB6_PANIC_TEST" "$QEMU_VER" "$QEMU $QBASE -vga std -serial stdio -kernel k_pt.bin" "$rc" \
    "note:      exit code 35 is expected (the panic finished and told QEMU 'fail'); a deadlock would end with 124" "$NOTE" "$HW_NOTE"
[ "$rc" = 35 ] || status=1

# 4. the panic screen: same kernel without the exit device, so it halts and can be photographed
python3 -I qmp_cmds.py .s4.txt "PANIC at" "screendump $(pwd)/.panic.ppm" $QEMU $QV -kernel k_pt.bin > .q4.txt 2>&1; rc=$?
./.fbcheck .panic.ppm .s4.txt --panic > panic_screen.out 2>&1; crc=$?
rec panic_screen "fbcheck.cc --panic" "$QEMU_VER" \
    "python3 -I qmp_cmds.py serial.txt 'PANIC at' 'screendump panic.ppm' $QEMU $QV -kernel k_pt.bin; ./fbcheck panic.ppm serial.txt --panic" \
    "$crc" "$HW_NOTE"
[ "$rc" = 0 ] && [ "$crc" = 0 ] || status=1

# 5. forensic evidence: the hang, then the monitor's view of the stuck CPU
qrun .s5.txt 15 k_hang.bin -vga std; rc=$?
strip_boot .s5.txt > forensic_serial.out
rec forensic_serial "b6_main.cc built with -DB6_PANIC_TEST -DPANIC_KEEPS_LOCK" "$QEMU_VER" \
    "$QEMU $QBASE -vga std -serial stdio -kernel k_hang.bin (stopped by a 15 s time limit)" "$rc" \
    "note:      exit code 124 = the run was stopped by the time limit: the kernel never finished its panic" "$NOTE" "$HW_NOTE"
[ "$rc" = 124 ] || status=1
python3 -I qmp_cmds.py .s6.txt "holding the log lock" "sleep 3;info registers;x /16xb \$pc" \
    $QEMU $QV -device isa-debug-exit,iobase=0xf4,iosize=0x04 -kernel k_hang.bin > .q6.txt 2>&1; rc=$?
rip=$(grep -o 'RIP=[0-9a-f]*' .q6.txt | head -n 1 | cut -d= -f2)
{
    grep -v -E '^(ES|CS|SS|DS|FS|GS|LDT|TR|GDT|IDT|DR[0-9]|CCS|FCW|FPR|XMM|YMM|ZMM|MXCSR|EFER|Opmask|PKRU|BND)' .q6.txt \
        | grep -v -E '^ *(FPR|XMM)' | sed '/^$/d'
    echo "== addr2line -f -C -e k_hang.elf 0x$rip =="
    addr2line -f -C -e k_hang.elf "0x$rip"
    echo "== objdump -d --no-show-raw-insn around RIP =="
    objdump -d --no-show-raw-insn -C --start-address=$((0x$rip - 12)) --stop-address=$((0x$rip + 12)) k_hang.elf | grep -E '^ *ffff'
} > forensic_monitor.out
rec forensic_monitor "qmp_cmds.py; addr2line" "$QEMU_VER; $(addr2line --version | head -n 1)" \
    "python3 -I qmp_cmds.py serial.txt 'holding the log lock' 'sleep 3;info registers;x /16xb \$pc' $QEMU $QV -kernel k_hang.bin; addr2line -f -C -e k_hang.elf <RIP>" \
    "$rc" "note:      segment, descriptor-table, debug and floating-point register lines are left out" "$HW_NOTE"
[ "$rc" = 0 ] || status=1

rm -f k_b6.* k_pt.* k_hang.* .fbcheck .s?.txt .q?.txt .screen.ppm .panic.ppm
sed -i "s#$(pwd)/##g" ./*.out
exit $status
