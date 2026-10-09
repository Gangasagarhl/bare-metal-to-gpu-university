#!/usr/bin/env bash
# F2-50 lab: volatile and memory-mapped registers.
#   step qemu_pci  : QEMU's own view of the edu device (monitor "info pci")
#   step kernel    : the lab kernel finds edu and drives it through typed volatile wrappers
#   step codegen   : what the compiler emits for plain, volatile and bit-field accesses
#   step forensic_O2 / forensic_O0 : the teammate's "fast" kernel at two optimisation levels
set -u
cd "$(dirname "$0")" || exit 2
LAB="$(pwd)"
status=0
TOOL="$(clang++ --version | head -n 1); $(ld.lld --version | head -n 1)"
QV="$(qemu-system-x86_64 --version | head -n 1)"
W="${LAB}/.work"; rm -rf "$W"; mkdir -p "$W"
KF="--target=i386-unknown-none-elf -std=c++20 -ffreestanding -fno-exceptions -fno-rtti -fno-pic -Wall -Wextra -Werror"
QEMU="qemu-system-x86_64 -display none -serial none -no-reboot -debugcon stdio -device isa-debug-exit,iobase=0xf4,iosize=0x04 -device edu"

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
K="${LAB}/kernel"; F="${LAB}/forensic"; H="${LAB}/host"
HW="emulated device (QEMU edu, TCG); untested on physical hardware"

begin qemu_pci "run.sh (step qemu_pci)" "(sleep 3; echo 'info pci'; echo quit) | qemu-system-x86_64 -display none -serial none -monitor stdio -device edu" "; $QV" "$HW"
c "(sleep 3; echo 'info pci'; sleep 1; echo quit) | timeout 30 qemu-system-x86_64 -display none -serial none -monitor stdio -device edu 2>&1 | sed 's/\\x1b\\[[0-9;]*[A-Za-z]//g' | tr -d '\\r' | grep -v -E '^\\(qemu\\)|^QEMU .* monitor' | sed -n '/Bus  0, device   4/,/^  Bus/p' | grep -v '^  Bus  0, device   5'"
finish

begin kernel "kernel/mmio.h kernel/edu.h kernel/pci.h kernel/kmain.cpp" "clang++ $KF -O2 -c kmain.cpp; ld.lld -m elf_i386 -T link.ld; $QEMU -kernel kernel.elf" "; $QV" "$HW"
c "clang++ $KF -O2 -c $K/kmain.cpp -o kmain.o"
c "clang --target=i386-unknown-none-elf -c $K/boot.S -o boot.o"
c "ld.lld -m elf_i386 -T $K/link.ld boot.o kmain.o -o kernel.elf"
c "timeout 20 $QEMU -kernel kernel.elf" 33
finish "note:      exit code 33 is expected ((0x10 << 1) | 1 from isa-debug-exit)"

begin codegen "host/regs_codegen.cpp" "g++ -std=c++20 -O2 -c regs_codegen.cpp; objdump -d (and the bit-field function for AArch64)" "; $(g++ --version | head -n 1); $(aarch64-linux-gnu-g++ --version | head -n 1)"
c "g++ -std=c++20 -O2 -fcf-protection=none -Wall -Wextra -Werror -c $H/regs_codegen.cpp -o codegen.o"
for f in _Z15setModeBitfieldPV8CtrlBits _Z11setModeMaskPVj _Z15writeTwicePlainPj _Z18writeTwiceVolatilePVj _Z9pollPlainPKj _Z12pollVolatilePVKj; do
    c "objdump -d --no-show-raw-insn codegen.o | sed -n '/<$f>:/,/^\$/p'"
done
c "aarch64-linux-gnu-g++ -std=c++20 -O2 -c $H/regs_codegen.cpp -o codegen_arm64.o"
c "aarch64-linux-gnu-objdump -d --no-show-raw-insn codegen_arm64.o | sed -n '/<_Z15setModeBitfieldPV8CtrlBits>:/,/^\$/p'"
finish

for o in O2 O0; do
    begin forensic_$o "forensic/kmain_fast.cpp" "clang++ $KF -$o -c kmain_fast.cpp; ld.lld; $QEMU -kernel fast_$o.elf" "; $QV" "$HW"
    c "clang++ $KF -$o -I $K -c $F/kmain_fast.cpp -o fast_$o.o"
    c "clang --target=i386-unknown-none-elf -c $K/boot.S -o boot.o"
    c "ld.lld -m elf_i386 -T $K/link.ld boot.o fast_$o.o -o fast_$o.elf"
    c "timeout 20 $QEMU -kernel fast_$o.elf" 33
    if [ "$o" = O2 ]; then
        c "objdump -d --no-show-raw-insn fast_O2.elf | sed -n '/<kmain>:/,/^\$/p' | grep -E -A8 'movl +\\\$0xa,0x8'"
    fi
    finish
done
rm -rf "$W"
exit $status
