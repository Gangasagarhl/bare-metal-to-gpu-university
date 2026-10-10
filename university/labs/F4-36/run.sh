#!/usr/bin/env bash
# F4-36 run.sh: the SDHCI driver and the SD card protocol on QEMU's raspi3b machine, with an
# SDSC-sized (64 MiB) and an SDHC-sized (4 GiB) card image; host-side verification of what was
# written; a pin-mux mistake; the forensic evidence (data written to the wrong place).
set -u -o pipefail
cd "$(dirname "$0")"
. ../F4-31/lablib.sh
status=0
B=.build
rm -rf "$B"; mkdir -p "$B"
ok() { [ "$1" = "$2" ] || { echo "F4-36 step $3: exit $1, expected $2" >&2; status=1; }; }
RPI="qemu-system-aarch64 -M raspi3b -nographic -semihosting"
SRC="../F4-35/kmain.cc ../F4-35/dm.cc ../F4-35/drivers.cc ../F4-35/bcm2835_gpio.cc"

# 1. the kernel (the F4-35 kernel plus this chapter's driver) and the devicetree
{ kbuild $B/k.bin $B/o $SRC sdhci.cc && aarch64-linux-gnu-size $B/k.elf | sed "s#$B/##" &&
  python3 ../F4-31/minidtc.py raspi3b.dts $B/raspi3b.dtb | sed "s#$B/##"; } > kbuild.out 2>&1; rc=$?
rec kbuild "sdhci.cc raspi3b.dts + the F4-35 kernel files" "$XGXX_VER; $XLD_VER; $PY_VER" \
    "aarch64-linux-gnu-g++ $KFLAGS -fpie -c <file>; ld -pie -T ../F4-31/kernel.ld; objcopy -O binary; python3 ../F4-31/minidtc.py raspi3b.dts raspi3b.dtb" "$rc"
ok "$rc" 0 kbuild

# 2. two card images and two runs
for size in 64 4096; do
    name=sd_${size}m
    python3 mksd.py $B/$name.img $size > $B/mk.txt
    qrun 60 -- $RPI -kernel $B/k.bin -dtb $B/raspi3b.dtb -drive if=sd,format=raw,file=$B/$name.img > $name.out; rc=$?
    rec $name "k.bin with raspi3b.dtb; card image from mksd.py ($size MiB)" "$QEMU_VER; $PY_VER" \
        "python3 mksd.py card.img $size; $RPI -kernel k.bin -dtb raspi3b.dtb -drive if=sd,format=raw,file=card.img" "$rc" \
        "note:      exit code 0 = the kernel ended the run (semihosting; this machine has no PSCI node)" "$EMU_NOTE"
    ok "$rc" 0 $name
    python3 verify_image.py $B/$name.img 2048 32 > verify_${size}m.out; rc=$?
    rec verify_${size}m verify_image.py "$PY_VER" "python3 verify_image.py card.img 2048 32" "$rc"
    ok "$rc" 0 verify_${size}m
done
sed "s#$B/##" $B/mk.txt > mksd.out
rec mksd mksd.py "$PY_VER" "python3 mksd.py card.img 4096" "0"

# 2b. the same 64 MiB run with QEMU's own trace of the commands its SD controller model received
python3 mksd.py $B/t.img 64 > /dev/null
qrun 60 -- $RPI -kernel $B/k.bin -dtb $B/raspi3b.dtb -drive if=sd,format=raw,file=$B/t.img -trace 'sdhci_send_command' > $B/trace.txt; rc=$?
grep -a 'sdhci_send_command' $B/trace.txt | sed 's/^[0-9]*@[0-9.]*://' | head -n 14 > sd_trace.out
echo "[...] ($(grep -a -c 'sdhci_send_command' $B/trace.txt) commands in total; the rest are CMD24/CMD17 pairs of the self-test)" >> sd_trace.out
rec sd_trace "k.bin with raspi3b.dtb, 64 MiB card" "$QEMU_VER" "$RPI -kernel k.bin -dtb raspi3b.dtb -drive if=sd,... -trace sdhci_send_command (first 14 trace lines)" "$rc" "$EMU_NOTE"
ok "$rc" 0 sd_trace

# 3. common mistake: the pins routed to the other SD controller (function 4 instead of 7)
sed 's/dr403,sd-pins = <&gpio 48 6 7>;/dr403,sd-pins = <\&gpio 48 6 4>;/' raspi3b.dts > $B/pins.dts
python3 ../F4-31/minidtc.py $B/pins.dts $B/pins.dtb > /dev/null
python3 mksd.py $B/p.img 64 > /dev/null
qrun 60 -- $RPI -kernel $B/k.bin -dtb $B/pins.dtb -drive if=sd,format=raw,file=$B/p.img | grep -E 'pins|sdhci|present|mmc@' > pinmux.out; rc=$?
rec pinmux "k.bin with raspi3b.dts changed to dr403,sd-pins = <&gpio 48 6 4>" "$QEMU_VER" \
    "$RPI -kernel k.bin -dtb pins.dtb -drive if=sd,...; grep for the SD lines" "$rc" "$EMU_NOTE"

# 4. forensic evidence: a kernel build from a branch, 4 GiB card (see the answer key)
sed 's/    g_card.block_addressing = (ocr \& (1u << 30)) != 0;/    g_card.block_addressing = false;   \/\/ "simplified"/' sdhci.cc > .sdhci_branch.cc
kbuild $B/kf.bin $B/of $SRC .sdhci_branch.cc > $B/kf.txt 2>&1
python3 mksd.py $B/f.img 4096 > /dev/null
qrun 60 -- $RPI -kernel $B/kf.bin -dtb $B/raspi3b.dtb -drive if=sd,format=raw,file=$B/f.img | grep -E 'sdhci|ACMD41|CMD3|block 0|self-test' > forensic_kernel.out; rc=$?
rec forensic_kernel "the branch build of the kernel (evidence pack)" "$QEMU_VER" "$RPI -kernel k.bin -dtb raspi3b.dtb -drive if=sd,format=raw,file=card4g.img; grep for the SD lines" "$rc" "$EMU_NOTE"
python3 verify_image.py $B/f.img 2048 32 > forensic_verify.out; rc=$?
rec forensic_verify verify_image.py "$PY_VER" "python3 verify_image.py card4g.img 2048 32" "$rc" \
    "note:      exit code 1 = the check failed (this is the evidence)"
ok "$rc" 1 forensic_verify

rm -rf "$B" .sdhci_branch.cc
exit $status
