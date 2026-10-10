#!/usr/bin/env bash
# F12-21 run.sh: the multi-tool steps of the chapter.
#   ifstream_strace : Listing 1 (ifstream_read.cpp) under strace: C++ reads versus read() calls.
#   nvme_qemu       : an NVMe Read command seen by QEMU's emulated NVMe controller. The host
#                     software is QEMU's own x86 firmware (SeaBIOS) reading the boot sector of an
#                     NVMe disk; QEMU's trace events print each command the controller receives.
#   uorb_polled     : the forensic evidence: uorb_trace.cpp with the configuration uorb_polled.in.
#   tools_check     : records which kernel tracing tools this machine has (most are missing here).
set -u
cd "$(dirname "$0")"
FLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
status=0
head_log() {  # head_log <name> <listing> <toolchain line> <command>
    {
        echo "listing:   $2 (run by run.sh)"
        echo "toolchain: $3"
        echo "command:   $4"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
    } > "$1.log"
}
gxx="$(g++ --version | head -n 1)"

# ---- step 1: ifstream_strace ---------------------------------------------------------------
head_log ifstream_strace ifstream_read.cpp "$gxx; $(strace -V | head -n 1)" \
    "g++ $FLAGS ifstream_read.cpp -o ifstream_read; ASAN_OPTIONS=detect_leaks=0 strace -e trace=openat,read,close -o trace.txt ./ifstream_read"
if g++ $FLAGS ifstream_read.cpp -o .bin_ifs 2> ifs.build; then
    # LeakSanitizer cannot run under ptrace, so leak checking is switched off for this run only
    ASAN_OPTIONS=detect_leaks=0 timeout 10 strace -e trace=openat,read,close -o .trace.txt \
        ./.bin_ifs > .prog.txt 2>&1; rc=$?
    {
        echo "--- what the program printed"
        cat .prog.txt
        echo "--- what strace saw for sample.txt (other files and descriptors left out)"
        awk '/"sample.txt"/ { fd = $NF; print; next }
             fd != "" && ($0 ~ "^read\\(" fd "," || $0 ~ "^close\\(" fd "\\)") { print }' .trace.txt
    } > ifstream_strace.out
    echo "exit code: $rc" >> ifstream_strace.log
    [ "$rc" = 0 ] || status=1
else
    echo "result:    BUILD FAILED" >> ifstream_strace.log; cat ifs.build >> ifstream_strace.log
    status=1
fi
rm -f .bin_ifs .trace.txt .prog.txt ifs.build

# ---- step 2: nvme_qemu ---------------------------------------------------------------------
qv="$(qemu-system-x86_64 --version | head -n 1)"
head_log nvme_qemu "run.sh (disk image made here)" "$qv" \
    "qemu-system-x86_64 -M pc -display none -serial none -monitor none -no-reboot -chardev file,id=dbg,path=fw.txt -device isa-debugcon,iobase=0x402,chardev=dbg -device isa-debug-exit,iobase=0xf4,iosize=0x04 -drive file=nvme.img,if=none,id=nv0,format=raw -device nvme,serial=se403demo,drive=nv0 -trace pci_nvme_admin_cmd -trace pci_nvme_io_cmd -trace pci_nvme_read"
echo "hardware:  emulated NVMe controller only (QEMU -device nvme); untested on a real NVMe drive" \
    >> nvme_qemu.log
# 1 MiB disk; sector 0 = boot sector: mov al,1 / out 0xf4,al (isa-debug-exit) / hlt / jmp $-1
dd if=/dev/zero of=nvme.img bs=1024 count=1024 status=none
printf '\xb0\x01\xe6\xf4\xf4\xeb\xfd' | dd of=nvme.img conv=notrunc status=none
printf '\x55\xaa' | dd of=nvme.img bs=1 seek=510 conv=notrunc status=none
timeout 30 qemu-system-x86_64 -M pc -display none -serial none -monitor none -no-reboot \
    -chardev file,id=dbg,path=fw.txt -device isa-debugcon,iobase=0x402,chardev=dbg \
    -device isa-debug-exit,iobase=0xf4,iosize=0x04 \
    -drive file=nvme.img,if=none,id=nv0,format=raw -device nvme,serial=se403demo,drive=nv0 \
    -trace pci_nvme_admin_cmd -trace pci_nvme_io_cmd -trace pci_nvme_read 2> .qtrace.txt
rc=$?
{
    echo "--- firmware messages (debug console), first lines and the boot line"
    grep -a -m 1 'SeaBIOS (version' fw.txt
    grep -a 'Booting from' fw.txt
    echo "--- admin commands the controller received (count, command)"
    grep -a 'pci_nvme_admin_cmd' .qtrace.txt | sed -n "s/.*opname '\\(.*\\)'.*/\\1/p" | uniq -c
    echo "--- I/O commands the controller received"
    grep -a -E 'pci_nvme_io_cmd|pci_nvme_read' .qtrace.txt
    echo "--- QEMU exit status $rc (3 = the boot sector ran and wrote 1 to isa-debug-exit)"
} > nvme_qemu.out
echo "exit code: $rc (expected 3: (1 << 1) | 1 from isa-debug-exit)" >> nvme_qemu.log
[ "$rc" = 3 ] || status=1
rm -f nvme.img fw.txt .qtrace.txt

# ---- step 3: uorb_polled -------------------------------------------------------------------
head_log uorb_polled uorb_trace.cpp "$gxx" \
    "g++ $FLAGS uorb_trace.cpp -o uorb_trace; ./uorb_trace < uorb_polled.in"
if g++ $FLAGS uorb_trace.cpp -o .bin_uorb 2> uorb.build; then
    timeout 10 ./.bin_uorb < uorb_polled.in > uorb_polled.out 2>&1; rc=$?
    echo "exit code: $rc" >> uorb_polled.log
    echo "stdin:     uorb_polled.in" >> uorb_polled.log
    [ "$rc" = 0 ] || status=1
else
    echo "result:    BUILD FAILED" >> uorb_polled.log; cat uorb.build >> uorb_polled.log; status=1
fi
rm -f .bin_uorb uorb.build

# ---- step 4: tools_check: which kernel tracing tools does this machine offer? ----------------
head_log tools_check run.sh "$(uname -sr)" \
    "perf --version; ls /sys/kernel/tracing; command -v bpftrace trace-cmd blktrace; head -c 0 /proc/kallsyms"
{
    echo "\$ perf --version"; perf --version 2>&1 | grep -v '^$' | head -n 3
    echo "\$ ls /sys/kernel/tracing"
    if [ -n "$(ls -A /sys/kernel/tracing 2>/dev/null)" ]; then ls /sys/kernel/tracing | head -n 3
    else echo "(empty or missing: no tracing file system mounted)"; fi
    for t in bpftrace trace-cmd blktrace; do
        echo "\$ command -v $t"; command -v "$t" || echo "(not installed)"
    done
    echo "\$ test -r /proc/kallsyms && test -r /proc/self/stack"
    if test -r /proc/kallsyms && test -r /proc/self/stack; then echo "both readable"; else echo "not readable"; fi
} > tools_check.out 2>&1
echo "exit code: 0" >> tools_check.log
exit $status
