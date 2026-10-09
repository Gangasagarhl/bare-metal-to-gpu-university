#!/usr/bin/env bash
# F3-27 lab: the B10 synchronization tests inside the OS303 kernel (QEMU, 1 CPU), then the
# forensic case (a spinlock taken both by a thread and by the timer interrupt handler).
set -u
cd "$(dirname "$0")"
. ../F3-26/labtools.sh
B=.build; rm -rf "$B"; status=0
build_step build "$B" || status=1
qemu_step sync "F3-27 sync.cc, test_sync.cc" "$B" 1 sync 1 || status=1
qemu_step forensic_irqlock "F3-27 test_sync.cc (stats_worker, stats_hook)" "$B" 1 irqlock 3 || status=1
addrs=$(grep -oE 'rip=0x[0-9a-f]+' forensic_irqlock.out | sed 's/rip=//' | awk '!seen[$0]++')
a2l_step forensic_addr2line "$B" $addrs
rm -rf "$B"
exit $status
