#!/usr/bin/env bash
# F4-29 run.sh: build the RV64 D6 kernel (SBI timer, PLIC, HSM SMP, SBI IPIs) and boot it on
# QEMU virt with 1, 4 and 8 harts and on QEMU sifive_u (a model of a real board, D7's stand-in);
# then the two forensic builds: a handler that never completes the PLIC claim, and a hart
# list that does not skip the board's monitor core.
set -u -o pipefail
cd "$(dirname "$0")"
. ../F4-23/dlab.sh
status=0
SRC="../F4-28/boot.S ../F4-28/trapvec.S ../F4-28/trap.cc ../F4-28/sbi.cc uart_any.cc plic.cc smp.S d6_main.cc"

# 1. build (F4-26's TicketLock header on the include path)
kbuild riscv64 k_d6 "-I../F4-26" $SRC; rc=$?
riscv64-linux-gnu-nm k_d6.elf | grep -E ' (secondary_entry|secondary_main|kmain)$' > build.out 2>&1
rec build "dlab.sh kbuild; F4-28 boot.S trapvec.S trap.cc sbi.cc + uart_any.cc plic.cc smp.S d6_main.cc + F3-18 files" \
    "$RV_VER" "kbuild riscv64 k_d6 \"-I../F4-26\" $SRC" "$rc"
[ "$rc" = 0 ] || status=1

# typed input: one character every 0.3 s after the kernel has had 3 s to start. (8 harts get
# only 500 lock rounds: with more, the ticket lock on 8 vCPUs over 4 host CPUs took minutes.)
typist() { sleep 3; for c in h e l l o; do printf '%s' "$c"; sleep 0.3; done; printf '\n'; sleep 1; }
boot() {   # boot <name> <timeout> <kernel> <machine and options>
    local name="$1" t="$2" k="$3" m="$4" rc
    local q="qemu-system-riscv64 -M $m -m 256M -display none -monitor none -serial stdio -no-reboot"
    typist | timeout "$t" $q -kernel "$k.bin" > "$name.out" 2>&1; rc=${PIPESTATUS[1]}
    sed -i 's/\r$//' "$name.out"
    rec "$name" "d6_main.cc ($k)" "$QEMU_RV_VER; firmware: QEMU's bundled OpenSBI" \
        "<typist> | $q -kernel $k.bin" "$rc" "note:      input: 'hello' and a newline typed 0.3 s apart" "$HW_NOTE"
    return $rc
}
boot virt1 60 k_d6 "virt -smp 1"; grep -q '^D6 ok' virt1.out || status=1
boot virt4 60 k_d6 "virt -smp 4"; grep -q '^D6 ok' virt4.out || status=1
boot virt8 120 k_d6 "virt -smp 8 -append rounds=500"; grep -q '^D6 ok' virt8.out || status=1
# sifive_u: firmware cannot power the machine off here (the output shows the SRST error), so
# the run ends at the time limit; the pass line is still 'D6 ok'
boot sifive_u 20 k_d6 "sifive_u -smp 2"; grep -q '^D6 ok' sifive_u.out || status=1
echo "note:      sifive_u ends with exit code 124 (time limit): see the SBI system reset line" >> sifive_u.log

# 2. forensic: the interrupt handler forgets plic::complete
kbuild riscv64 k_nc "-I../F4-26 -DFORENSIC_NO_COMPLETE" $SRC || status=1
boot forensic_nocomplete 60 k_nc "virt -smp 2"
grep -q '^UART: 1 characters' forensic_nocomplete.out || status=1

# 3. a common mistake: starting every hart in the devicetree, including sifive_u's monitor core
kbuild riscv64 k_ah "-I../F4-26 -DFORENSIC_ALL_HARTS" $SRC || status=1
boot all_harts 20 k_ah "sifive_u -smp 2"
grep -q 'only 1 of 2 harts came online' all_harts.out || status=1
rm -f k_d6.elf k_d6.bin k_nc.elf k_nc.bin k_ah.elf k_ah.bin
exit $status
