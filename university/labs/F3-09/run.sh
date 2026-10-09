#!/usr/bin/env bash
# F3-09 lab: milestone P3 ("debug a machine you did not write") on QEMU + OVMF:
#   1. disassemble the reset-vector bytes of the firmware file (objdump, 16-bit mode);
#   2. start QEMU halted at power-on, attach GDB, read the registers and step 8 instructions;
#   3. let the firmware run, stop it, find CR3 and walk one translation by hand (walk.py);
#   4. forensic evidence: the same firmware with its last 16 bytes erased.
# Every step writes <name>.out (real output) and <name>.log (run record).
set -u -o pipefail
cd "$(dirname "$0")"
status=0
B=.build
rm -rf "$B"; mkdir -p "$B"
QEMU=qemu-system-x86_64
QEMUV="$($QEMU --version | head -n 1)"
GDBV="$(gdb --version | head -n 1)"
OBJV="$(objdump --version | head -n 1)"
OVMFV="OVMF from the ovmf package $(dpkg-query -W -f '${Version}' ovmf)"
CODE=/usr/share/OVMF/OVMF_CODE_4M.fd
EMU="hardware:  untested on hardware; QEMU 8.2.2 q35 machine with the distribution's OVMF, not a real PC"
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
# keep only the general registers of QEMU's "info registers" (FPU/SSE/debug lines are trimmed)
trim_regs() { grep -v -E '^(FPR|XMM|DR[0-9]|FCW)' | grep -v '^$' | sed 's/^\(EFER=.*\)$/\1\n[...] (FPU, SSE and debug registers trimmed)/'; }

# 1. the reset vector bytes, disassembled as 16-bit code at the address where the CPU sees them
size=$(stat -c %s $CODE)
tail -c 16 $CODE > $B/vector.bin
dd if=$CODE of=$B/ff10.bin bs=1 skip=$((size - 0xf0)) count=16 status=none
{
    echo "== 16 bytes at 0xfffffff0 (the last 16 bytes of the file) =="
    objdump -D -b binary -m i8086 --adjust-vma=0xfffffff0 $B/vector.bin | sed -n '/^fffffff0/,$p'
    echo "== 16 bytes at 0xffffff10 (the target of the jump at 0xfffffffc) =="
    objdump -D -b binary -m i8086 --adjust-vma=0xffffff10 $B/ff10.bin | sed -n '/^ffffff10/,$p'
} > vector.out 2>&1; rc=$?
rec vector "OVMF_CODE_4M.fd (last 16 bytes; 16 bytes at file offset size-0xf0)" "$OBJV; $OVMFV" \
    "tail -c 16 OVMF_CODE_4M.fd > vector.bin; objdump -D -b binary -m i8086 --adjust-vma=0xfffffff0 vector.bin (and the same for 0xffffff10)" "$rc"
[ "$rc" = 0 ] || status=1

# 2. QEMU halted at power-on (-S), GDB attached through QEMU's GDB stub (-gdb)
port=$((20000 + RANDOM % 20000))
cp /usr/share/OVMF/OVMF_VARS_4M.fd $B/vars.fd
$QEMU -machine q35 -m 256M -display none -serial null -monitor none -net none -S -gdb tcp:127.0.0.1:$port \
    -drive if=pflash,format=raw,readonly=on,file=$CODE -drive if=pflash,format=raw,file=$B/vars.fd &
qpid=$!
sleep 1
timeout 60 gdb -batch -nx -ex "target remote 127.0.0.1:$port" -x reset.gdb 2>&1 | trim_regs \
    | grep -v -E '^warning: No executable|^determining executable|^Kill the program|^\[Inferior' > gdb_reset.out; rc=${PIPESTATUS[0]}
wait $qpid 2>/dev/null
rec gdb_reset reset.gdb "$GDBV; $QEMUV; $OVMFV" \
    "$QEMU -machine q35 -m 256M -display none -serial null -monitor none -net none -S -gdb tcp:127.0.0.1:<port> -drive if=pflash,...OVMF_CODE_4M.fd -drive if=pflash,...OVMF_VARS_4M.fd(copy) & gdb -batch -nx -ex 'target remote 127.0.0.1:<port>' -x reset.gdb" \
    "$rc" "$EMU"
grep -q "CS =f000 ffff0000" gdb_reset.out || status=1

# 3. page-table root and one translation walked by hand (walk.py talks to the QEMU monitor)
cp /usr/share/OVMF/OVMF_VARS_4M.fd $B/vars.fd
sock="$PWD/$B/mon.sock"
python3 walk.py 6 -- $QEMU -machine q35 -m 256M -display none -serial null -net none \
    -monitor unix:$sock,server,nowait \
    -drive if=pflash,format=raw,readonly=on,file=$CODE -drive if=pflash,format=raw,file=$B/vars.fd > walk.out 2>&1; rc=$?
rec walk walk.py "$(python3 --version); $QEMUV; $OVMFV" \
    "python3 walk.py 6 -- $QEMU -machine q35 -m 256M -display none -serial null -net none -monitor unix:<socket>,server,nowait -drive if=pflash,...OVMF_CODE_4M.fd -drive if=pflash,...OVMF_VARS_4M.fd(copy)" \
    "$rc (0 = the hand walk and QEMU's gva2gpa agree)" "$EMU"
[ "$rc" = 0 ] || status=1

# 4. forensic evidence: the healthy firmware's console for 8 s, then a copy of the firmware
#    whose last 16 bytes were overwritten with zeros: its console, its reset bytes and a GDB look
python3 ../F3-10/qemu_run.py --timeout 8 -- $QEMU -machine q35 -m 256M -display none -serial stdio -no-reboot -net none \
    -drive if=pflash,format=raw,readonly=on,file=$CODE -drive if=pflash,format=raw,file=$B/vars.fd > healthy_console.out 2>&1; rc=$?
rec healthy_console "(no listing: OVMF with no disk attached)" "$QEMUV; $OVMFV" \
    "python3 ../F3-10/qemu_run.py --timeout 8 -- $QEMU -machine q35 -m 256M -display none -serial stdio -no-reboot -net none -drive if=pflash,...OVMF_CODE_4M.fd -drive if=pflash,...OVMF_VARS_4M.fd(copy)" \
    "$rc (124 = stopped by the 8 s time limit)" "$EMU"
cp $CODE $B/broken_code.fd
dd if=/dev/zero of=$B/broken_code.fd bs=1 seek=$((size - 16)) count=16 conv=notrunc status=none
cp /usr/share/OVMF/OVMF_VARS_4M.fd $B/vars.fd
python3 ../F3-10/qemu_run.py --timeout 8 -- $QEMU -machine q35 -m 256M -display none -serial stdio -no-reboot -net none \
    -drive if=pflash,format=raw,readonly=on,file=$B/broken_code.fd -drive if=pflash,format=raw,file=$B/vars.fd > forensic_console.out 2>&1; rc=$?
[ -s forensic_console.out ] || echo "(the serial port printed nothing in 8 seconds)" > forensic_console.out
rec forensic_console "(no listing: broken_code.fd)" "$QEMUV" \
    "python3 ../F3-10/qemu_run.py --timeout 8 -- $QEMU -machine q35 -m 256M -display none -serial stdio -no-reboot -net none -drive if=pflash,...broken_code.fd -drive if=pflash,...OVMF_VARS_4M.fd(copy)" \
    "$rc (124 = stopped by the 8 s time limit)" "$EMU"
g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined reset_bytes.cpp -o $B/reset_bytes
(cd $B && ./reset_bytes broken_code.fd) > forensic_bytes.out 2>&1; rc=$?
rec forensic_bytes reset_bytes.cpp "$(g++ --version | head -n 1)" \
    "g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined reset_bytes.cpp -o reset_bytes; ./reset_bytes broken_code.fd" "$rc"
cp /usr/share/OVMF/OVMF_VARS_4M.fd $B/vars.fd
$QEMU -machine q35 -m 256M -display none -serial null -monitor none -net none -S -gdb tcp:127.0.0.1:$((port + 1)) \
    -drive if=pflash,format=raw,readonly=on,file=$B/broken_code.fd -drive if=pflash,format=raw,file=$B/vars.fd &
qpid=$!
sleep 1
timeout 60 gdb -batch -nx -ex "target remote 127.0.0.1:$((port + 1))" -x reset.gdb 2>&1 | trim_regs \
    | grep -v -E '^warning: No executable|^determining executable|^Kill the program|^\[Inferior' > forensic_gdb.out; rc=${PIPESTATUS[0]}
wait $qpid 2>/dev/null
rec forensic_gdb reset.gdb "$GDBV; $QEMUV" \
    "same as gdb_reset, with broken_code.fd instead of OVMF_CODE_4M.fd" "$rc" "$EMU"

rm -rf "$B"
exit $status
