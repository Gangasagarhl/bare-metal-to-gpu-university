#!/usr/bin/env bash
# F11-05 lab: TrustZone on QEMU's AArch64 "virt" board with secure=on.
#   tz             our EL3 monitor + normal-world program: SMC service, then a direct read of
#                  secure RAM from the normal world (expected: a synchronous external abort)
#   tz_control     the same program, but the monitor leaves EL1 in the SECURE state (SCR_EL3.NS=0):
#                  the same read succeeds; the NS bit is the difference
#   crosscheck     Python recomputes the keyed digests printed in tz.out
#   forensic_peek  a monitor with a leftover "peek" service, and the program that used it
#   fixed_peek     the same program against a monitor whose peek checks the address
# Every step writes <name>.out (real output) and <name>.log (run record).
set -u -o pipefail
cd "$(dirname "$0")"
status=0
B=.build
rm -rf "$B"; mkdir -p "$B"
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
CXXF="--target=aarch64-none-elf -mcpu=cortex-a57 -std=c++20 -ffreestanding -fno-exceptions -fno-rtti -nostdlib -mgeneral-regs-only -O2 -Wall -Wextra -Wpedantic -Werror"
TC="$(clang++ --version | head -n 1); $(ld.lld --version | head -n 1); $(qemu-system-aarch64 --version | head -n 1)"
HW="hardware:  untested on hardware; QEMU virt board (secure=on, cortex-a57), not a real Arm SoC; no TF-A, no OP-TEE"
QEMU="qemu-system-aarch64 -machine virt,secure=on -cpu cortex-a57 -nographic -monitor none -serial stdio -semihosting-config enable=on,target=native"

monitor() {  # monitor <out.bin> [defines...]
    local out=$1; shift
    clang++ $CXXF "$@" -c monitor.cc -o $B/monitor.o && clang --target=aarch64-none-elf "$@" -c el3_entry.S -o $B/el3_entry.o &&
        ld.lld -T el3.ld $B/el3_entry.o $B/monitor.o -o $B/el3.elf && llvm-objcopy -O binary $B/el3.elf "$out" ||
        { echo "F11-05: monitor build failed"; status=1; }
}
normal() {  # normal <out.elf> [defines...]
    local out=$1; shift
    clang++ $CXXF "$@" -c normal.cc -o $B/normal.o && clang --target=aarch64-none-elf -c ns_entry.S -o $B/ns_entry.o &&
        ld.lld -T ns.ld $B/ns_entry.o $B/normal.o -o "$out" || { echo "F11-05: normal-world build failed"; status=1; }
}
run() {  # run <name> <el3.bin> <ns.elf> <listing text> <expected exit> <exit meaning>
    timeout 30 $QEMU -bios "$2" -device loader,file="$3" > "$1.out" 2>&1; local rc=$?
    sed -i 's/\r$//' "$1.out"
    rec "$1" "$4" "$TC" "$QEMU -bios $(basename "$2") -device loader,file=$(basename "$3")" "$rc ($6)" "$HW"
    [ "$rc" = "$5" ] || { echo "F11-05: $1 exited $rc, expected $5"; status=1; }
}

monitor $B/el3.bin
normal $B/ns.elf
run tz $B/el3.bin $B/ns.elf "monitor.cc el3_entry.S el3.ld (secure, EL3); normal.cc ns_entry.S ns.ld (EL1)" 0 \
    "0 = the normal world's abort handler asked the monitor to end the run with code 0"
grep -q "EC 0x25, DFSC 0x10" tz.out || status=1

monitor $B/el3_control.bin -DCONTROL_SECURE_EL1
run tz_control $B/el3_control.bin $B/ns.elf "monitor.cc el3_entry.S built with -DCONTROL_SECURE_EL1; normal.cc unchanged" 1 \
    "1 = the read of secure RAM succeeded and the program asked the monitor to end the run with code 1"
grep -q "read succeeded" tz_control.out || status=1

# crosscheck: the demo key is byte i = 0xA5 xor (7 i mod 256); digest = first 8 bytes of SHA-256(key || message, 8 bytes little-endian)
python3 - tz.out > crosscheck.out 2>&1 <<'PY'
import hashlib, re, sys
key = bytes((0xA5 ^ (i * 7)) & 0xFF for i in range(32))
text = open(sys.argv[1]).read()
ok = True
for msg, got in re.findall(r"digest\((0x[0-9a-f]+)\) = (0x[0-9a-f]+)", text):
    want = hashlib.sha256(key + int(msg, 16).to_bytes(8, "little")).digest()[:8].hex()
    print("message %s: tz.out %s, Python hashlib 0x%s, %s" % (msg, got, want, "equal" if got == "0x" + want else "DIFFERENT"))
    ok = ok and got == "0x" + want
sys.exit(0 if ok else 1)
PY
rc=$?
rec crosscheck "(no listing: an inline Python check, shown in run.sh)" "$(python3 --version)" "python3 - tz.out  (hashlib.sha256(key + message))" "$rc"
[ "$rc" = 0 ] || status=1

monitor $B/el3_peek.bin -DWITH_PEEK
normal $B/ns_attack.elf -DATTACK
run forensic_peek $B/el3_peek.bin $B/ns_attack.elf "monitor.cc built with -DWITH_PEEK; normal.cc built with -DATTACK" 0 \
    "0 = the program asked the monitor to end the run with code 0"
monitor $B/el3_fixed.bin -DWITH_PEEK -DPEEK_CHECKED
run fixed_peek $B/el3_fixed.bin $B/ns_attack.elf "monitor.cc built with -DWITH_PEEK -DPEEK_CHECKED; normal.cc built with -DATTACK" 0 \
    "0 = the program asked the monitor to end the run with code 0"
grep -q "0x00000000ffffffff 0x00000000ffffffff" fixed_peek.out || status=1

rm -rf "$B"
exit $status
