#!/usr/bin/env bash
# F3-11 lab (milestone A4, the part this build can do): look inside the OVMF firmware image
# and watch the CPU during the first seconds of a boot.
#   1. fvscan: firmware volumes, firmware files by PI type (SEC, PEI, DXE ...) and module names;
#   2. sample.py: CPU mode and memory region over time, sampled through the QEMU monitor;
#   3. forensic evidence: the same two views of a firmware image with 4 bytes overwritten.
# Every step writes <name>.out (real output) and <name>.log (run record).
set -u -o pipefail
cd "$(dirname "$0")"
status=0
B=.build
rm -rf "$B"; mkdir -p "$B"
QEMU=qemu-system-x86_64
QEMUV="$($QEMU --version | head -n 1)"
GXX="g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
GXXV="$(g++ --version | head -n 1); liblzma $(dpkg-query -W -f '${Version}' liblzma-dev 2>/dev/null)"
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

# 1. build and run fvscan on the firmware image the labs boot
$GXX fvscan.cc -llzma -o $B/fvscan > $B/build.txt 2>&1 || { cat $B/build.txt; status=1; }
$B/fvscan $CODE > fvscan.out 2>&1; rc=$?
rec fvscan fvscan.cc "$GXXV; input: $OVMFV" "$GXX fvscan.cc -llzma -o fvscan && ./fvscan $CODE" "$rc"
[ "$rc" = 0 ] || status=1
grep -q "SecMain" fvscan.out || status=1
python3 lastfile.py $CODE 0x348000 > lastfile.out 2>&1; rc=$?
rec lastfile lastfile.py "$(python3 --version); input: $OVMFV" "python3 lastfile.py $CODE 0x348000" "$rc"
[ "$rc" = 0 ] || status=1

# 2. sample the CPU every 5 ms for 4 s from power-on (QEMU starts halted, sample.py resumes it)
cp /usr/share/OVMF/OVMF_VARS_4M.fd $B/vars.fd
python3 sample.py 4 5 -- $QEMU -machine q35 -m 256M -display none -serial null -net none -S \
    -monitor unix:$PWD/$B/m1.sock,server,nowait \
    -drive if=pflash,format=raw,readonly=on,file=$CODE -drive if=pflash,format=raw,file=$B/vars.fd > sample.out 2>&1; rc=$?
rec sample sample.py "$(python3 --version); $QEMUV; $OVMFV" \
    "python3 sample.py 4 5 -- $QEMU -machine q35 -m 256M -display none -serial null -net none -S -monitor unix:<socket>,server,nowait -drive if=pflash,...OVMF_CODE_4M.fd -drive if=pflash,...OVMF_VARS_4M.fd(copy)" \
    "$rc" "$EMU; times are host wall-clock times in this container (TCG emulation), not the speed of any real PC"
[ "$rc" = 0 ] || status=1

# 3. forensic evidence: four bytes at file offset 0x10000 overwritten with 0x55
cp $CODE $B/broken.fd
printf '\x55\x55\x55\x55' | dd of=$B/broken.fd bs=1 seek=$((0x10000)) conv=notrunc status=none
cmp -l $CODE $B/broken.fd | head -n 4 > $B/cmp.txt
{ echo "cmp -l OVMF_CODE_4M.fd broken.fd (byte offset counted from 1, old and new values in octal):"; cat $B/cmp.txt; } > forensic_cmp.out
rec forensic_cmp "(no listing: cmp)" "$(cmp --version | head -n 1)" "cmp -l OVMF_CODE_4M.fd broken.fd" "0"
python3 ../F3-10/qemu_run.py --timeout 10 -- $QEMU -machine q35 -m 256M -display none -serial stdio -no-reboot -net none \
    -drive if=pflash,format=raw,readonly=on,file=$B/broken.fd -drive if=pflash,format=raw,file=$B/vars.fd > forensic_console.out 2>&1; rc=$?
[ -s forensic_console.out ] || echo "(the serial port printed nothing in 10 seconds)" > forensic_console.out
rec forensic_console "(no listing: broken.fd)" "$QEMUV" \
    "python3 ../F3-10/qemu_run.py --timeout 10 -- $QEMU -machine q35 -m 256M -display none -serial stdio -no-reboot -net none -drive if=pflash,...broken.fd -drive if=pflash,...OVMF_VARS_4M.fd(copy)" \
    "$rc (124 = stopped by the 10 s time limit)" "$EMU"
cp /usr/share/OVMF/OVMF_VARS_4M.fd $B/vars.fd
python3 sample.py 4 5 -- $QEMU -machine q35 -m 256M -display none -serial null -net none -S \
    -monitor unix:$PWD/$B/m2.sock,server,nowait \
    -drive if=pflash,format=raw,readonly=on,file=$B/broken.fd -drive if=pflash,format=raw,file=$B/vars.fd > forensic_sample.out 2>&1; rc=$?
rec forensic_sample sample.py "$(python3 --version); $QEMUV" \
    "python3 sample.py 4 5 -- (same QEMU command as the sample step, with broken.fd)" "$rc" "$EMU"
$B/fvscan $B/broken.fd 2>&1 | sed "s#$B/##" > forensic_fvscan.out; rc=${PIPESTATUS[0]}
rec forensic_fvscan fvscan.cc "$GXXV" "./fvscan broken.fd" "$rc"

rm -rf "$B"
exit $status
