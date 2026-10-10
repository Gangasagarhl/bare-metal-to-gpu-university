#!/usr/bin/env bash
# F3-26 lab: build the OS303 kernel, run the B9 thread tests in QEMU, then the forensic
# case (a kernel stack overflow caught by a guard page) and addr2line on its backtrace.
set -u
cd "$(dirname "$0")"
. ./labtools.sh
B=.build; rm -rf "$B"; status=0
build_step build "$B" || status=1
qemu_step threads "F3-26 kernel, test_threads.cc" "$B" 1 threads 1 || status=1
qemu_step forensic_overflow "F3-26 kernel, test_threads.cc (parse_group)" "$B" 1 stackoverflow 3 || status=1
addrs=$(sed -n '/backtrace/,/PANIC/p' forensic_overflow.out | grep -oE '^ +[0-9a-f]{16}$' | awk '!seen[$0]++' | sed 's/^ */0x/')
a2l_step forensic_addr2line "$B" $addrs
rm -rf "$B"
exit $status
