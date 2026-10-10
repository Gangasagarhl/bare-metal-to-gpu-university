#!/usr/bin/env bash
# F1-41: (1) disassemble the two polling loops at -O0 and -O2; (2) run the forensic
# "driver" at -O0 and -O2; (3) try port I/O from user mode; (4) ask QEMU for its
# memory-mapped and port-I/O address maps.
set -u -o pipefail
cd "$(dirname "$0")"
status=0
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
GXX="$(g++ --version | head -n 1)"
OD="$(objdump --version | head -n 1)"
F="-std=c++20 -Wall -Wextra -Wpedantic -Werror"
# (1) the polling loops
for o in O0 O2; do
    g++ $F -$o -c poll.cc -o .poll.o || status=1
    objdump -d --no-show-raw-insn -M intel -C .poll.o | sed -n '/^[0-9a-f]* </,$p' > "poll_$o.out"; rc=$?
    rec "poll_$o" poll.cc "$GXX; $OD" "g++ $F -$o -c poll.cc -o poll.o && objdump -d --no-show-raw-insn -M intel -C poll.o" \
        "$rc" "hardware:  compiled and disassembled only (not executed)"
    rm -f .poll.o
done
# (2) the forensic driver, debug and release builds
for o in O0 O2; do
    g++ $F -$o -pthread driver_bug.cc -o .bin_db || status=1
    ./.bin_db > "driver_bug_$o.out" 2>&1; rc=$?
    rec "driver_bug_$o" driver_bug.cc "$GXX" "g++ $F -$o -pthread driver_bug.cc -o driver_bug && ./driver_bug" \
        "$rc" "note:      the helper thread stands in for the hardware; the waited time is one measurement on a shared container (AH-23)"
    rm -f .bin_db
done
# (3) port I/O from user mode
g++ $F -O2 portio.cc -o .bin_pio || status=1
objdump -d --no-show-raw-insn -M intel -C .bin_pio | awk '/^[0-9a-f]+ <main>:/,/^$/' | grep -E '<main>:|\s(in|out)\s' > portio_asm.out
rec portio_asm portio.cc "$GXX; $OD" "objdump -d --no-show-raw-insn -M intel -C portio | (lines of main with in/out only)" 0 \
    "note:      filtered: only the function header and the in/out instructions of main are kept"
( ./.bin_pio ) > portio.out 2>&1; rc=$?
if [ "$rc" -ge 128 ]; then echo "[the program was killed by signal $((rc - 128)) before printing more]" >> portio.out; fi
rec portio portio.cc "$GXX" "g++ $F -O2 portio.cc -o portio && ./portio" "$rc" \
    "note:      expected: the CPU refuses 'in' in user mode; exit code 139 = 128 + signal 11 (SIGSEGV)"
rm -f .bin_pio
# (4) QEMU address maps (q35 PC with one 16550-compatible serial port, firmware only)
QV="$(qemu-system-x86_64 --version | head -n 1)"
python3 -I hmp.py 2 "info mtree" qemu-system-x86_64 -machine q35 -display none -serial null \
    -nographic -no-reboot > .mtree.txt 2>&1; rc=$?
awk '/^memory-region/{s=0} /^address-space: memory$/{s=1;print;next} /^address-space: I\/O$/{s=2;print;next} /^address-space/{s=0} s' .mtree.txt \
    | grep -E '^address-space|ioapic|hpet|apic-msi|  pic$|: pic$|: pit$|serial|i8042|: rtc$|pc.ram$|system$|pci' \
    | awk '!seen[$0]++' > qemu_mtree.out
[ "$rc" = 0 ] || status=1
rec qemu_mtree hmp.py "$QV" \
    "python3 -I hmp.py 2 \"info mtree\" qemu-system-x86_64 -machine q35 -display none -serial null -nographic -no-reboot" \
    "$rc" "note:      filtered: the 'memory' and 'I/O' address spaces only, lines naming the devices of this chapter; duplicates dropped"
rm -f .mtree.txt
exit $status
