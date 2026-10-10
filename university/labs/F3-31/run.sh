#!/usr/bin/env bash
# F3-31 run.sh: build the initial RAM disk and the mini kernel, boot it in QEMU, compare the
# kernel's listing with the host's view of the same archive (B14 test 1), and make the
# forensic evidence (a block cache with one changed line).
set -u -o pipefail
cd "$(dirname "$0")"
. ./lablib.sh
status=0
KSRC="boot.S kbase.cc vfs.cc tarfs.cc infofs.cc b14_main.cc"

# 1. the archive and the host's view of it
./mkinitrd.sh initrd.tar; rc=$?
{ tar --numeric-owner -tvf initrd.tar | awk '{print $1, $2, $3, $6}'; echo "archive size: $(stat -c %s initrd.tar) bytes"; } > initrd.out
rec initrd "mkinitrd.sh" "$(tar --version | head -n 1)" "./mkinitrd.sh initrd.tar; tar --numeric-owner -tvf initrd.tar" "$rc"
[ "$rc" = 0 ] || status=1
python3 host_view.py initrd.tar > host_view.out; rc=$?
rec host_view "host_view.py" "$(python3 --version)" "python3 host_view.py initrd.tar" "$rc"
python3 tar_header.py initrd.tar \
  docs/guides/a-rather-long-directory-name-for-testing/another-long-directory-name-inside/a-file-whose-full-path-is-longer-than-one-hundred-characters.txt \
  > tarhdr.out; rc=$?
rec tarhdr "tar_header.py" "$(python3 --version)" "python3 tar_header.py initrd.tar docs/guides/.../a-file-whose-full-path-is-longer-than-one-hundred-characters.txt" "$rc"

# 2. the kernel
kbuild k31.elf $KSRC > kbuild.txt 2>&1; rc=$?
{ cat kbuild.txt; size -A k31.elf | grep -E '^(section|\.text|\.rodata|\.data|\.bss|Total)'; } > build.out
rec build "boot.S kbase.cc vfs.cc tarfs.cc infofs.cc b14_main.cc kernel.ld" "$GXX_VER; $LD_VER" \
    "g++ $KFLAGS -c <each file>; ld -m elf_i386 -nostdlib -static -T kernel.ld -o k31.elf *.o" "$rc"
[ "$rc" = 0 ] || status=1

# 3. boot with the archive as a Multiboot module
qboot boot.out 30 k31.elf -initrd initrd.tar; rc=$?
sed -i 's/\r$//' boot.out
rec boot "b14_main.cc (kernel k31.elf, module initrd.tar)" "$QEMU_VER" \
    "$QEMU $QBASE -serial stdio -kernel k31.elf -initrd initrd.tar" "$rc" \
    "note:      exit code 33 = pass: the kernel wrote 0x10 to isa-debug-exit and QEMU exits with (0x10 << 1) | 1" \
    "note:      the carriage returns the kernel sends before each newline were removed from boot.out" "$HW_NOTE"
[ "$rc" = 33 ] || status=1

# 4. acceptance test 1: the kernel's listing equals the host's view (lines under /sys come
#    from infofs, which is not in the archive, so they are left out of the comparison)
sed -n '/^== listing ==$/,/^== end of listing ==$/p' boot.out | grep -E '^[d-] ' \
    | grep -vE ' /sys(/|$)' | sort > .k.txt
grep -E '^[d-] ' host_view.out | grep -vE ' /sys(/|$)' | sort > .h.txt
{
    echo "kernel lines compared: $(wc -l < .k.txt); host lines: $(wc -l < .h.txt)"
    if diff .h.txt .k.txt; then echo "listing: MATCH"; else echo "listing: DIFFERENT"; fi
    k="$(grep '^fnv1a' boot.out)"; h="$(grep '^fnv1a' host_view.out)"
    echo "kernel: $k"; echo "host:   $h"
    [ "$k" = "$h" ] && echo "contents: MATCH" || echo "contents: DIFFERENT"
} > compare.out
grep -q '^listing: MATCH' compare.out && grep -q '^contents: MATCH' compare.out; rc=$?
rec compare "run.sh step 4 (diff of boot.out listing and host_view.out)" "$(diff --version | head -n 1)" \
    "diff <(host listing, sorted) <(kernel listing, sorted); compare fnv1a lines" "$rc"
[ "$rc" = 0 ] || status=1

# 5. forensic evidence: the same cache with one line changed (see the answer key)
sed 's/^        buf.dirty = true;                \/\/ the disk copy is now out of date/        if (last_was_miss_) buf.dirty = true;/' \
    bcache.cpp > .bcache_change.cc
hostbuild .fb -x c++ .bcache_change.cc > .fb.txt 2>&1 && { timeout 60 ./.fb > forensic_bcache.out 2>&1; rc=$?; } || rc=99
rec forensic_bcache "bcache.cpp after the colleague's change (one line; see the answer key)" "$GXX_VER" \
    "g++ $HOSTFLAGS bcache_changed.cc -o bcache_changed; ./bcache_changed" "$rc" \
    "note:      exit code 1 is expected: the program's own check reports a read mismatch"
[ "$rc" = 1 ] || status=1

rm -f k31.elf initrd.tar kbuild.txt .k.txt .h.txt .fb .fb.txt .bcache_change.cc
sed -i "s#$(pwd)/##g" ./*.out
exit $status
