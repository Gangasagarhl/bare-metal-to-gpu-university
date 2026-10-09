#!/usr/bin/env bash
# F1-73 lab: build bare-metal firmware with our own start-up code, report its size,
# show the emulated microcontroller's memory tree, boot it in QEMU and read the UART;
# then the same idea on RISC-V; then the forensic build.
# Every step writes <name>.out (real output) and <name>.log (run record).
set -u -o pipefail
cd "$(dirname "$0")"
status=0
B=.build
rm -rf "$B"; mkdir -p "$B"
CXX=clang++
ARMFLAGS="--target=thumbv7m-none-eabi -mcpu=cortex-m3 -std=c++20 -ffreestanding -fno-exceptions -fno-rtti -nostdlib -Os -g -Wall -Wextra -Wpedantic -Werror"
QEMU_ARM="qemu-system-arm"
CLANGV="$($CXX --version | head -n 1); $(ld.lld --version | head -n 1)"
QEMUV="$($QEMU_ARM --version | head -n 1)"
rec() {  # rec <name> <listing> <toolchain> <command> <exit code> [extra line]
    {
        echo "listing:   $2"
        echo "toolchain: $3"
        echo "command:   $4"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
        echo "exit code: $5"
        if [ $# -ge 6 ]; then echo "$6"; fi
    } > "$1.log"
}
EMU="hardware:  untested on hardware; run in QEMU's emulated board, not on a real microcontroller"

# 1. build: compile both files, link with our linker script, write a map file
cmd="$CXX $ARMFLAGS -c startup.cc main.cc && ld.lld -T mps2_an385.ld --gc-sections -Map firmware.map startup.o main.o -o firmware.elf"
{
    $CXX $ARMFLAGS -c startup.cc -o $B/startup.o &&
    $CXX $ARMFLAGS -c main.cc -o $B/main.o &&
    ld.lld -T mps2_an385.ld --gc-sections -Map $B/firmware.map $B/startup.o $B/main.o -o $B/firmware.elf &&
    echo "build ok: firmware.elf written"
} > build.out 2>&1; rc=$?
rec build "startup.cc main.cc mps2_an385.ld" "$CLANGV" "$cmd" "$rc" "note:      target thumbv7m-none-eabi (Armv7-M, Cortex-M3), freestanding"
[ "$rc" = 0 ] || status=1

# 2. size report (H1 acceptance: image size and RAM use are reported)
llvm-size -B $B/firmware.elf | sed "s#$B/##" > size.out 2>&1; rc=$?
rec size firmware.elf "$(llvm-size --version | sed -n 2p | sed 's/^ *//')" "llvm-size -B firmware.elf" "$rc"
[ "$rc" = 0 ] || status=1

# 3. symbols in address order: where the linker put everything
llvm-nm -n $B/firmware.elf | grep -v ' [aN] ' > symbols.out 2>&1; rc=$?
rec symbols firmware.elf "$(llvm-nm --version | sed -n 2p | sed 's/^ *//')" "llvm-nm -n firmware.elf" "$rc"
[ "$rc" = 0 ] || status=1

# 4. the first 64 bytes of the image: the vector table as raw words
llvm-objdump -s --start-address=0 --stop-address=0x40 -j .text $B/firmware.elf | sed "s#$B/##" > vectors.out 2>&1; rc=$?
rec vectors firmware.elf "$(llvm-objdump --version | sed -n 2p | sed 's/^ *//')" "llvm-objdump -s --start-address=0 --stop-address=0x40 -j .text firmware.elf" "$rc"
[ "$rc" = 0 ] || status=1

# 5. what is inside the emulated microcontroller: QEMU's flat memory tree (system address space)
(echo "info mtree -f"; sleep 1; echo quit) | timeout 10 $QEMU_ARM -M mps2-an385 -nographic -monitor stdio -serial null -S 2>&1 \
    | tr -d '\r' | sed 's/\x1b\[[0-9;]*[A-Za-z]//g' \
    | awk '/Root memory region: system/{p=1; next} p && /^$/{exit} p' > mtree.out; rc=$?
rec mtree "(QEMU monitor command)" "$QEMUV" "qemu-system-arm -M mps2-an385 -nographic -monitor stdio -S, then: info mtree -f" "$rc" \
    "note:      only the system address space of the emulated board is kept"
[ -s mtree.out ] || status=1

# 5b. the names QEMU gives the two emulated boards this course uses
{ $QEMU_ARM -machine help | grep -E '^(mps2-an385|stm32vldiscovery) '; qemu-system-riscv64 -machine help | grep -E '^virt '; } > machines.out 2>&1; rc=$?
rec machines "(QEMU machine list)" "$QEMUV" "qemu-system-arm -machine help | grep -E 'mps2-an385|stm32vldiscovery'; qemu-system-riscv64 -machine help | grep virt" "$rc"

# 6. boot: the UART output of the firmware (firmware never exits; the time limit stops QEMU)
timeout 3 $QEMU_ARM -M mps2-an385 -nographic -monitor none -serial stdio -kernel $B/firmware.elf > boot.out 2>&1; rc=$?
rec boot firmware.elf "$QEMUV" "timeout 3 qemu-system-arm -M mps2-an385 -nographic -monitor none -serial stdio -kernel firmware.elf" \
    "$rc (124 = stopped by the 3 s time limit; the firmware loops forever by design)" "$EMU"
grep -q "hello from bare metal" boot.out || status=1

# 7. RISC-V comparison: same idea, different start-up contract
RVFLAGS="-march=rv64imac -mabi=lp64 -mcmodel=medany -mno-relax -fno-pic -no-pie -std=c++20 -ffreestanding -fno-exceptions -fno-rtti -nostdlib -nostartfiles -static -Os -Wall -Wextra -Werror -Wl,--build-id=none,--no-warn-rwx-segments"
cmd="riscv64-linux-gnu-g++ $RVFLAGS -T rv_virt.ld rv_start.S rv_main.cc -o rv.elf && timeout 5 qemu-system-riscv64 -M virt -bios none -nographic -monitor none -serial stdio -kernel rv.elf"
{
    riscv64-linux-gnu-g++ $RVFLAGS -T rv_virt.ld rv_start.S rv_main.cc -o $B/rv.elf &&
    timeout 5 qemu-system-riscv64 -M virt -bios none -nographic -monitor none -serial stdio -kernel $B/rv.elf
} > rv_boot.out 2>&1; rc=$?
rec rv_boot "rv_start.S rv_main.cc rv_virt.ld" "$(riscv64-linux-gnu-g++ --version | head -n 1); $(qemu-system-riscv64 --version | head -n 1)" \
    "$cmd" "$rc (0 = the firmware stopped QEMU through the test device)" "$EMU"
[ "$rc" = 0 ] || status=1

# 8. forensic evidence: the same firmware built with broken_startup.cc
{
    $CXX $ARMFLAGS -c broken_startup.cc -o $B/bstartup.o &&
    ld.lld -T mps2_an385.ld --gc-sections $B/bstartup.o $B/main.o -o $B/broken.elf &&
    timeout 3 $QEMU_ARM -M mps2-an385 -nographic -monitor none -serial stdio -kernel $B/broken.elf
} > forensic_boot.out 2>&1; rc=$?
rec forensic_boot "broken_startup.cc main.cc mps2_an385.ld" "$CLANGV; $QEMUV" \
    "same build as step 1 with broken_startup.cc, then timeout 3 qemu-system-arm -M mps2-an385 -nographic -monitor none -serial stdio -kernel broken.elf" \
    "$rc (134 = QEMU aborted itself after reporting a CPU lockup)" "$EMU"
llvm-objdump -d --no-show-raw-insn $B/broken.elf | sed -n '/<Reset_Handler>:/,/^$/p' > forensic_disasm.out 2>&1
llvm-objdump -d --no-show-raw-insn $B/firmware.elf | sed -n '/<Reset_Handler>:/,/^$/p' > good_disasm.out 2>&1
rec forensic_disasm broken.elf "$(llvm-objdump --version | sed -n 2p | sed 's/^ *//')" "llvm-objdump -d --no-show-raw-insn broken.elf (Reset_Handler only)" 0
llvm-nm -n $B/broken.elf | grep ' [Tt] ' > forensic_symbols.out 2>&1
rec forensic_symbols broken.elf "$(llvm-nm --version | sed -n 2p | sed 's/^ *//')" "llvm-nm -n broken.elf (code symbols only)" 0
rec good_disasm firmware.elf "$(llvm-objdump --version | sed -n 2p | sed 's/^ *//')" "llvm-objdump -d --no-show-raw-insn firmware.elf (Reset_Handler only)" 0

# replace machine-specific process ids by a marked placeholder (AH-25)
sed -i -E 's/from pid [0-9]+/from pid [pid removed]/; /^\.\/run\.sh: line [0-9]+: +[0-9]+ Aborted/d' boot.out forensic_boot.out rv_boot.out

# keep the map file as evidence (text), delete binaries
sed -e "s#$B/##g" -e '/ \.debug_loclists$/,$d' $B/firmware.map > firmware.map.txt
rm -rf "$B"
exit $status
