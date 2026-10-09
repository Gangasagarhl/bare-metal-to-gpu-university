#!/usr/bin/env bash
# F4-23 run.sh: answer the platform checklist for three QEMU machines from the machines
# themselves (devicetree blobs dumped by QEMU, the x86 memory map from the QEMU monitor),
# show how machine options change the answers, and make the forensic evidence (a RISC-V
# kernel with a hard-coded UART address, booted on a second board).
set -u -o pipefail
cd "$(dirname "$0")"
. ./dlab.sh
status=0
HOSTFLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
QX86_VER="$(qemu-system-x86_64 --version | head -n 1)"

# 1. the host tool
g++ $HOSTFLAGS fdt_tool.cc -o .fdt_tool > tool.out 2>&1; rc=$?
echo "(no compiler messages)" >> tool.out
rec tool "fdt_tool.cc + fdt.h" "$HOST_VER" "g++ $HOSTFLAGS fdt_tool.cc -o fdt_tool" "$rc"
[ "$rc" = 0 ] || exit 1

# 2. AArch64 virt: GICv3, 4 CPUs, 256 MiB
A_OPTS="-M virt,gic-version=3,dumpdtb=.virt_a64.dtb -cpu cortex-a57 -smp 4 -m 256M -nographic"
timeout 20 qemu-system-aarch64 $A_OPTS > .q.txt 2>&1; rc=$?
./.fdt_tool checklist .virt_a64.dtb > checklist_aarch64.out 2>&1 || status=1
rec checklist_aarch64 "fdt_tool.cc" "$QEMU_A64_VER; $HOST_VER" \
    "qemu-system-aarch64 $A_OPTS; fdt_tool checklist virt_a64.dtb" "$rc" "$HW_NOTE"
./.fdt_tool dump .virt_a64.dtb > dump_aarch64.out 2>&1 || status=1
rec dump_aarch64 "fdt_tool.cc" "$QEMU_A64_VER; $HOST_VER" "fdt_tool dump virt_a64.dtb" "0" "$HW_NOTE"

# 3. RISC-V virt: 4 harts, 256 MiB
R_OPTS="-M virt,dumpdtb=.virt_rv.dtb -smp 4 -m 256M -nographic"
timeout 20 qemu-system-riscv64 $R_OPTS > .q.txt 2>&1; rc=$?
./.fdt_tool checklist .virt_rv.dtb > checklist_riscv64.out 2>&1 || status=1
rec checklist_riscv64 "fdt_tool.cc" "$QEMU_RV_VER; $HOST_VER" \
    "qemu-system-riscv64 $R_OPTS; fdt_tool checklist virt_rv.dtb" "$rc" "$HW_NOTE"
./.fdt_tool dump .virt_rv.dtb > dump_riscv64.out 2>&1 || status=1
rec dump_riscv64 "fdt_tool.cc" "$QEMU_RV_VER; $HOST_VER" "fdt_tool dump virt_rv.dtb" "0" "$HW_NOTE"

# 4. x86-64 q35: no devicetree; the QEMU monitor shows the memory map the firmware will
#    describe to the kernel through E820/UEFI and ACPI (OS301, OS302)
X_OPTS="-M q35 -m 256M -smp 4 -S -display none -nodefaults -monitor stdio"
printf 'info mtree -f\ninfo cpus\nquit\n' | timeout 20 qemu-system-x86_64 $X_OPTS > .q.txt 2>&1; rc=$?
{
    echo "== info mtree -f: the flat view of address space \"memory\" (the CPU's view) =="
    tr -d '\r' < .q.txt | awk '/^FlatView #/{blk=""} {blk=blk $0 "\n"} /^ AS "memory"/{want=1} \
        /^$/{if (want) {printf "%s", blk; exit} }' | grep -E '\(prio [0-9]+, (ram|rom|i/o)\)'
    echo "== info cpus =="
    tr -d '\r' < .q.txt | grep -E '^\*? *CPU #'
} > checklist_x86.out
rec checklist_x86 "QEMU monitor commands info mtree -f, info cpus" "$QX86_VER" \
    "printf 'info mtree -f\\ninfo cpus\\nquit\\n' | qemu-system-x86_64 $X_OPTS" "$rc" "$HW_NOTE"

# 5. machine options change the answers: GICv2 instead of GICv3; AIA instead of the PLIC
{
    timeout 20 qemu-system-aarch64 -M virt,gic-version=2,dumpdtb=.v2.dtb -cpu cortex-a57 -m 256M -nographic > .q.txt 2>&1
    echo "== qemu-system-aarch64 -M virt,gic-version=2 =="
    ./.fdt_tool checklist .v2.dtb | grep -E '^row (4|5)'
    timeout 20 qemu-system-riscv64 -M virt,aia=aplic-imsic,dumpdtb=.aia.dtb -smp 2 -m 256M -nographic > .q.txt 2>&1
    echo "== qemu-system-riscv64 -M virt,aia=aplic-imsic -smp 2 =="
    ./.fdt_tool checklist .aia.dtb | grep -E '^row 4'
    timeout 20 qemu-system-riscv64 -M sifive_u,dumpdtb=.su.dtb -smp 2 -m 256M -nographic > .q.txt 2>&1
    echo "== qemu-system-riscv64 -M sifive_u -smp 2 (a model of a real board) =="
    ./.fdt_tool checklist .su.dtb
} > variants.out 2>&1
rec variants "fdt_tool.cc" "$QEMU_A64_VER; $QEMU_RV_VER" \
    "dumpdtb for virt,gic-version=2 (aarch64), virt,aia=aplic-imsic and sifive_u (riscv64); fdt_tool checklist" "0" "$HW_NOTE"
grep -q 'arm,cortex-a15-gic' variants.out || status=1

# 6. forensic: the D5 kernel (F4-28) built with the UART address written as a constant,
#    booted on virt (where the constant is right) and on sifive_u (where it is not)
RVSRC="../F4-28/boot.S ../F4-28/trapvec.S ../F4-28/trap.cc ../F4-28/uart16550.cc ../F4-28/sbi.cc ../F4-28/d5_main.cc neutral_tests.cc"
kbuild riscv64 k_fixed "-DFORENSIC_FIXED_UART" $RVSRC || status=1
timeout 20 qemu-system-riscv64 -M virt -m 256M -nographic -no-reboot -kernel k_fixed.bin > .virt.txt 2>&1; rc_virt=$?
SU="qemu-system-riscv64 -M sifive_u -smp 2 -m 256M -nographic -no-reboot"
timeout 10 $SU -kernel k_fixed.bin > forensic_serial.out 2>&1; rc=$?
rec forensic_serial "d5_main.cc built with -DFORENSIC_FIXED_UART" "$QEMU_RV_VER" "$SU -kernel k_fixed.bin" "$rc" \
    "note:      124 = stopped by the 10 s time limit; the same image on -M virt exited $rc_virt and printed $(grep -c . .virt.txt) lines" "$HW_NOTE"
[ "$rc" = 124 ] || status=1
{
    echo "== the same image on -M virt: last 2 lines =="
    tail -n 2 .virt.txt
    echo "== qemu -M sifive_u -d int: traps whose tval is 0x10000005 (first 3) and how many in 3 s =="
    timeout 3 $SU -kernel k_fixed.bin -serial none -monitor none -d int 2>&1 >/dev/null \
        | grep 'tval:0x0000000010000005' > .int.txt
    head -n 3 .int.txt | sed 's/riscv_cpu_do_interrupt: //'
    echo "count: $(wc -l < .int.txt)"
    epc="$(head -n 1 .int.txt | sed 's/.*epc:\(0x[0-9a-f]*\).*/\1/')"
    echo "== addr2line -f -C -e k_fixed.elf $epc =="
    riscv64-linux-gnu-addr2line -f -C -e k_fixed.elf "$epc"
} > forensic_int.out 2>&1
sed -i "s#$(pwd)/##g; s#$LABS/#labs/#g" forensic_int.out
rec forensic_int "k_fixed.elf" "$QEMU_RV_VER; $(riscv64-linux-gnu-addr2line --version | head -n 1)" \
    "$SU -kernel k_fixed.bin -d int | grep tval:0x...10000005; addr2line" "0" "$HW_NOTE"
rm -f .fdt_tool .q.txt .virt.txt .int.txt .virt_a64.dtb .virt_rv.dtb .v2.dtb .aia.dtb .su.dtb k_fixed.elf k_fixed.bin
exit $status
