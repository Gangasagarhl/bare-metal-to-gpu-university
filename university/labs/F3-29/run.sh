#!/usr/bin/env bash
# F3-29 lab: user mode and system calls (B12). The ELF header of a user program, the user
# tests in QEMU (CPU model 'max' so that SMEP and SMAP exist), then the forensic case
# "the kernel trusted a user pointer": a kernel built with -DBUG_TRUST_USER_POINTER,
# once with SMAP on and once with nosmap=1, and addr2line on the crash dump.
set -u
cd "$(dirname "$0")"
. ../F3-26/labtools.sh
B=.build; BB=.build_bug; rm -rf "$B" "$BB"; status=0
build_step build "$B" || status=1
(cd "$B" && readelf -h -l u_hello.elf) > readelf_hello.out 2>&1
rec readelf_hello "u_hello.elf (F3-29 crt0.S, ulib.cc, u_hello.cc)" "$(readelf --version | head -n 1)" \
    "readelf -h -l u_hello.elf" "$?"
qemu_step user "F3-29 syscall.S, syscall.cc, uaccess.S, test_user.cc, u_*.cc" "$B" 1 user 1 max || status=1
build_step build_bug "$BB" -DBUG_TRUST_USER_POINTER || status=1
qemu_step forensic_smap "F3-29 syscall.cc built with -DBUG_TRUST_USER_POINTER" "$BB" 1 user 3 max || status=1
qemu_step forensic_nosmap "F3-29 syscall.cc built with -DBUG_TRUST_USER_POINTER" "$BB" 1 "user nosmap=1" 3 max || status=1
addrs=$(sed -n '/backtrace/,/PANIC/p' forensic_nosmap.out | grep -oE '^ +[0-9a-f]{16}$' | awk '!seen[$0]++' | sed 's/^ */0x/')
a2l_step forensic_addr2line "$BB" $addrs
rm -rf "$B" "$BB"
exit $status
