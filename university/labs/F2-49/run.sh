#!/usr/bin/env bash
# F2-49 lab: inline assembly in small wrappers.
#   step asm_demo     : the user-mode wrappers of host/cpu.h (sanitizers on)
#   step no_volatile  : an rdtsc wrapper without volatile, at -O0 and -O2
#   step user_cr0     : a privileged instruction in user mode (expected: SIGSEGV)
#   step kernel       : the same kind of instruction in ring 0 (QEMU), through cpu32.h
#   step forensic_O0 / forensic_O2 : the "ready flag" evidence; forensic_disasm: its code
set -u
cd "$(dirname "$0")" || exit 2
LAB="$(pwd)"
status=0
TOOL="$(g++ --version | head -n 1)"
QV="$(qemu-system-x86_64 --version | head -n 1)"
W="${LAB}/.work"; rm -rf "$W"; mkdir -p "$W"
SAN="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
KF="--target=i386-unknown-none-elf -std=c++20 -ffreestanding -fno-exceptions -fno-rtti -fno-pic -O2 -Wall -Wextra -Werror"
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
    sed -i -e "s#${W}/##g" -e "s#${LAB}/##g" -e 's/^bash: line [0-9]*: *[0-9]* /bash: /' "$OUT"
    echo "exit code: $LAST" >> "$LOG"
    [ -n "${1:-}" ] && echo "$1" >> "$LOG"
    if [ "$BAD" != 0 ]; then echo "result:    STEP FAILED ($BAD command(s) did not give the expected exit code)" >> "$LOG"; status=1; fi
    cd "$LAB" || exit 2
}
H="${LAB}/host"; K="${LAB}/kernel"; F="${LAB}/forensic"

begin asm_demo "host/cpu.h host/asm_demo.cpp" "g++ $SAN -O2 asm_demo.cpp -o asm_demo && ./asm_demo"
c "g++ $SAN -O2 $H/asm_demo.cpp -o asm_demo"
c "./asm_demo"
c "objdump -d --no-show-raw-insn asm_demo | grep -E '\\s(cpuid|rdtsc|bsr)' | awk '{print \$2, \$3}' | sort | uniq -c"
finish "hardware:  the CPU of the build container (user mode); the vendor string is that machine's"

begin no_volatile "host/no_volatile.cpp" "g++ -std=c++20 -Wall -Wextra -Werror -O0 / -O2 no_volatile.cpp; count rdtsc instructions with objdump"
for o in O0 O2; do
    c "g++ -std=c++20 -Wall -Wextra -Werror -$o $H/no_volatile.cpp -o nv_$o && ./nv_$o"
    c "echo \"rdtsc instructions in the -$o program: \$(objdump -d nv_$o | grep -cP '\\trdtsc')\""
done
finish

begin user_cr0 "host/user_cr0.cpp" "g++ -std=c++20 -Wall -Wextra -Werror user_cr0.cpp -o user_cr0 && ./user_cr0 (expected to be killed)"
c "g++ -std=c++20 -Wall -Wextra -Werror $H/user_cr0.cpp -o user_cr0"
c "./user_cr0" 139
finish "note:      exit code 139 (128 + 11, SIGSEGV) is expected: user mode may not read CR0"

begin kernel "kernel/cpu32.h kernel/kmain.cpp" "clang++ $KF -c kmain.cpp; ld.lld -m elf_i386 -T link.ld; $QEMU -kernel kernel.elf" "; $(clang++ --version | head -n 1); $QV" "emulated PC (QEMU, TCG); untested on a physical machine"
c "clang++ $KF -c $K/kmain.cpp -o kmain.o"
c "clang --target=i386-unknown-none-elf -c $K/boot.S -o boot.o"
c "ld.lld -m elf_i386 -T $K/link.ld boot.o kmain.o -o kernel.elf"
c "objdump -d --no-show-raw-insn kernel.elf | grep -E '\\s(mov +%cr0|pushf|popf|cli|in |out )' | awk '{\$1=\"\"; print}' | sort | uniq -c"
c "timeout 20 $QEMU -kernel kernel.elf" 33
finish "note:      exit code 33 is expected ((0x10 << 1) | 1 from isa-debug-exit)"

for o in O0 O2; do
    begin forensic_$o "forensic/ready.cpp" "g++ -std=c++20 -Wall -Wextra -Werror -$o ready.cpp -o ready && ./ready"
    c "g++ -std=c++20 -Wall -Wextra -Werror -$o -fcf-protection=none $F/ready.cpp -o ready_$o"
    if [ "$o" = O0 ]; then c "./ready_$o"; else c "./ready_$o" 1; fi
    finish
done
echo "note:      exit code 1 is expected at -O2: that is the bug" >> "${LAB}/forensic_O2.log"

begin forensic_disasm "forensic/ready.cpp" "objdump -d of publish() at -O2"
c "g++ -std=c++20 -O2 -fcf-protection=none $F/ready.cpp -o ready_O2"
c "objdump -d --no-show-raw-insn ready_O2 | sed -n '/<_Z7publishP6Buffer>:/,/^\$/p'"
finish
rm -rf "$W"
exit $status
