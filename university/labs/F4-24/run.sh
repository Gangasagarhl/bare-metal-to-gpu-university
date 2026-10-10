#!/usr/bin/env bash
# F4-24 run.sh: build the AArch64 D1 kernel, boot it on QEMU virt entered at EL1 and at EL2,
# record the outline-atomics link error, and make the forensic evidence (a kernel that
# forgets to leave EL2).
set -u -o pipefail
cd "$(dirname "$0")"
. ../F4-23/dlab.sh
status=0
SRC="boot.S vectors.S trap.cc pl011.cc psci.cc d1_main.cc ../F4-23/neutral_tests.cc"
QA64="qemu-system-aarch64 -M virt,gic-version=3 -cpu cortex-a57 -m 256M -nographic -no-reboot"

# 1. build, check the image header, prove the reused F3-18 files are unchanged
kbuild aarch64 k_d1 "" $SRC; rc=$?
{
    echo "== image header: first 64 bytes of k_d1.bin (od -A x -t x1) =="
    od -A x -t x1 -N 64 k_d1.bin
    echo "== ELF entry and sections (readelf) =="
    aarch64-linux-gnu-readelf -h k_d1.elf | grep -E 'Machine|Entry'
    aarch64-linux-gnu-readelf -SW k_d1.elf | grep -E ' \.(text|rodata|data|bss) '
    echo "== vector_table address (must be 2 KiB aligned) =="
    aarch64-linux-gnu-nm k_d1.elf | grep -E ' (vector_table|_start|kmain)$'
    echo "== sha256 of the F3-18 files compiled unchanged into this kernel =="
    reuse_record
} > build.out 2>&1
rec build "dlab.sh kbuild; boot.S vectors.S trap.cc pl011.cc psci.cc d1_main.cc neutral_tests.cc + F3-18 files" \
    "$A64_VER" "kbuild aarch64 k_d1 \"\" $SRC" "$rc"
[ "$rc" = 0 ] || status=1

# 2. boot entered at EL1 (QEMU virt default)
timeout 30 $QA64 -kernel k_d1.bin > boot_el1.out 2>&1; rc=$?
rec boot_el1 "d1_main.cc" "$QEMU_A64_VER" "$QA64 -kernel k_d1.bin" "$rc" \
    "note:      exit code 0 after PSCI SYSTEM_OFF; pass is the line 'D1 ok'" "$HW_NOTE"
grep -q '^D1 ok' boot_el1.out || status=1

# 3. boot entered at EL2 (virtualization=on): boot.S must drop to EL1
QA64_EL2="qemu-system-aarch64 -M virt,gic-version=3,virtualization=on -cpu cortex-a57 -m 256M -nographic -no-reboot"
timeout 30 $QA64_EL2 -kernel k_d1.bin > boot_el2.out 2>&1; rc=$?
rec boot_el2 "d1_main.cc" "$QEMU_A64_VER" "$QA64_EL2 -kernel k_d1.bin" "$rc" \
    "note:      exit code 0 after PSCI SYSTEM_OFF; pass is the line 'D1 ok'" "$HW_NOTE"
grep -q '^D1 ok' boot_el2.out || status=1

# 4. a common mistake: building the same kernel without -mno-outline-atomics
KFLAGS_aarch64="-mgeneral-regs-only -mstrict-align" kbuild aarch64 k_outline "" $SRC > outline.out 2>&1; rc=$?
sed -i "s#$(pwd)/##g; s#\.obj_k_outline/##g" outline.out
rec outline "the D1 kernel built WITHOUT -mno-outline-atomics" "$A64_VER" \
    "kbuild aarch64 k_outline \"\" $SRC  (KFLAGS_aarch64 without -mno-outline-atomics)" \
    "$rc" "result:    link failed as expected (the undefined helper symbols are the lesson)"
rm -f k_outline.elf k_outline.bin
[ "$rc" != 0 ] || status=1

# 5. forensic: the same kernel with the EL2-to-EL1 drop compiled out, entered at EL2
kbuild aarch64 k_for "-DFORENSIC_STAY_AT_EL2" $SRC || status=1
timeout 10 $QA64_EL2 -kernel k_for.bin > forensic_serial.out 2>&1; rc=$?
rec forensic_serial "d1_main.cc with boot.S built with -DFORENSIC_STAY_AT_EL2" "$QEMU_A64_VER" \
    "$QA64_EL2 -kernel k_for.bin" "$rc" "note:      124 = stopped by the 10 s time limit (the kernel hung)" "$HW_NOTE"
[ "$rc" = 124 ] || status=1
{
    echo "== qemu -d int (first 30 lines; QEMU stopped when the pipe closed) =="
    timeout 10 $QA64_EL2 -kernel k_for.bin -serial none -monitor none -d int 2>&1 >/dev/null | head -n 30 > .int.txt
    cat .int.txt
    elr="$(grep -m 1 -o 'with ELR 0x[0-9a-f]*' .int.txt | awk '{print $3}')"
    echo "== addr2line for the first ELR ($elr) =="
    aarch64-linux-gnu-addr2line -f -C -e k_for.elf "$elr"
    rm -f .int.txt
    echo "== the instruction word at physical address 0x200 (QEMU monitor: xp /1wx 0x200) =="
    printf 'xp /1wx 0x200\nquit\n' | timeout 10 $QA64_EL2 -kernel k_for.bin -serial none -monitor stdio -S \
        2>&1 | grep -E '^0000000000000200'
} > forensic_int.out 2>&1
rec forensic_int "k_for.elf (the forensic kernel)" "$QEMU_A64_VER; $(aarch64-linux-gnu-addr2line --version | head -n 1)" \
    "$QA64_EL2 -kernel k_for.bin -d int | head -n 30; addr2line; monitor xp /1wx 0x200" "0" "$HW_NOTE"
sed -i "s#$(pwd)/##g; s#$LABS/#labs/#g" forensic_int.out
rm -f k_d1.elf k_d1.bin k_for.elf k_for.bin
exit $status
