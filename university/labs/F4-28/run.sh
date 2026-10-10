#!/usr/bin/env bash
# F4-28 run.sh: build the RV64 D5 kernel, boot it on QEMU virt through OpenSBI, record the
# soft-float header mistake, and make the forensic evidence (a kernel linked at the address
# an M-mode tutorial uses, loaded where OpenSBI jumps).
set -u -o pipefail
cd "$(dirname "$0")"
. ../F4-23/dlab.sh
status=0
SRC="boot.S trapvec.S trap.cc uart16550.cc sbi.cc d5_main.cc ../F4-23/neutral_tests.cc"
QRV="qemu-system-riscv64 -M virt -m 256M -nographic -no-reboot"

# 1. build and inspect
kbuild riscv64 k_d5 "" $SRC; rc=$?
{
    echo "== ELF header and sections (readelf) =="
    riscv64-linux-gnu-readelf -h k_d5.elf | grep -E 'Machine|Entry|Flags'
    riscv64-linux-gnu-readelf -SW k_d5.elf | grep -E ' \.(text|rodata|data|bss) '
    echo "== symbols =="
    riscv64-linux-gnu-nm k_d5.elf | grep -E ' (_start|trap_vector|kmain|__global_pointer\$)$'
    echo "== sha256 of the F3-18 files compiled unchanged into this kernel =="
    reuse_record
} > build.out 2>&1
rec build "dlab.sh kbuild; boot.S trapvec.S trap.cc uart16550.cc sbi.cc d5_main.cc neutral_tests.cc + F3-18 files" \
    "$RV_VER" "kbuild riscv64 k_d5 \"\" $SRC" "$rc"
[ "$rc" = 0 ] || status=1

# 2. boot through OpenSBI (QEMU's default -bios) on the virt machine
timeout 30 $QRV -kernel k_d5.bin > d5.out 2>&1; rc=$?
rec d5 "d5_main.cc" "$QEMU_RV_VER; firmware: QEMU's bundled OpenSBI (banner in the output)" \
    "$QRV -kernel k_d5.bin" "$rc" "note:      exit code 0 after SBI system reset; pass is the line 'D5 ok'" "$HW_NOTE"
grep -q '^D5 ok' d5.out || status=1

# 3. a common mistake: the soft-float kernel ABI without the empty stubs header
riscv64-linux-gnu-g++ $KFLAGS_COMMON -march=rv64imac_zicsr_zifencei -mabi=lp64 -mcmodel=medany \
    -I. -I../F4-23 -I../F3-18 -c trap.cc -o .stubs.o > stubs.out 2>&1; rc=$?
sed -i "s#$(pwd)/##g" stubs.out
rec stubs "trap.cc compiled with -mabi=lp64 but without -I include (the empty gnu/stubs-lp64.h)" "$RV_VER" \
    "riscv64-linux-gnu-g++ <kernel flags> -march=rv64imac_zicsr_zifencei -mabi=lp64 -mcmodel=medany -c trap.cc" \
    "$rc" "result:    compile failed as expected (messages saved in stubs.out)"
rm -f .stubs.o
[ "$rc" != 0 ] || status=1

# 4. forensic: linked at 0x80000000 (the start of RAM, where an M-mode-only tutorial puts its
#    kernel), but loaded by QEMU at 0x80200000 because OpenSBI owns the start of RAM
sed 's/0x80200000;/0x80000000;/' linker.ld > .linker_bad.ld
KLDS=.linker_bad.ld kbuild riscv64 k_bad "" $SRC || status=1
timeout 8 $QRV -kernel k_bad.bin > forensic_serial.out 2>&1; rc=$?
rec forensic_serial "d5_main.cc linked with linker.ld changed to 0x80000000" "$QEMU_RV_VER" \
    "$QRV -kernel k_bad.bin" "$rc" "note:      124 = stopped by the 8 s time limit (no kernel output)" "$HW_NOTE"
[ "$rc" = 124 ] || status=1
{
    echo "== the linker script change =="
    diff linker.ld .linker_bad.ld
    echo "== readelf: entry point of k_bad.elf =="
    riscv64-linux-gnu-readelf -h k_bad.elf | grep Entry
    echo "== qemu -d int (first 9 lines; QEMU stopped when the pipe closed) =="
    timeout 8 $QRV -kernel k_bad.bin -serial none -monitor none -d int 2>&1 >/dev/null | head -n 9 > .int.txt
    sed 's/riscv_cpu_do_interrupt: //' .int.txt
    fetch="$(grep -m 1 'desc=fault_fetch' .int.txt | sed 's/.*tval:\(0x[0-9a-f]*\).*/\1/')"
    load="$(grep -m 1 'desc=fault_load' .int.txt | sed 's/.*epc:\(0x[0-9a-f]*\).*/\1/')"
    echo "== addr2line -f -C -e k_bad.elf $fetch (the address the fetch fault reports) =="
    riscv64-linux-gnu-addr2line -f -C -e k_bad.elf "$fetch"
    moved="$(printf '0x%x' $(( load - 0x200000 )))"
    echo "== addr2line -f -C -e k_bad.elf $moved (the load-fault pc $load minus 0x200000) =="
    riscv64-linux-gnu-addr2line -f -C -e k_bad.elf "$moved"
    echo "== nm k_bad.elf: the constructor list =="
    riscv64-linux-gnu-nm k_bad.elf | grep -E '__init_array_(start|end)'
    rm -f .int.txt
} > forensic_int.out 2>&1
sed -i "s#$(pwd)/##g; s#$LABS/#labs/#g" forensic_int.out
rec forensic_int "k_bad.elf" "$QEMU_RV_VER; $(riscv64-linux-gnu-addr2line --version | head -n 1)" \
    "$QRV -kernel k_bad.bin -d int | head -n 9; addr2line; nm" "0" "$HW_NOTE"
rm -f k_d5.elf k_d5.bin k_bad.elf k_bad.bin .linker_bad.ld
exit $status
