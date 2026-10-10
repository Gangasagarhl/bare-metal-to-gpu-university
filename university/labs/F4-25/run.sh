#!/usr/bin/env bash
# F4-25 run.sh: build the D2 kernel (MMU, GICv3, generic timer), boot it on QEMU virt with
# GICv3 and with GICv2, record a UART-interrupt mistake, and make the forensic evidence
# (a timer handler that never re-arms the timer).
set -u -o pipefail
cd "$(dirname "$0")"
. ../F4-23/dlab.sh
status=0
SRC="../F4-24/boot.S ../F4-24/vectors.S ../F4-24/trap.cc ../F4-24/pl011.cc ../F4-24/psci.cc \
mmu.cc gic.cc gtimer.cc d2_main.cc ../F4-23/neutral_tests.cc"
QA64="qemu-system-aarch64 -M virt,gic-version=3 -cpu cortex-a57 -m 256M -display none -monitor none -serial stdio -no-reboot"

# 1. build
kbuild aarch64 k_d2 "" $SRC; rc=$?
{
    echo "== sections start on 4 KiB pages (readelf -SW k_d2.elf) =="
    aarch64-linux-gnu-readelf -SW k_d2.elf | grep -E ' \.(text|rodata|data|bss) '
    aarch64-linux-gnu-nm k_d2.elf | grep -E ' (__text_end|__rodata_end|__stack_top)$'
} > build.out 2>&1
rec build "dlab.sh kbuild; F4-24 files + mmu.cc gic.cc gtimer.cc d2_main.cc + F3-18 files" "$A64_VER" \
    "kbuild aarch64 k_d2 \"\" $SRC" "$rc"
[ "$rc" = 0 ] || status=1

# 2. GICv3 machine; the UART receives the six bytes of typed.txt ("hello" and a newline)
timeout 30 $QA64 -kernel k_d2.bin < typed.txt > d2.out 2>&1; rc=$?
rec d2 "d2_main.cc" "$QEMU_A64_VER" "$QA64 -kernel k_d2.bin < typed.txt" "$rc" \
    "note:      exit code 0 after PSCI SYSTEM_OFF; pass is the line 'D2 ok'" "$HW_NOTE"
grep -q '^D2 ok' d2.out || status=1

# 3. GICv2 machine: the kernel must refuse clearly (D2 acceptance test 2)
QV2="${QA64/gic-version=3/gic-version=2}"
timeout 30 $QV2 -kernel k_d2.bin < /dev/null > gicv2.out 2>&1; rc=$?
rec gicv2 "d2_main.cc" "$QEMU_A64_VER" "$QV2 -kernel k_d2.bin" "$rc" "$HW_NOTE"
grep -q 'unsupported by this kernel' gicv2.out || status=1

# 4. a mistake seen while writing this lab: clearing every PL011 interrupt status on enable
kbuild aarch64 k_mis "-DMISTAKE_CLEAR_RX_STATUS" $SRC || status=1
timeout 30 $QA64 -kernel k_mis.bin < typed.txt 2>&1 | tail -n 5 > rx_mistake.out; rc=$?
rec rx_mistake "pl011.cc built with -DMISTAKE_CLEAR_RX_STATUS (last 5 lines kept)" "$QEMU_A64_VER" \
    "$QA64 -kernel k_mis.bin < typed.txt | tail -n 5" "$rc" "result:    D2 FAILED is the expected result of this build" "$HW_NOTE"
grep -q 'D2 FAILED' rx_mistake.out || status=1

# 5. forensic: the timer handler does not set the next deadline
kbuild aarch64 k_storm "-DFORENSIC_NO_REARM" $SRC || status=1
timeout 15 $QA64 -kernel k_storm.bin < typed.txt > .storm.txt 2>&1; rc=$?
{
    awk 'NR <= 3' .storm.txt
    echo "[...]"
    grep -E '^(GICD|generic timer)' .storm.txt
    grep -E '^irq: ' .storm.txt | head -n 3
    echo "[...]"
    tail -n 3 .storm.txt
    echo "lines starting with 'irq: tick': $(grep -c '^irq: tick' .storm.txt)"
    echo "lines starting with 'ticks:': $(grep -c '^ticks:' .storm.txt)"
} > forensic_storm.out
rec forensic_storm "d2_main.cc built with -DFORENSIC_NO_REARM (trimmed: [...] marks cuts)" "$QEMU_A64_VER" \
    "$QA64 -kernel k_storm.bin < typed.txt" "$rc" "note:      124 = stopped by the 15 s time limit" "$HW_NOTE"
[ "$rc" = 124 ] || status=1
rm -f .storm.txt k_d2.elf k_d2.bin k_mis.elf k_mis.bin k_storm.elf k_storm.bin
exit $status
