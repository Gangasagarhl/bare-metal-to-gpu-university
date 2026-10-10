#!/usr/bin/env bash
# F9-67 run.sh: resolve the configuration, build the robot image (kernel, devicetree, config),
# pack it into a GPT disk image, check it, boot it in QEMU, measure boot stages over five boots,
# and record the forensic evidence (an image that refuses to arm).
set -u -o pipefail
cd "$(dirname "$0")"
. ./rblib.sh
status=0
B=.build
rm -rf "$B"; mkdir -p "$B"
ok() { [ "$1" = "$2" ] || { echo "F9-67 step $3: exit $1, expected $2" >&2; status=1; }; }

# 1. configuration (Listing 1) and a variant that shows a dropped option
python3 mkconfig.py robot.config $B/.config $B/robot_config.h > mkconfig.out 2>&1; rc=$?
rec mkconfig "mkconfig.py robot.config" "$PY_VER" "python3 mkconfig.py robot.config .config robot_config.h" "$rc"
ok "$rc" 0 mkconfig
mkdir -p $B/nortc
python3 mkconfig.py robot_nortc.config $B/nortc/.config $B/nortc/robot_config.h > mkconfig_drop.out 2>&1; rc=$?
rec mkconfig_drop "mkconfig.py robot_nortc.config" "$PY_VER" "python3 mkconfig.py robot_nortc.config .config robot_config.h" "$rc"
ok "$rc" 0 mkconfig_drop

# 2. the kernel image (Listing 2) and the devicetree (Listing 3)
kbuild $B/Image $B/o $B robot.cc > kbuild.out 2>&1; rc=$?
{ echo "build exit code: $rc"; aarch64-linux-gnu-size $B/o/image.elf | sed "s#$B/o/##";
  echo "Image: $(stat -c %s $B/Image) bytes"; } >> kbuild.out
rec kbuild "start.S kbase.cc kbase.h fdt.h robot.cc kernel.ld (+ generated robot_config.h)" "$XGXX_VER; $XLD_VER" \
    "aarch64-linux-gnu-g++ $KFLAGS -fpie -c <file>; ld -pie --no-dynamic-linker -T kernel.ld; objcopy -O binary" "$rc"
ok "$rc" 0 kbuild
python3 minidtc.py robot.dts $B/robot.dtb 2>&1 | sed "s#$B/##" > dtc.out; rc=${PIPESTATUS[0]}
rec dtc "robot.dts" "$PY_VER (minidtc.py, the course's own DTS compiler)" "python3 minidtc.py robot.dts robot.dtb" "$rc"
ok "$rc" 0 dtc

# 2b. reproducibility: build the kernel image a second time in another folder and compare
kbuild $B/Image2 $B/o2 $B robot.cc > /dev/null 2>&1
{ (cd $B && sha256sum Image Image2) | sed "s#$B/##"
  if cmp -s $B/Image $B/Image2; then echo "identical: yes (same inputs, same bytes)"; else echo "identical: NO"; fi; } > repro.out
rec repro "robot image built twice" "$XGXX_VER; $XLD_VER" "kbuild twice into separate folders; sha256sum; cmp" "0"

# 3. pack (Listing 4), then look inside the disk image the way a learner would
./mkimage.sh $B/robot-disk.img $B/Image $B/robot.dtb $B/.config > $B/mk.txt 2>&1; rc=$?
{ sed "s#$B/##" $B/mk.txt
  echo "--- sgdisk -p robot-disk.img (partition table)"
  sgdisk -p $B/robot-disk.img | sed -n '/^Number/,$p'
  echo "--- mdir -i robot-disk.img@@1M :: (files in partition 1)"
  mdir -i $B/robot-disk.img@@1M :: | grep -E 'Image|IMAGE|robot|manifest|files'
  echo "--- manifest.txt"
  mtype -i $B/robot-disk.img@@1M ::manifest.txt; } > mkimage.out 2>&1
rec mkimage "mkimage.sh" "$(sgdisk --version | head -n 1); dosfstools $(dpkg-query -W -f='${Version}' dosfstools) (mkfs.vfat); $(mtools --version | head -n 1)" \
    "./mkimage.sh robot-disk.img Image robot.dtb .config" "$rc"
ok "$rc" 0 mkimage

# 4. what a boot loader would do: take the files out of partition 1, check them, boot them
mkdir -p $B/boot
for f in Image robot.dtb robot.cfg manifest.txt; do mcopy -n -i $B/robot-disk.img@@1M "::$f" $B/boot/; done
{ echo "--- sha256sum -c manifest.txt (files read back from the disk image)"
  (cd $B/boot && sha256sum -c manifest.txt)
  echo "--- serial console of the boot"; } > boot.out 2>&1; crc=$?
t_start=$(date +%s%N)
qrun 20 -- $ROBOT_VM -nographic -semihosting -kernel $B/boot/Image -dtb $B/boot/robot.dtb \
    -append "robot.cycles=500" >> boot.out; rc=$?
t_end=$(date +%s%N)
echo "--- host: QEMU process ran for $(( (t_end - t_start) / 1000000 )) ms of wall-clock time" >> boot.out
rec boot "robot-disk.img (files from partition 1)" "$QEMU_VER" \
    "$ROBOT_VM -nographic -semihosting -kernel Image -dtb robot.dtb -append \"robot.cycles=500\"" "$rc" \
    "note:      exit code 0 = the image ended the run itself through semihosting; checksum check exit $crc" "$EMU_NOTE"
ok "$rc" 0 boot; ok "$crc" 0 checksums

# 5. boot-time measurement: five boots, the stage time stamps of each (emulated time)
{ printf '%-5s %12s %12s %12s %12s %10s\n' boot console_us drivers_us estop_us armed_us host_ms
  for i in 1 2 3 4 5; do
      t0=$(date +%s%N)
      qrun 20 -- $ROBOT_VM -nographic -semihosting -kernel $B/boot/Image -dtb $B/boot/robot.dtb \
          -append "robot.cycles=1" > $B/bt.txt
      t1=$(date +%s%N)
      s() { sed -n "s/^\[ *\([0-9]*\) us\] $1.*/\1/p" $B/bt.txt; }
      printf '%-5s %12s %12s %12s %12s %10s\n' $i "$(s console)" "$(s drivers)" "$(s e-stop)" "$(s ARMED)" $(( (t1 - t0) / 1000000 ))
  done; } > boottime.out
rec boottime "robot image, booted five times" "$QEMU_VER" \
    "$ROBOT_VM -nographic -semihosting -kernel Image -dtb robot.dtb -append \"robot.cycles=1\" (x5)" "0" \
    "note:      stage times are counter values printed by the image (emulated time since reset); host_ms is the wall-clock time of the whole QEMU process" "$EMU_NOTE"

# 6. forensic evidence: the release-2 devicetree
python3 minidtc.py robot_forensic.dts $B/forensic.dtb > /dev/null
qrun 20 -- $ROBOT_VM -nographic -semihosting -kernel $B/Image -dtb $B/forensic.dtb \
    -append "robot.cycles=500" > forensic_boot.out; rc=$?
rec forensic_boot "the release-2 image from the evidence pack" "$QEMU_VER" \
    "$ROBOT_VM -nographic -semihosting -kernel Image -dtb robot-release2.dtb -append \"robot.cycles=500\"" "$rc" \
    "note:      exit code 6 = the image refused to arm (expected for this evidence)" "$EMU_NOTE"
ok "$rc" 6 forensic_boot
diff -u --label robot.dts --label robot_forensic.dts robot.dts robot_forensic.dts > forensic_diff.out; rc=$?
rec forensic_diff "robot.dts robot_forensic.dts" "$(diff --version | head -n 1)" "diff -u robot.dts robot_forensic.dts" "$rc" \
    "note:      exit code 1 = the files differ (expected)"
ok "$rc" 1 forensic_diff

rm -rf "$B"
exit $status
