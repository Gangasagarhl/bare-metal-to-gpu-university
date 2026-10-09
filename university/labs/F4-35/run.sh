#!/usr/bin/env bash
# F4-35 run.sh: (1) the DR403 kernel with its devicetree-driven driver model on QEMU virt
# (milestone D8, QEMU acceptance tests 1 and 3); (2) the simulated SoC with clocks, resets and
# pins on the host; (3) the forensic evidence "device reads all zeros".
set -u -o pipefail
cd "$(dirname "$0")"
. ../F4-31/lablib.sh
status=0
B=.build
rm -rf "$B"; mkdir -p "$B"
ok() { [ "$1" = "$2" ] || { echo "F4-35 step $3: exit $1, expected $2" >&2; status=1; }; }
VIRT="qemu-system-aarch64 -M virt -cpu cortex-a53 -m 256M -nographic -semihosting -device virtio-rng-device"

# 1. the kernel
kbuild $B/k.bin $B/o kmain.cc dm.cc drivers.cc bcm2835_gpio.cc > kbuild.out 2>&1; rc=$?
{ echo "build exit code: $rc"; aarch64-linux-gnu-size $B/k.elf | sed "s#$B/##";
  echo "driver descriptors in the image (section dr403_drivers -> .drivers):";
  aarch64-linux-gnu-nm -C $B/k.elf | grep '_driver$' | sed 's/^.*::/  /' | sort; } >> kbuild.out
rec kbuild "kmain.cc dm.cc dm.h drivers.cc bcm2835_gpio.cc + ../F4-31/{start.S,kbase.cc,kernel.ld}" "$XGXX_VER; $XLD_VER" \
    "aarch64-linux-gnu-g++ $KFLAGS -fpie -c <file>; ld -pie --no-dynamic-linker -T kernel.ld; objcopy -O binary" "$rc"
ok "$rc" 0 kbuild
qrun 30 -- $VIRT -kernel $B/k.bin > d8_virt.out; rc=$?
sed -i 's/RTCDR [0-9]* then [0-9]*/RTCDR <t> then <t+1>/' d8_virt.out
rec d8_virt "the kernel image k.bin" "$QEMU_VER" "$VIRT -kernel k.bin" "$rc" \
    "note:      exit code 0 = the kernel powered the machine off through PSCI SYSTEM_OFF" \
    "note:      the two RTC readings (seconds since 1970, different on every run) are replaced by <t> and <t+1>; the check on them is the printed word 'ticking'" "$EMU_NOTE"
ok "$rc" 0 d8_virt
python3 d8check.py d8_virt.out > d8_check.out; rc=$?
rec d8_check d8check.py "$PY_VER" "python3 d8check.py d8_virt.out" "$rc"
ok "$rc" 0 d8_check

# 2. the simulated SoC: the tree, the probe log, the register dump
hostbuild $B/socsim socsim.cc > $B/sb.txt 2>&1; rc=$?
{ cat $B/sb.txt; echo "build of socsim.cc: exit $rc (no messages above means no warnings)"; } > socsim_build.out
rec socsim_build "socsim.cc ../F4-31/fdt.h" "$GXX_VER" "g++ $HOSTFLAGS socsim.cc -o socsim" "$rc"
ok "$rc" 0 socsim_build
python3 ../F4-31/minidtc.py soc.dts $B/soc.dtb > minidtc_soc.out 2>&1; rc=$?
sed -i "s#$B/##g" minidtc_soc.out
rec minidtc_soc ../F4-31/minidtc.py "$PY_VER" "python3 ../F4-31/minidtc.py soc.dts soc.dtb" "$rc"
$B/socsim $B/soc.dtb > socsim_ok.out 2>&1; rc=$?
rec socsim_ok "socsim.cc with soc.dtb" "$GXX_VER" "./socsim soc.dtb" "$rc" "note:      simulated SoC (this course's model), not real hardware"
ok "$rc" 0 socsim_ok

# 3. forensic evidence: the same program, the devicetree shipped with the "new board revision"
sed 's/clocks = <&ccu 2>;/clocks = <\&ccu 3>;/' soc.dts > board_rev_b.dts
python3 ../F4-31/minidtc.py board_rev_b.dts $B/rev_b.dtb > /dev/null
$B/socsim $B/rev_b.dtb > forensic_tsens.out 2>&1; rc=$?
rec forensic_tsens "socsim.cc with board_rev_b.dtb (the evidence pack's tree)" "$GXX_VER" "./socsim board_rev_b.dtb" "$rc" \
    "note:      simulated SoC (this course's model), not real hardware"
ok "$rc" 0 forensic_tsens
grep -n -A6 'tsens@10003000 {' board_rev_b.dts > forensic_dts.out
rec forensic_dts board_rev_b.dts "$(grep --version | head -n 1)" "grep -n -A6 'tsens@10003000 {' board_rev_b.dts" "0"

rm -rf "$B"
exit $status
