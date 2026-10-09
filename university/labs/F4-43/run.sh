#!/usr/bin/env bash
# F4-43 run.sh: milestone G2 (a 2D virtio-gpu driver) in QEMU (q35, virtio-gpu-pci).
#   build    : the F4-43 kernel (DR301 base + F4-05 virtio transport + virtio_gpu.cc)
#   vgpu     : 640x480 run; QEMU screen dumps of frames 1 and 2 taken through the monitor
#   screens  : the acceptance test: each dump compared with the reference image (refimg.cc)
#   trace    : QEMU's own trace of the virtio-gpu commands it received in the vgpu run
#   res800   : the same kernel on a device configured for 800x600
#   forensic : a kernel that flushes the damaged area without transferring it first
set -u -o pipefail
cd "$(dirname "$0")"
. ../F4-01/lablib.sh
status=0
SRC="../F4-01/boot.S ../F4-01/kbase.cc ../F4-01/div64.cc ../F4-02/pci.cc ../F4-05/virtio.cc virtio_gpu.cc f443_main.cc"
QM="-machine q35 -m 64M -nodefaults -display none -no-reboot -monitor none -device isa-debug-exit,iobase=0xf4,iosize=0x04"
SHOTS=(wait:"frame 1 ready" "hmp:screendump .f1.ppm" "hmp:sendkey spc" wait:"frame 2 ready" "hmp:screendump .f2.ppm" "hmp:sendkey spc" exit)
SHOTS_SHOWN="wait:'frame 1 ready' 'hmp:screendump f1.ppm' 'hmp:sendkey spc' wait:'frame 2 ready' 'hmp:screendump f2.ppm' 'hmp:sendkey spc' exit"

kbuild k443.elf $SRC > .kb.txt 2>&1; rc=$?
{ cat .kb.txt; size -A k443.elf | grep -E '^(section|\.text|\.rodata|\.data|\.bss|Total)'; } > build.out
rec build "$SRC kernel.ld" "$GXX_VER; $LD_VER" \
    "g++ $KFLAGS -c <each file>; ld -m elf_i386 -nostdlib -static -T kernel.ld -o k443.elf *.o" "$rc"
[ "$rc" = 0 ] || status=1
hostbuild .refimg refimg.cc || status=1

# boot <kernel> <device options> <serial file> <trace file>: QEMU with screen dumps
boot() {
    rm -f "$3" "$4" .f1.ppm .f2.ppm
    python3 -I ../F4-03/qmp_drive.py "$3" "${SHOTS[@]}" -- $QEMU $QM -serial file:"$3" -kernel "$1" \
        -device "$2" -trace 'virtio_gpu_cmd_*' -trace virtio_gpu_update_cursor -D "$4" > .drive.txt 2>&1
}
screens() {   # compare both dumps with the reference; prints one line per frame
    local r=0
    for n in 1 2; do ./.refimg "$n" ".f$n.ppm" || r=1; done
    return $r
}
strip_trace() { sed 's/^[0-9]*@[0-9.]*://' "$1"; }

# 1. 640x480
boot k443.elf virtio-gpu-pci,xres=640,yres=480 .ser.txt .trace.txt; rc=$?
{ sed 's/\r$//' .ser.txt; echo "== monitor steps (qmp_drive.py) =="; cat .drive.txt; } > vgpu.out
rec vgpu "f443_main.cc, virtio_gpu.cc (kernel k443.elf)" "$QEMU_VER; $(python3 --version)" \
    "python3 -I ../F4-03/qmp_drive.py ser.txt $SHOTS_SHOWN -- $QEMU $QM -serial file:ser.txt -kernel k443.elf -device virtio-gpu-pci,xres=640,yres=480 -trace 'virtio_gpu_cmd_*' -trace virtio_gpu_update_cursor -D trace.txt" "$rc" \
    "note:      exit code 33 = pass (isa-debug-exit 0x10)" "$HW_NOTE"
[ "$rc" = 33 ] || status=1
screens > screens.out; rc=$?
rec screens "refimg.cc (reference renderer, same scene.h as the kernel)" "$GXX_VER" \
    "g++ $HOSTFLAGS refimg.cc -o refimg; ./refimg 1 f1.ppm; ./refimg 2 f2.ppm" "$rc"
[ "$rc" = 0 ] || status=1
strip_trace .trace.txt > trace.out
rec trace "(none: QEMU's trace of the vgpu run)" "$QEMU_VER" "the -trace options of the vgpu step; timestamps removed" 0
grep -q 'virtio_gpu_update_cursor scanout 0, x 300, y 250, move' trace.out || status=1

# 2. 800x600: the driver takes the size from GET_DISPLAY_INFO
boot k443.elf virtio-gpu-pci,xres=800,yres=600 .ser.txt .trace.txt; rc=$?
{ grep -E 'scanout 0:|frame [12] ready|damage:|done' .ser.txt | sed 's/\r$//'; screens; } > res800.out; rc2=$?
rec res800 "f443_main.cc (kernel k443.elf), refimg.cc" "$QEMU_VER" \
    "python3 -I ../F4-03/qmp_drive.py ser.txt $SHOTS_SHOWN -- $QEMU $QM -serial file:ser.txt -kernel k443.elf -device virtio-gpu-pci,xres=800,yres=600 ...; ./refimg 1 f1.ppm; ./refimg 2 f2.ppm" "$rc" \
    "note:      exit code 33 = pass; screens compared: $([ "$rc2" = 0 ] && echo MATCH || echo MISMATCH)" "$HW_NOTE"
[ "$rc" = 33 ] && [ "$rc2" = 0 ] || status=1

# 3. forensic: the update path forgets TRANSFER_TO_HOST_2D
KEXTRA="-DF443_SKIP_TRANSFER_ON_UPDATE" kbuild k443f.elf $SRC > .kbf.txt 2>&1 || status=1
boot k443f.elf virtio-gpu-pci,xres=640,yres=480 .serf.txt .tracef.txt; rc=$?
{ echo "== serial log (from GET_DISPLAY_INFO on; EDID lines omitted) =="
  sed 's/\r$//' .serf.txt | grep -v '^edid ' | sed -n '/GET_DISPLAY_INFO/,$p'
  echo "== screen dumps compared with the reference renderer =="; screens
  echo "== QEMU trace: commands received for resource 0x1 =="
  strip_trace .tracef.txt | grep 'res 0x1'; } > forensic.out
rec forensic "f443_main.cc built with -DF443_SKIP_TRANSFER_ON_UPDATE, refimg.cc" "$QEMU_VER" \
    "python3 -I ../F4-03/qmp_drive.py serf.txt $SHOTS_SHOWN -- $QEMU $QM -serial file:serf.txt -kernel k443f.elf -device virtio-gpu-pci,xres=640,yres=480 -trace 'virtio_gpu_cmd_*' -D tracef.txt; ./refimg 1 f1.ppm; ./refimg 2 f2.ppm" "$rc" \
    "note:      exit code 33: the kernel itself sees no error; the evidence is in the screen dump and the trace" "$HW_NOTE"
[ "$rc" = 33 ] || status=1
grep -q 'frame 2: .*MISMATCH' forensic.out || status=1

rm -f k443.elf k443f.elf .kb.txt .kbf.txt .ser.txt .serf.txt .trace.txt .tracef.txt .f1.ppm .f2.ppm .drive.txt .refimg
exit $status
