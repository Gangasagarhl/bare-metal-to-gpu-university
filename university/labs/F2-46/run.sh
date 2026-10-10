#!/usr/bin/env bash
# F2-46 lab: linker scripts. A tiny 32-bit x86 kernel is linked with link.ld, inspected with
# readelf, nm and the linker's map file, and booted in QEMU (Multiboot loader, -kernel).
#   step build   : compile, link with -Map, readelf, nm, map excerpt
#   step boot    : QEMU runs the kernel; it prints the addresses link.ld exported
#   step higher  : the same objects linked with higher.ld (VMA != LMA); read, not booted
#   step forensic_new   : the teammate's link_new.ld plus a 16 KiB table: QEMU refuses it
#   step forensic_fixed : the same table with the original link.ld: boots
set -u
cd "$(dirname "$0")" || exit 2
LAB="$(pwd)"
status=0
TOOL="$(clang++ --version | head -n 1); $(ld.lld --version | head -n 1)"
QV="$(qemu-system-x86_64 --version | head -n 1)"
W="${LAB}/.work"; rm -rf "$W"; mkdir -p "$W"
CXXF="--target=i386-unknown-none-elf -std=c++20 -ffreestanding -fno-exceptions -fno-rtti -fno-pic -O2 -Wall -Wextra -Werror"
QEMU="qemu-system-x86_64 -display none -serial none -no-reboot -debugcon stdio -device isa-debug-exit,iobase=0xf4,iosize=0x04"

begin() {   # begin <name> <listing> <command summary> [extra toolchain] [hardware]
    NAME="$1"; LAST=0; BAD=0
    OUT="${LAB}/${NAME}.out"; LOG="${LAB}/${NAME}.log"; : > "$OUT"
    {
        echo "listing:   $2"
        echo "toolchain: $TOOL${4:-}"
        echo "command:   $3"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
        if [ -n "${5:-}" ]; then echo "hardware:  $5"; fi
    } > "$LOG"
    cd "$W" || exit 2
}
c() {
    echo "\$ $1" >> "$OUT"
    bash -c "$1" >> "$OUT" 2>&1
    LAST=$?
    if [ "$LAST" != "${2:-0}" ]; then BAD=$((BAD + 1)); fi
    if [ "$LAST" != 0 ]; then echo "(exit code: $LAST)" >> "$OUT"; fi
}
finish() {
    sed -i -e "s#${W}/##g" -e "s#${LAB}/##g" "$OUT"
    echo "exit code: $LAST" >> "$LOG"
    [ -n "${1:-}" ] && echo "$1" >> "$LOG"
    if [ "$BAD" != 0 ]; then echo "result:    STEP FAILED ($BAD command(s) did not give the expected exit code)" >> "$LOG"; status=1; fi
    cd "$LAB" || exit 2
}

K="${LAB}/kernel"
begin build "kernel/boot.S kernel/console.h kernel/kmain.cpp kernel/link.ld" "clang++ $CXXF -c kmain.cpp; clang --target=i386-unknown-none-elf -c boot.S; ld.lld -m elf_i386 -T link.ld -Map=kernel.map boot.o kmain.o -o kernel.elf"
c "clang++ $CXXF -c $K/kmain.cpp -o kmain.o"
c "clang --target=i386-unknown-none-elf -c $K/boot.S -o boot.o"
c "ld.lld -m elf_i386 -T $K/link.ld -Map=kernel.map boot.o kmain.o -o kernel.elf"
c "readelf -lW kernel.elf"
c "readelf -SW kernel.elf | sed -n '/^Section Headers/,/^Key to Flags/p' | grep -v '^Key to Flags'"
c "nm -n kernel.elf"
c "cat kernel.map"
finish

begin boot "kernel/kmain.cpp kernel/link.ld" "$QEMU -kernel kernel.elf" "; $QV" "emulated PC (QEMU, TCG); untested on a physical machine"
c "timeout 20 $QEMU -kernel kernel.elf" 33
finish "note:      exit code 33 is expected: kmain wrote 0x10 to the isa-debug-exit port and QEMU exits with (0x10 << 1) | 1"

begin higher "kernel/higher.ld" "ld.lld -m elf_i386 -T higher.ld boot.o kmain.o -o higher.elf; readelf -lW; nm"
c "ld.lld -m elf_i386 -T $K/higher.ld boot.o kmain.o -o higher.elf"
c "readelf -hW higher.elf | grep -E 'Entry point'"
c "readelf -lW higher.elf | sed -n '/Program Headers/,/Section to Segment/p' | grep -v 'Section to Segment'"
c "nm -n higher.elf | grep -E ' (_start|kmain|__kernel_start|__bss_end|g_initialised)\$'"
finish

begin forensic_new "forensic/tables.cpp forensic/link_new.ld kernel/kmain.cpp kernel/boot.S" "ld.lld -m elf_i386 -T link_new.ld -Map=new.map boot.o kmain.o tables.o -o new.elf; $QEMU -kernel new.elf" "; $QV" "emulated PC (QEMU, TCG)"
c "clang++ $CXXF -c ${LAB}/forensic/tables.cpp -o tables.o"
c "ld.lld -m elf_i386 -T ${LAB}/forensic/link_new.ld -Map=new.map boot.o kmain.o tables.o -o new.elf"
c "timeout 20 $QEMU -kernel new.elf" 1
c "ls -l new.elf | awk '{print \$5, \$9}'"
c "readelf -lW new.elf | sed -n '/Program Headers/,/Section to Segment/p' | grep -v 'Section to Segment'"
c "grep -E '^ +[0-9a-f]+ +[0-9a-f]+ +[0-9a-f]+ +[0-9]+ (\\.[a-z]+|.*\\.o:\\(\\.(multiboot|rodata))' new.map"
c "echo \"multiboot magic found at file offset: \$(grep -obUaP '\\x02\\xb0\\xad\\x1b' new.elf | head -n 1 | cut -d: -f1)\""
finish "note:      exit code 1 of QEMU is expected here: it refuses to load the kernel"

begin forensic_fixed "forensic/tables.cpp kernel/link.ld" "ld.lld -m elf_i386 -T link.ld boot.o kmain.o tables.o -o fixed.elf; $QEMU -kernel fixed.elf" "; $QV" "emulated PC (QEMU, TCG)"
c "ld.lld -m elf_i386 -T $K/link.ld -Map=fixed.map boot.o kmain.o tables.o -o fixed.elf"
c "echo \"multiboot magic found at file offset: \$(grep -obUaP '\\x02\\xb0\\xad\\x1b' fixed.elf | head -n 1 | cut -d: -f1)\""
c "timeout 20 $QEMU -kernel fixed.elf | head -n 1" 0
finish
rm -rf "$W"
exit $status
