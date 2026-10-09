#!/usr/bin/env bash
# F3-14 lab: two boot protocols side by side.
#   1. build the OS301 kernel stub (ELF64, higher half) and inspect it with readelf;
#   2. elfinfo: the loader's ELF check (elf64.hpp) on the real kernel, then on damaged copies;
#   3. a Multiboot (version 1) kernel booted by QEMU's own loader (-kernel), printing what it got;
#   4. forensic evidence: the ELF64 kernel given to QEMU's -kernel option.
# Every step writes <name>.out (real output) and <name>.log (run record).
set -u -o pipefail
cd "$(dirname "$0")"
status=0
B=.build
rm -rf "$B"; mkdir -p "$B"
QEMU=qemu-system-x86_64
QEMUV="$($QEMU --version | head -n 1)"
GXXV="$(g++ --version | head -n 1); $(ld.lld --version | head -n 1)"
EMU="hardware:  untested on hardware; QEMU 8.2.2 q35 machine, not a real PC"
rec() {  # rec <name> <listing> <toolchain> <command> <exit code text> [extra line]
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
KFLAGS="-std=c++20 -ffreestanding -fno-exceptions -fno-rtti -nostdlib -fno-pic -fno-pie -mcmodel=kernel -mno-red-zone -mgeneral-regs-only -fno-stack-protector -O2 -Wall -Wextra -Wpedantic -Werror"
GXX="g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"

# 1. the kernel stub
cmd="g++ $KFLAGS -c kernel.cc -o kernel.o && ld.lld -T kernel.ld -z max-page-size=4096 -nostdlib -static -o kernel.elf kernel.o && readelf -hlW kernel.elf"
{
    g++ $KFLAGS -c kernel.cc -o $B/kernel.o &&
    ld.lld -T kernel.ld -z max-page-size=4096 -nostdlib -static -o $B/kernel.elf $B/kernel.o &&
    (cd $B && readelf -hlW kernel.elf) | grep -E 'Class|Type:|Machine|Entry|program headers|Program Headers|LOAD|Type  |Segment|^ +0[0-9] '
} > kernel_build.out 2>&1; rc=$?
rec kernel_build "kernel.cc bootinfo.hpp kernel.ld" "$GXXV; $(readelf --version | head -n 1)" "$cmd" "$rc"
[ "$rc" = 0 ] || status=1

# 2. the loader's ELF check, compiled for the host with sanitizers
$GXX elfinfo.cc -o $B/elfinfo || status=1
(cd $B && ./elfinfo kernel.elf) > elfinfo.out 2>&1; rc=$?
rec elfinfo "elfinfo.cc elf64.hpp" "$(g++ --version | head -n 1)" "$GXX elfinfo.cc -o elfinfo; ./elfinfo kernel.elf" "$rc"
[ "$rc" = 0 ] || status=1
(cd $B && ./elfinfo --attack kernel.elf) > elfattack.out 2>&1; rc=$?
rec elfattack "elfinfo.cc elf64.hpp" "$(g++ --version | head -n 1)" "./elfinfo --attack kernel.elf" "$rc"
[ "$rc" = 0 ] || status=1

# 3. the Multiboot comparison kernel, loaded by QEMU itself
MFLAGS="-m32 -std=c++20 -ffreestanding -fno-exceptions -fno-rtti -nostdlib -fno-pic -fno-pie -mgeneral-regs-only -fno-stack-protector -O2 -Wall -Wextra -Wpedantic -Werror"
g++ $MFLAGS -c mb_main.cc -o $B/mb_main.o && g++ -m32 -c mb_entry.S -o $B/mb_entry.o &&
    ld.lld -m elf_i386 -T mb.ld -nostdlib -static -o $B/mb.elf $B/mb_entry.o $B/mb_main.o || status=1
qargs="-machine q35 -m 256M -display none -serial stdio -no-reboot -net none"
(cd $B && python3 ../../F3-10/qemu_run.py --timeout 20 -- $QEMU $qargs -kernel mb.elf -append "hello from the command line" \
    -device isa-debug-exit,iobase=0xf4,iosize=0x04) > multiboot.out 2>&1; rc=$?
rec multiboot "mb_entry.S mb_main.cc mb.ld" "$GXXV; $QEMUV; SeaBIOS $(dpkg-query -W -f '${Version}' seabios)" \
    "g++ $MFLAGS -c mb_main.cc; g++ -m32 -c mb_entry.S; ld.lld -m elf_i386 -T mb.ld -nostdlib -static -o mb.elf mb_entry.o mb_main.o; python3 ../F3-10/qemu_run.py --timeout 20 -- $QEMU $qargs -kernel mb.elf -append 'hello from the command line' -device isa-debug-exit,iobase=0xf4,iosize=0x04" \
    "$rc (33 = the kernel wrote 0x10 to isa-debug-exit)" "$EMU"
[ "$rc" = 33 ] || status=1

# 4. forensic evidence: hand the ELF64 higher-half kernel to QEMU's -kernel loader
(cd $B && $QEMU $qargs -kernel kernel.elf) > forensic_qemu.out 2>&1; rc=$?
rec forensic_qemu "kernel.elf" "$QEMUV" "$QEMU $qargs -kernel kernel.elf" "$rc" "$EMU"
(cd $B && readelf -nW kernel.elf; echo "(readelf -n printed the notes above; nothing means no note sections)") > forensic_notes.out 2>&1
rec forensic_notes kernel.elf "$(readelf --version | head -n 1)" "readelf -nW kernel.elf" "0"
(cd $B && readelf -hW mb.elf | grep -E 'Class|Machine|Entry') > forensic_mbclass.out 2>&1
rec forensic_mbclass mb.elf "$(readelf --version | head -n 1)" "readelf -hW mb.elf | grep -E 'Class|Machine|Entry'" "0"

rm -rf "$B"
exit $status
