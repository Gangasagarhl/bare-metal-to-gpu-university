#!/usr/bin/env bash
# F4-32 run.sh: a four-stage Arm boot chain written for this course (ROM -> BL2 -> BL31 -> BL33)
# booted in QEMU's virt machine with the EL3 "secure" extension, plus the damaged-image test,
# an exception-level trace, and the forensic evidence (a monitor that hands over wrongly).
set -u -o pipefail
cd "$(dirname "$0")"
. ../F4-31/lablib.sh
status=0
B=.build
rm -rf "$B"; mkdir -p "$B"
ok() { [ "$1" = "$2" ] || { echo "F4-32 step $3: exit $1, expected $2" >&2; status=1; }; }
QM="qemu-system-aarch64 -M virt,secure=on -cpu cortex-a53 -m 256M -nographic -semihosting"

# 1. build the three firmware stages (fixed addresses) and the kernel (position independent)
stage() {  # stage NAME TEXT_BASE RAM_BASE extra-objects...
    local n="$1" tb="$2" rb="$3"
    shift 3
    $XGXX $KFLAGS -fno-pie $SCR -c "$n.cc" -o "$B/$n.o" &&
    $XLD -T stage.ld --defsym=TEXT_BASE="$tb" --defsym=RAM_BASE="$rb" -o "$B/$n.elf" \
        "$B/stage_start.o" "$B/kbase_fixed.o" "$B/$n.o" "$@" &&
    aarch64-linux-gnu-objcopy -O binary "$B/$n.elf" "$B/$n.bin"
}
build_all() {  # build_all OUTDIR-suffix
    $XGXX $KFLAGS -fno-pie -c stage_start.S -o $B/stage_start.o &&
    $XGXX $KFLAGS -fno-pie -c ../F4-31/kbase.cc -o $B/kbase_fixed.o &&
    $XGXX $KFLAGS -fno-pie -c bl31_vec.S -o $B/bl31_vec.o &&
    stage rom 0x0 0x0e0f0000 && stage bl2 0x0e000000 0 && stage bl31 0x0e100000 0 $B/bl31_vec.o &&
    kbuild $B/bl33.bin $B/k bl33.cc
}
SCR=""
{ build_all && aarch64-linux-gnu-size $B/rom.elf $B/bl2.elf $B/bl31.elf $B/bl33.elf | sed "s#$B/##"; } > build.out 2>&1; rc=$?
rec build "stage_start.S stage.ld rom.cc bl2.cc bl31.cc bl31_vec.S bl33.cc bootimg.h ../F4-31/start.S ../F4-31/kbase.cc ../F4-31/kernel.ld" \
    "$XGXX_VER; $XLD_VER" \
    "aarch64-linux-gnu-g++ $KFLAGS -fno-pie -c <stage>.cc; ld -T stage.ld --defsym=TEXT_BASE=... --defsym=RAM_BASE=...; objcopy -O binary (BL33: -fpie, ld -pie -T ../F4-31/kernel.ld)" "$rc"
ok "$rc" 0 build
IMGS="$B/bl2.bin:0x0e000000 $B/bl31.bin:0x0e100000 $B/bl33.bin:0x40080000"
python3 mkflash.py $B/flash.bin $B/rom.bin $IMGS > mkflash.out 2>&1; rc=$?
sed -i "s#$B/##g" mkflash.out
rec mkflash mkflash.py "$PY_VER" "python3 mkflash.py flash.bin rom.bin bl2.bin:0x0e000000 bl31.bin:0x0e100000 bl33.bin:0x40080000" "$rc"
ok "$rc" 0 mkflash

# 2. the whole chain
qrun 20 -- $QM -bios $B/flash.bin > boot_ok.out; rc=$?
rec boot_ok "flash.bin (all four stages)" "$QEMU_VER" "$QM -bios flash.bin" "$rc" \
    "note:      exit code 0 = BL31 handled the kernel's SYSTEM_OFF call and ended the run (semihosting)" "$EMU_NOTE"
ok "$rc" 0 boot_ok

# 3. the same boot with QEMU's exception log: every change of exception level, as QEMU saw it
qrun 20 -- $QM -bios $B/flash.bin -d int -D $B/int.txt > /dev/null; rc=$?
grep -a -E 'Exception return|Taking exception|from EL|to EL|ESR|ELR' $B/int.txt | sed 's/^ *//' | head -n 40 > boot_trace.out
rec boot_trace "flash.bin (all four stages)" "$QEMU_VER" "$QM -bios flash.bin -d int -D int.txt; grep for exception entries and returns" "$rc" \
    "note:      lines are QEMU's own interrupt/exception log, filtered with grep, first 40 kept" "$EMU_NOTE"
ok "$rc" 0 boot_trace

# 4. a damaged image: one bit of BL33 flipped after its CRC-32 was computed
python3 mkflash.py $B/flash_bad.bin $B/rom.bin $IMGS --corrupt BL33 > corrupt_mkflash.out 2>&1; rc=$?
sed -i "s#$B/##g" corrupt_mkflash.out
rec corrupt_mkflash mkflash.py "$PY_VER" "python3 mkflash.py flash_bad.bin rom.bin <images> --corrupt BL33" "$rc"
qrun 20 -- $QM -bios $B/flash_bad.bin > corrupt.out; rc=$?
rec corrupt "flash_bad.bin (BL33 payload with one bit flipped)" "$QEMU_VER; $PY_VER" \
    "python3 mkflash.py flash_bad.bin ... --corrupt BL33; $QM -bios flash_bad.bin" "$rc" \
    "note:      exit code 6 = BL2 refused the image and stopped the run (expected)" "$EMU_NOTE"
ok "$rc" 6 corrupt

# 5. common mistake: the kernel takes the first PL011 in the tree without looking at "status"
sed 's/ && t.available(n)//' bl33.cc > .bl33_nostatus.cc
{ kbuild $B/bl33_ns.bin $B/kns .bl33_nostatus.cc &&
  python3 mkflash.py $B/flash_ns.bin $B/rom.bin $B/bl2.bin:0x0e000000 $B/bl31.bin:0x0e100000 $B/bl33_ns.bin:0x40080000 > /dev/null; } > $B/ns.txt 2>&1
qrun 8 -- $QM -bios $B/flash_ns.bin -d int -D $B/nsint.txt > nostatus.out; rc=$?
{ echo "== QEMU exception log, the first exception taken at EL1 =="
  grep -a -A4 'Taking exception 4' $B/nsint.txt | head -n 5 | sed 's/^ *//'
  echo "== exceptions taken in 8 s, by kind =="
  grep -a 'Taking exception' $B/nsint.txt | sed 's/ on CPU.*//' | sort | uniq -c; } >> nostatus.out
rec nostatus "bl33.cc with the status check removed (sed 's/ && t.available(n)//')" "$QEMU_VER" "$QM -bios flash_ns.bin -d int -D int.txt; then grep/uniq on int.txt" "$rc" \
    "note:      exit code 124 = still running after 8 s (no kernel output); kept for 'Common mistakes'" "$EMU_NOTE"
ok "$rc" 124 nostatus

# 6. forensic evidence: BL31 built with a different SCR_EL3 value (see the answer key)
SCR="-DSCR_VALUE=0x31"
{ build_all && python3 mkflash.py $B/flash_f.bin $B/rom.bin $IMGS > /dev/null; } > $B/f.txt 2>&1
qrun 8 -- $QM -bios $B/flash_f.bin -d int -D $B/fint.txt > forensic_serial.out; rc=$?
rec forensic_serial "flash_f.bin (the build from the evidence pack)" "$QEMU_VER" "$QM -bios flash_f.bin" "$rc" \
    "note:      exit code 7 = BL31's handler for an unexpected exception stopped the run" "$EMU_NOTE"
ok "$rc" 7 forensic_serial
grep -a -E 'Exception return|Taking exception|ESR|ELR|from EL|to EL' $B/fint.txt | sed 's/^ *//' | uniq -c | head -n 14 > forensic_int.out
rec forensic_int "QEMU exception log of the same run" "$QEMU_VER" "$QM -bios flash_f.bin -d int -D int.txt; grep; uniq -c (repeats counted); first 14 lines" "0" "$EMU_NOTE"

rm -rf "$B" .bl33_nostatus.cc
exit $status
