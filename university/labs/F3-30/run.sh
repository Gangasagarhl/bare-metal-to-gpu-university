#!/usr/bin/env bash
# F3-30 lab: processes and the ELF loader (B13). The program headers of a user program,
# then in QEMU: argv/auxv and init with ten children, 10,000 spawn/exit cycles, and the
# exec-path fuzzer; then the forensic build -DBUG_LEAK_PAGE_TABLES (expected to fail).
set -u
cd "$(dirname "$0")"
. ../F3-26/labtools.sh
B=.build; BB=.build_bug; rm -rf "$B" "$BB"; status=0
build_step build "$B" || status=1
(cd "$B" && readelf -h -l u_child.elf) > readelf_child.out 2>&1
rec readelf_child "u_child.elf (F3-29 crt0.S, ulib.cc, F3-30 u_child.cc)" "$(readelf --version | head -n 1)" \
    "readelf -h -l u_child.elf" "$?"
qemu_step proc "F3-30 process.cc, test_proc.cc, u_init.cc, u_child.cc, u_args.cc" "$B" 1 proc 1 max || status=1
qemu_step execloop "F3-30 test_proc.cc (test_execloop)" "$B" 1 execloop 1 max || status=1
qemu_step fuzz "F3-30 test_proc.cc (test_fuzz), elf_check.h" "$B" 1 fuzz 1 max || status=1
build_step build_bug "$BB" -DBUG_LEAK_PAGE_TABLES || status=1
qemu_step forensic_leak "F3-30 process.cc built with -DBUG_LEAK_PAGE_TABLES" "$BB" 1 execloop 3 max || status=1
rm -rf "$B" "$BB"
exit $status
