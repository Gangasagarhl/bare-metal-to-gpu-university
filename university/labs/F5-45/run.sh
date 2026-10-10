#!/usr/bin/env bash
# F5-45 run.sh: the DS403 kernel speaks 9P to QEMU's own 9P server (virtio-9p-pci), with
# QEMU's v9fs trace events switched on so that the server's view of every message is recorded.
#   1. build       kernel f545.elf (boot.S, k.cc from F5-44 + virtio9p.cc p9.cc f545_main.cc)
#   2. session     version, attach, walk, open, read, create, write, clunk; host checks the file
#   3. forensic    the same session asked to walk to "Hello.txt"
set -u -o pipefail
cd "$(dirname "$0")"
. ../F5-44/lablib.sh
HW_NOTE="hardware:  untested on hardware; QEMU 8.2.2 with TCG (no KVM), one emulated PC with a virtio-9p-pci device sharing a host directory (no physical counterpart)"
status=0
K=../F5-44
KSRC="$K/boot.S $K/k.cc virtio9p.cc p9.cc f545_main.cc"
B=.build
rm -rf "$B"; mkdir -p "$B/export"

kbuild f545.elf $KSRC > build.out 2>&1; rc=$?
size -A f545.elf | grep -E '^(section|\.text|\.rodata|\.data|\.bss|Total)' >> build.out
rec build "boot.S k.cc (F5-44) virtio9p.cc p9.cc f545_main.cc kernel.ld" "$GXX_VER; $LD_VER" \
    "g++ $KFLAGS -c <each file>; ld -m elf_i386 -nostdlib -static -T kernel.ld -o f545.elf *.o" "$rc"
[ "$rc" = 0 ] || status=1

Q9="$QEMU $QNODE -serial stdio -kernel f545.elf -fsdev local,id=f0,path=$B/export,security_model=none -device virtio-9p-pci,fsdev=f0,mount_tag=ds403share,disable-modern=on -trace v9fs_*"
session() {  # session <name> <cmdline>
    printf 'hello from the host side of 9P\n' > "$B/export/hello.txt"
    rm -f "$B/export/from-kernel.txt"
    timeout 30 $Q9 -append "$2" > "$1.out" 2>&1
    local rc=$?
    sed -i 's/\r$//' "$1.out"
    rec "$1" "f545_main.cc virtio9p.cc p9.cc (kernel f545.elf)" "$QEMU_VER" \
        "$QEMU $QNODE -serial stdio -kernel f545.elf -append \"$2\" -fsdev local,id=f0,path=export,security_model=none -device virtio-9p-pci,fsdev=f0,mount_tag=ds403share,disable-modern=on -trace v9fs_*" "$rc" \
        "note:      lines starting with v9fs_ are QEMU's own trace of its 9P server; the others are the kernel's serial output" \
        "note:      exit code 33 = the kernel wrote 0x10 to isa-debug-exit" "$HW_NOTE"
    return $rc
}

session session "node=1"; rc=$?
[ "$rc" = 33 ] || status=1
{
    echo "host: ls -l export (owner columns left out)"
    ls -l "$B/export" | awk 'NR>1 {print "  " $1, $5, $9}'
    echo "host: cat export/from-kernel.txt"
    sed 's/^/  /' "$B/export/from-kernel.txt"
    grep -q '^written by the DS403 kernel over 9P$' "$B/export/from-kernel.txt" && echo "file written through 9P is on the host: PASS" || echo "FAIL"
    grep -q '31 bytes: "hello from the host side of 9P\\n"' session.out && echo "kernel read the host file through 9P: PASS" || echo "FAIL"
    for id in 100 104 110 112 116 114 118 120; do
        grep -q " id $id " session.out && echo "QEMU's trace shows message id $id: PASS" || echo "QEMU's trace shows message id $id: FAIL"
    done
} > check.out
grep -q FAIL check.out && { rc=1; status=1; } || rc=0
rec check "run.sh step 2 (host-side ls, cat, grep)" "$(ls --version | head -n 1)" "see run.sh" "$rc"

session forensic "node=1 walk=Hello.txt"; rc=$?
[ "$rc" = 33 ] || status=1
rm -rf "$B" f545.elf
exit $status
