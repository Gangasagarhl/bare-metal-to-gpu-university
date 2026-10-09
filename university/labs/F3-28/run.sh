#!/usr/bin/env bash
# F3-28 lab: SMP bring-up (B11). The kernel reads the ACPI MADT, starts every other CPU
# with INIT-SIPI-SIPI and checks the count for 1, 2, 4, 8 and 16 CPUs; then the SMP
# producer/consumer and TLB shootdown tests on 8 CPUs; then the shootdown switched off
# (expected to fail: stale TLB entries); then the forensic case "works on 1 CPU, hangs on 8".
set -u
cd "$(dirname "$0")"
. ../F3-26/labtools.sh
B=.build; rm -rf "$B"; status=0
build_step build "$B" || status=1
qemu_step madt "F3-28 acpi.cc, smp.cc (MADT printed)" "$B" 8 "smp madt=show countonly=1" 1 || status=1
for n in 1 2 4 8 16; do
    qemu_step "cpus_$n" "F3-28 smp.cc, test_smp.cc (CPU count only)" "$B" "$n" "smp countonly=1" 1 || status=1
done
qemu_step smp8 "F3-28 test_smp.cc (producer/consumer, TLB shootdown)" "$B" 8 smp 1 || status=1
qemu_step noshootdown "F3-28 test_smp.cc with the shootdown switched off" "$B" 8 "smp noshootdown=1" 3 || status=1
qemu_step abba_1cpu "F3-28 test_smp.cc (transfer, 8 tellers)" "$B" 1 abba 1 || status=1
qemu_step abba_8cpu "F3-28 test_smp.cc (transfer, 8 tellers)" "$B" 8 abba 3 || status=1
addrs=$(cat abba_1cpu.out abba_8cpu.out | grep -oE '(rip=|\(at )0x[0-9a-f]+' | grep -oE '0x[0-9a-f]+' | awk '!seen[$0]++')
a2l_step forensic_addr2line "$B" $addrs
rm -rf "$B"
exit $status
