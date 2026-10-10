#!/usr/bin/env bash
# BR-07 run.sh: the PC-to-SoC-board bridge, everything that needs more than one .cpp file.
#   known_good     - boot a known-good image (OpenSBI, shipped with QEMU) on the board model sifive_u
#   bootrom        - the board model's first code: its ROM words and the first instructions run
#   board_dump     - the board's devicetree (QEMU dumpdtb), read with F4-23's fdt_tool (DR402)
#   board_checklist- F4-23's nine-row platform checklist on the same blob
#   board_mtree    - QEMU's own memory map of the board model (an independent view)
#   deps_board     - bringup_deps.cc on the board's tree
#   deps_teach     - bringup_deps.cc on teach.dtb (an imaginary board with clocks, resets, pins,
#                    a regulator and power domains, written by teach_board.py)
#   deps_virt      - bringup_deps.cc on QEMU's Arm virt tree (dma-coherent everywhere)
#   deps_pi3       - bringup_deps.cc on F4-27's Raspberry Pi-shaped tree (ranges translation)
#   translate_build, translate_demo, translate_addr2line
#                  - trap 2 on QEMU raspi3b: a bus address used as a CPU address
#   flash_ok, flash_bad, flash_bad_trace, flash_recover
#                  - trap 3 on QEMU Arm virt: the boot flash overwritten without, then with,
#                    a recovery path (EDK2 firmware shipped with the distribution)
#   forensic_build, forensic_virt, forensic_board, forensic_errors
#                  - the forensic lab's evidence: a kernel with a hard-coded console address
# Every board here is a QEMU model: untested on hardware.
set -u -o pipefail
cd "$(dirname "$0")"
status=0
B=.build
rm -rf "$B"; mkdir -p "$B"

GXX_VER="$(g++ --version | head -n 1)"
XGXX_VER="$(aarch64-linux-gnu-g++ --version | head -n 1); $(aarch64-linux-gnu-ld --version | head -n 1)"
QRV="$(qemu-system-riscv64 --version | head -n 1)"
QA64="$(qemu-system-aarch64 --version | head -n 1)"
PY_VER="$(python3 --version)"
HW="hardware:  untested on hardware; QEMU 8.2.2 machine model with TCG, not a real board"
HOSTFLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
KFLAGS="-std=c++20 -ffreestanding -fno-exceptions -fno-rtti -fno-threadsafe-statics -fno-stack-protector \
-fno-pie -fno-pic -O2 -g -Wall -Wextra -Wpedantic -Werror -mgeneral-regs-only -mstrict-align -mno-outline-atomics"

rec() {   # rec NAME LISTING TOOLCHAIN COMMAND EXIT [extra lines...]
    local name="$1" listing="$2" tool="$3" cmd="$4" rc="$5"
    shift 5
    {
        echo "listing:   $listing"
        echo "toolchain: $tool"
        echo "command:   $cmd"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
        echo "exit code: $rc"
        for extra in "$@"; do echo "$extra"; done
    } > "$name.log"
}
want() {  # want STEP CONDITION-DESCRIPTION ; reads the condition's status from $?
    if [ "$1" != 0 ]; then echo "BR-07 step $2: check failed" >&2; status=1; fi
}
clean_serial() { tr -d '\r\000' | sed 's/\x1b\[[0-9;=]*[A-Za-z]//g'; }
hmp() {   # hmp QEMU ARGS... : monitor commands from stdin, answers only
    local q="$1"; shift
    timeout 30 "$q" "$@" -display none -monitor stdio -serial null -S 2>&1 |
        clean_serial | grep -a -v '^(qemu)' | grep -a -v '^QEMU .* monitor'
}

BOARD="qemu-system-riscv64 -M sifive_u -m 256M"

# 1. known-good image on the board: OpenSBI as shipped with QEMU (-bios default). It prints what
#    it learned about the platform and then hands over to a next stage that does not exist here,
#    so the run ends at the time limit: 124 is the expected exit code; the pass is the banner.
timeout 5 $BOARD -display none -monitor none -serial stdio -bios default -no-reboot < /dev/null 2>&1 |
    clean_serial | grep -a -v 'terminating on signal' > known_good.out
rc=${PIPESTATUS[0]}
rec known_good "OpenSBI firmware shipped with QEMU (-bios default)" "$QRV" \
    "timeout 5 $BOARD -display none -monitor none -serial stdio -bios default -no-reboot" "$rc" \
    "note:      124 = stopped by the 5 s limit (expected: nothing follows the firmware); pass = the 'Platform Name' line" \
    "note:      carriage returns, NUL bytes and terminal escape codes removed; the boot hart number may change between runs" "$HW"
grep -q '^Platform Name *: SiFive HiFive Unleashed A00' known_good.out; want $? known_good

# 2. the first code: QEMU's ROM for this board, before anything runs, then the first blocks executed
{
    echo "== ROM words at 0x1000 (monitor: xp /12xw 0x1000, machine stopped before the first instruction) =="
    printf 'xp /12xw 0x1000\nquit\n' | hmp qemu-system-riscv64 -M sifive_u -m 256M -bios default | grep -a '^0000'
    echo "== first three translated blocks (-d in_asm) =="
    timeout 2 $BOARD -display none -monitor none -serial null -bios default -d in_asm -D $B/asm.txt < /dev/null > /dev/null 2>&1
    awk '/^0x/ {print} /^0x0000000080000000/ {n++} n && /^$/ {exit}' $B/asm.txt | awk '!seen[$0]++' | head -n 12
    echo "== QEMU's own description of the board model's boot-source option (-M sifive_u,help) =="
    qemu-system-riscv64 -M sifive_u,help 2>&1 | grep -a 'start-in-flash' | sed 's/^ *//'
} > bootrom.out 2>&1
rec bootrom "QEMU monitor and -d in_asm" "$QRV" \
    "$BOARD -bios default -S -monitor stdio: xp /12xw 0x1000; then -d in_asm for 2 s (first blocks, duplicates removed); then -M sifive_u,help" "0" "$HW"
grep -q '0x80000000:' bootrom.out; want $? bootrom

# 3. dump the board's devicetree and read it with DR402's tool (F4-23 fdt_tool.cc, unchanged)
$BOARD -machine dumpdtb=$B/board.dtb -display none > /dev/null 2>&1
g++ $HOSTFLAGS -I../F4-23 ../F4-23/fdt_tool.cc -o $B/fdt_tool; rc=$?
want $rc fdt_tool_build
$B/fdt_tool dump $B/board.dtb > board_dump.out 2>&1; rc=$?
rec board_dump "F4-23 fdt_tool.cc and fdt.h (DR402), unchanged" "$GXX_VER; $QRV" \
    "$BOARD -machine dumpdtb=board.dtb; g++ $HOSTFLAGS -I../F4-23 ../F4-23/fdt_tool.cc -o fdt_tool; ./fdt_tool dump board.dtb" "$rc" \
    "note:      board.dtb: $(stat -c %s $B/board.dtb) bytes, sha256 $(sha256sum $B/board.dtb | cut -c1-16)..." "$HW"
want $rc board_dump
$B/fdt_tool checklist $B/board.dtb > board_checklist.out 2>&1; rc=$?
rec board_checklist "F4-23 fdt_tool.cc (DR402), unchanged" "$GXX_VER" "./fdt_tool checklist board.dtb" "$rc"
want $rc board_checklist
printf 'info mtree -f\nquit\n' | hmp qemu-system-riscv64 -M sifive_u -m 256M -bios default |
    awk '/AS "memory"/{k=1} /^FlatView #2/{k=0} k' | grep -a -E '^\s+0' | sed 's/^ *//' > board_mtree.out
rec board_mtree "QEMU monitor" "$QRV" "$BOARD -bios default -S -monitor stdio; (qemu) info mtree -f (the CPU's flat view only)" "0" "$HW"
grep -q 'riscv.sifive.u.prci' board_mtree.out; want $? board_mtree

# 4. the dependency inventory (new in this bridge) on four trees
g++ $HOSTFLAGS bringup_deps.cc -o $B/bringup_deps > $B/deps_build.txt 2>&1; rc=$?
want $rc deps_build
$B/bringup_deps $B/board.dtb > deps_board.out 2>&1; rc=$?
rec deps_board "bringup_deps.cc (with F4-23's fdt.h)" "$GXX_VER" "g++ $HOSTFLAGS bringup_deps.cc -o bringup_deps; ./bringup_deps board.dtb" "$rc"
want $rc deps_board
python3 teach_board.py $B/teach.dtb > $B/teach.txt 2>&1
$B/bringup_deps $B/teach.dtb > deps_teach.out 2>&1; rc=$?
rec deps_teach "teach_board.py; bringup_deps.cc" "$PY_VER; $GXX_VER" "python3 teach_board.py teach.dtb; ./bringup_deps teach.dtb" "$rc" \
    "note:      teach.dtb describes an imaginary board; $(sed 's/^wrote [^:]*: \([0-9]*\) bytes.*/\1 bytes/' $B/teach.txt)"
want $rc deps_teach
qemu-system-aarch64 -M virt -cpu cortex-a53 -m 256M -machine dumpdtb=$B/virt.dtb -display none > /dev/null 2>&1
$B/bringup_deps $B/virt.dtb > $B/virt.txt 2>&1; rc=$?
awk '/^\/virtio_mmio@/ && !/^\/virtio_mmio@a000000 / {next} {print}' $B/virt.txt |
    sed 's/^\(  stage [0-9]*:.*virtio_mmio@a000000\) virtio_mmio@[^ ]*\( virtio_mmio@[^ ]*\)*/\1 [31 more virtio_mmio nodes]/' > deps_virt.out
echo "[...] the rows of virtio_mmio@a000200 to virtio_mmio@a003e00 (31 nodes, each like virtio_mmio@a000000) are not shown" >> deps_virt.out
rec deps_virt "bringup_deps.cc" "$GXX_VER; $QA64" \
    "qemu-system-aarch64 -M virt -cpu cortex-a53 -m 256M -machine dumpdtb=virt.dtb; ./bringup_deps virt.dtb (31 virtio rows trimmed)" "$rc"
want $rc deps_virt
python3 ../F4-27/make_dtb.py $B/pi3.dtb > /dev/null 2>&1
$B/bringup_deps $B/pi3.dtb > deps_pi3.out 2>&1; rc=$?
rec deps_pi3 "F4-27 make_dtb.py (DR402), unchanged; bringup_deps.cc" "$PY_VER; $GXX_VER" \
    "python3 ../F4-27/make_dtb.py pi3.dtb; ./bringup_deps pi3.dtb" "$rc"
want $rc deps_pi3

# 5. trap 2: a bus address used as a CPU address, on QEMU raspi3b with the same pi3.dtb
{
    aarch64-linux-gnu-g++ $KFLAGS -c start.S -o $B/start.o &&
    aarch64-linux-gnu-g++ $KFLAGS -c translate_demo.cc -o $B/td.o &&
    aarch64-linux-gnu-ld -nostdlib -static --no-warn-rwx-segments -T linker.ld -o $B/td.elf $B/start.o $B/td.o &&
    aarch64-linux-gnu-objcopy -O binary $B/td.elf $B/td.bin
} > translate_build.out 2>&1; rc=$?
echo "build exit code: $rc; image: $(stat -c %s $B/td.bin 2>/dev/null) bytes" >> translate_build.out
rec translate_build "start.S translate_demo.cc linker.ld (+ F4-23 fdt.h)" "$XGXX_VER" \
    "aarch64-linux-gnu-g++ $KFLAGS -c start.S / translate_demo.cc; ld -nostdlib -static -T linker.ld; objcopy -O binary" "$rc"
want $rc translate_build
QPI="qemu-system-aarch64 -M raspi3b -display none -monitor none -serial stdio -no-reboot -semihosting"
timeout 10 $QPI -dtb $B/pi3.dtb -kernel $B/td.bin < /dev/null 2>&1 | clean_serial > translate_demo.out
rc=${PIPESTATUS[0]}
rec translate_demo "td.bin (translate_demo.cc)" "$QA64" "$QPI -dtb pi3.dtb -kernel td.bin" "$rc" \
    "note:      exit code 3 = the program's exception handler stopped the run (expected: this is the trap)" "$HW"
[ "$rc" = 3 ]; want $? translate_demo
elr="$(sed -n 's/.*ELR_EL1=0x\([0-9a-f]*\).*/\1/p' translate_demo.out)"
aarch64-linux-gnu-addr2line -f -C -i -e $B/td.elf "0x$elr" | sed "s#$(pwd)/##" > translate_addr2line.out; rc=$?
rec translate_addr2line "td.elf (linked at 0x80000)" "$(aarch64-linux-gnu-addr2line --version | head -n 1)" \
    "aarch64-linux-gnu-addr2line -f -C -i -e td.elf 0x$elr" "$rc"

# 6. trap 3: writing on-board storage without a recovery path. The "board" is QEMU's Arm virt
#    machine; its boot flash (pflash unit 0) holds the distribution's EDK2 firmware (AAVMF).
FW=/usr/share/AAVMF/AAVMF_CODE.fd
VARS=/usr/share/AAVMF/AAVMF_VARS.fd
WRONG=/usr/share/qemu/opensbi-riscv64-generic-fw_dynamic.bin
FWVER="$(dpkg-query -W -f='${Package} ${Version}' qemu-efi-aarch64 2>/dev/null)"
VIRT="qemu-system-aarch64 -M virt -cpu cortex-a57 -m 512M -display none -monitor none -serial stdio -no-reboot"
boot_flash() {   # boot_flash OUTFILE : boot from $B/flash0.img for 8 s, serial output to OUTFILE
    cp "$VARS" $B/flash1.img
    timeout 8 $VIRT -drive if=pflash,format=raw,file=$B/flash0.img -drive if=pflash,format=raw,file=$B/flash1.img \
        < /dev/null 2>&1 | clean_serial | grep -a -v 'terminating on signal' > "$1"
    return "${PIPESTATUS[0]}"
}
FCMD="timeout 8 $VIRT -drive if=pflash,format=raw,file=flash0.img -drive if=pflash,format=raw,file=flash1.img"
cp "$FW" $B/flash0.img
{ echo "on-board flash (flash0.img): $(stat -c %s $B/flash0.img) bytes, sha256 $(sha256sum $B/flash0.img | cut -c1-16)..."
  echo "serial output of the boot (first line only; the firmware then tries a network boot):"; } > flash_ok.out
boot_flash $B/ok.txt; rc=$?
head -n 1 $B/ok.txt >> flash_ok.out
rec flash_ok "EDK2 firmware ($FWVER, $FW)" "$QA64" "$FCMD" "$rc" \
    "note:      124 = stopped by the 8 s limit (the firmware keeps running); pass = the line 'UEFI firmware (version ...)'" "$HW"
grep -q '^UEFI firmware' flash_ok.out; want $? flash_ok

# the careless update: an image for ANOTHER board written over the boot flash, no copy kept
python3 - "$WRONG" $B/flash0.img <<'EOF' > $B/write.txt
import sys
img = open(sys.argv[1], "rb").read()
with open(sys.argv[2], "r+b") as f:
    f.write(img)
print("wrote %d bytes of %s at offset 0 of the boot flash" % (len(img), sys.argv[1].rsplit("/", 1)[-1]))
EOF
{ cat $B/write.txt
  echo "on-board flash now: sha256 $(sha256sum $B/flash0.img | cut -c1-16)..."
  echo "serial output of the boot (8 s):"; } > flash_bad.out
boot_flash $B/bad.txt; rc=$?
if [ -s $B/bad.txt ]; then cat $B/bad.txt >> flash_bad.out; else echo "(nothing at all)" >> flash_bad.out; fi
rec flash_bad "the same boot after the update" "$QA64" "$FCMD" "$rc" \
    "note:      124 = stopped by the 8 s limit; expected: no output (the board looks dead)" "$HW"
[ ! -s $B/bad.txt ]; want $? flash_bad
cp "$VARS" $B/flash1.img
timeout 1 qemu-system-aarch64 -M virt -cpu cortex-a57 -m 512M -display none -monitor none -serial null -no-reboot \
    -drive if=pflash,format=raw,file=$B/flash0.img -drive if=pflash,format=raw,file=$B/flash1.img \
    -d int -D $B/int.txt < /dev/null > /dev/null 2>&1
{ echo "first 10 lines of QEMU's exception log (-d int, 1 s):"; head -n 10 $B/int.txt
  echo "[...] $(grep -c 'Taking exception' $B/int.txt) exceptions logged in 1 s; distinct PCs after them:"
  sed -n 's/^\.\.\.to EL1 PC \(0x[0-9a-f]*\).*/\1/p' $B/int.txt | sort | uniq -c | sed 's/^ */    /'; } > flash_bad_trace.out
rec flash_bad_trace "QEMU -d int" "$QA64" "timeout 1 qemu-system-aarch64 -M virt -cpu cortex-a57 ... -d int -D int.txt (first lines and a count)" "0" "$HW"

# recovery: the image saved and verified BEFORE the update is written back
cp "$FW" $B/backup.img
{ echo "backup taken before the update: sha256 $(sha256sum $B/backup.img | cut -c1-16)... (matches the original: $( [ "$(sha256sum < $B/backup.img)" = "$(sha256sum < $FW)" ] && echo yes || echo NO))"
  cp $B/backup.img $B/flash0.img
  echo "restored; on-board flash now: sha256 $(sha256sum $B/flash0.img | cut -c1-16)..."
  echo "serial output of the boot (first line only):"; } > flash_recover.out
boot_flash $B/rec.txt; rc=$?
head -n 1 $B/rec.txt >> flash_recover.out
rec flash_recover "EDK2 firmware restored from the backup" "$QA64" "$FCMD" "$rc" \
    "note:      124 = stopped by the 8 s limit; pass = the line 'UEFI firmware (version ...)' again" "$HW"
grep -q '^UEFI firmware' flash_recover.out; want $? flash_recover

# 7. forensic lab evidence: Lena's kernel, written on QEMU riscv64 virt, run unchanged on the board
RVX="-march=rv64imac_zicsr -mabi=lp64 -mcmodel=medany -ffreestanding -nostdlib -fno-exceptions -fno-rtti \
-fno-stack-protector -O2 -g -Wall -Wextra -Wpedantic -Werror"
{
    riscv64-linux-gnu-g++ $RVX -c rv_start.S -o $B/rv_start.o &&
    riscv64-linux-gnu-g++ $RVX -c hello_rv.cc -o $B/hello_rv.o &&
    riscv64-linux-gnu-ld --no-warn-rwx-segments -T rv.ld -o $B/hello_rv.elf $B/rv_start.o $B/hello_rv.o
} > forensic_build.out 2>&1; rc=$?
echo "build exit code: $rc" >> forensic_build.out
rec forensic_build "rv_start.S hello_rv.cc rv.ld" "$(riscv64-linux-gnu-g++ --version | head -n 1); $(riscv64-linux-gnu-ld --version | head -n 1)" \
    "riscv64-linux-gnu-g++ $RVX -c rv_start.S / hello_rv.cc; riscv64-linux-gnu-ld -T rv.ld -o hello_rv.elf" "$rc"
want $rc forensic_build
fboot() {   # fboot MACHINE SERIAL-FILE ERROR-LOG
    timeout 4 qemu-system-riscv64 -M "$1" -m 256M -display none -monitor none -serial stdio -bios default \
        -kernel $B/hello_rv.elf -d guest_errors -D "$3" < /dev/null 2>&1 | clean_serial |
        grep -a -v 'terminating on signal' > "$2"
    return "${PIPESTATUS[0]}"
}
FB="timeout 4 qemu-system-riscv64 -M <machine> -m 256M -display none -monitor none -serial stdio -bios default -kernel hello_rv.elf -d guest_errors -D errors.txt"
fboot virt $B/fv.txt $B/fv_err.txt; rc=$?
{ grep -a -E '^(Platform Name|Platform Console)' $B/fv.txt; echo "[... the rest of the OpenSBI banner ...]"
  sed -n '/^Boot HART MEDELEG/,$p' $B/fv.txt | tail -n +2
  echo "-- QEMU guest_errors log: $(wc -l < $B/fv_err.txt) lines --"; } > forensic_virt.out
rec forensic_virt "hello_rv.elf on QEMU riscv64 virt" "$QRV" "${FB/<machine>/virt}" "$rc" \
    "note:      124 = stopped by the 4 s limit (the kernel loops after printing); OpenSBI banner trimmed" "$HW"
grep -q "^hello from Lena's kernel" forensic_virt.out; want $? forensic_virt
fboot sifive_u $B/fu.txt $B/fu_err.txt; rc=$?
{ grep -a -E '^(Platform Name|Platform Console)' $B/fu.txt; echo "[... the rest of the OpenSBI banner ...]"
  sed -n '/^Boot HART MEDELEG/,$p' $B/fu.txt
  echo "-- (no further serial output) --"; } > forensic_board.out
rec forensic_board "hello_rv.elf on QEMU sifive_u (the board model)" "$QRV" "${FB/<machine>/sifive_u}" "$rc" \
    "note:      124 = stopped by the 4 s limit; expected: the kernel prints nothing" "$HW"
! grep -q "hello from" forensic_board.out; want $? forensic_board
{ echo "first 3 lines of the guest_errors log of the board run:"; head -n 3 $B/fu_err.txt
  echo "[...] $(wc -l < $B/fu_err.txt) lines in 4 s; distinct lines:"
  sort $B/fu_err.txt | uniq -c | sed 's/^ */    /'; } > forensic_errors.out
rec forensic_errors "QEMU -d guest_errors log of the board run" "$QRV" "head, wc and sort | uniq -c on errors.txt" "0" "$HW"

rm -rf "$B"
exit $status
