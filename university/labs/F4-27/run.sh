#!/usr/bin/env bash
# F4-27 run.sh: build the D4 Raspberry Pi kernel and run it on QEMU's raspi3b model (QEMU 8.2
# has no Pi 4 or Pi 5 model): once with no devicetree (board table), once with pi3.dtb from
# make_dtb.py (alias + /soc ranges + spin table), each five times because one early run in
# this build hung (see the chapter); then the forensic build that ignores /soc's ranges.
# Real hardware: untested. k_d4.bin is what a Pi would load as kernel8.img (see config.txt).
set -u -o pipefail
cd "$(dirname "$0")"
. ../F4-23/dlab.sh
status=0
SRC="../F4-24/boot.S ../F4-24/vectors.S ../F4-24/trap.cc ../F4-24/pl011.cc ../F4-26/smp.S pi_smp.S pi_main.cc \
../F4-23/neutral_tests.cc"
QPI="qemu-system-aarch64 -M raspi3b -display none -monitor none -serial stdio -no-reboot"

# 1. build: same F4-24/F4-26 files as the virt kernel, a linker script based at 0x80000
kbuild aarch64 k_d4 "-I../F4-26" $SRC; rc=$?
{
    echo "== linker script difference from F4-24 =="
    diff ../F4-24/linker.ld linker.ld | grep -E '^[<>] +\. = '
    echo "== image header (first 64 bytes; text_offset at byte 8, image size at byte 16) =="
    od -A x -t x8 -N 64 k_d4.bin
    echo "== symbols =="
    aarch64-linux-gnu-nm k_d4.elf | grep -E ' (_start|pi_secondary_entry|secondary_entry|kmain)$'
    echo "== kernel8.img would be k_d4.bin: $(stat -c %s k_d4.bin) bytes =="
    echo "== sha256 of the F3-18 files compiled unchanged into this kernel =="
    reuse_record
} > build.out 2>&1
rec build "dlab.sh kbuild; F4-24 boot.S vectors.S trap.cc pl011.cc + F4-26 smp.S + pi_smp.S pi_main.cc + F3-18 files" \
    "$A64_VER" "kbuild aarch64 k_d4 \"-I../F4-26\" $SRC" "$rc"
[ "$rc" = 0 ] || status=1

# 2. the devicetree a Pi firmware would pass, written by make_dtb.py
python3 make_dtb.py pi3.dtb > dtb.out 2>&1; rc=$?
echo "== F4-23's checklist tool on it (note row 6) ==" >> dtb.out
g++ -std=c++20 -O2 -I../F4-23 ../F4-23/fdt_tool.cc -o .tool && ./.tool checklist pi3.dtb >> dtb.out 2>&1
rec dtb "make_dtb.py; F4-23 fdt_tool.cc checklist" "$(python3 --version); $HOST_VER" \
    "python3 make_dtb.py pi3.dtb; fdt_tool checklist pi3.dtb" "$rc"

# 3. boot on raspi3b, five times without and five times with the devicetree
boot5() {   # boot5 <name> <extra QEMU options>
    local name="$1" extra="$2" i rc ok=0 codes=""
    for i in 1 2 3 4 5; do
        timeout 20 $QPI $extra -kernel k_d4.bin < /dev/null > ".$name.$i" 2>&1; rc=$?
        codes="$codes $rc"
        grep -q '^D4 ok' ".$name.$i" && ok=$((ok + 1))
    done
    codes="${codes# }"
    tr -d '\r' < ".$name.1" > "$name.out"
    rec "$name" "pi_main.cc" "$QEMU_A64_VER" "$QPI $extra -kernel k_d4.bin   (5 runs)" "${codes%% *}" \
        "note:      exit codes of the five runs: $codes; 'D4 ok' in $ok of 5; $name.out is run 1" \
        "note:      exit code 0 comes from the watchdog reset with -no-reboot; pass is the line 'D4 ok'" "$HW_NOTE"
    rm -f ".$name".?
    [ "$ok" -ge 1 ]
}
boot5 nodt "" || status=1
boot5 withdt "-dtb pi3.dtb" || status=1

# 4. forensic: the console address taken from reg without /soc's ranges
kbuild aarch64 k_nr "-I../F4-26 -DFORENSIC_NO_RANGES" $SRC || status=1
timeout 10 $QPI -dtb pi3.dtb -kernel k_nr.bin < /dev/null > forensic_serial.out 2>&1; rc=$?
rec forensic_serial "pi_main.cc built with -DFORENSIC_NO_RANGES" "$QEMU_A64_VER" "$QPI -dtb pi3.dtb -kernel k_nr.bin" \
    "$rc" "note:      $(wc -c < forensic_serial.out) bytes of output; exit code 0 although the kernel failed" "$HW_NOTE"
[ -s forensic_serial.out ] && status=1
{
    echo "== qemu -d int (QEMU exits by itself: the panic path resets through the watchdog) =="
    timeout 10 ${QPI/-serial stdio/-serial none} -dtb pi3.dtb -kernel k_nr.bin -d int 2>&1 > /dev/null | head -n 12 > .int.txt
    cat .int.txt
    elr="$(grep -m 1 -o 'with ELR 0x[0-9a-f]*' .int.txt | awk '{print $3}')"
    echo "== addr2line -f -C -e k_nr.elf $elr =="
    aarch64-linux-gnu-addr2line -f -C -e k_nr.elf "$elr"
    echo "== the /soc part of pi3.dtb (fdt_tool dump) =="
    ./.tool dump pi3.dtb | sed -n '/^    soc {/,/^    };/p'
    rm -f .int.txt
} > forensic_int.out 2>&1
sed -i "s#$(pwd)/##g; s#$LABS/#labs/#g" forensic_int.out
rec forensic_int "k_nr.elf" "$QEMU_A64_VER; $(aarch64-linux-gnu-addr2line --version | head -n 1)" \
    "$QPI -dtb pi3.dtb -kernel k_nr.bin -serial none -d int | head -n 12; addr2line" "0" "$HW_NOTE"
rm -f k_d4.elf k_d4.bin k_nr.elf k_nr.bin pi3.dtb .tool
exit $status
