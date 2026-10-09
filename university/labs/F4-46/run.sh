#!/usr/bin/env bash
# F4-46 run.sh: the parts of milestone G5 that can run without an Intel GPU.
#   record    : EDID blobs recorded from QEMU's virtio-gpu at five resolutions, read by the
#               F4-43 driver (GET_EDID); one edid_<W>x<H>.hex file each (kept as evidence)
#   edid_test : the EDID parser's host unit tests on the five recorded blobs
set -u -o pipefail
cd "$(dirname "$0")"
. ../F4-01/lablib.sh
status=0
G=../F4-43
SRC="../F4-01/boot.S ../F4-01/kbase.cc ../F4-01/div64.cc ../F4-02/pci.cc ../F4-05/virtio.cc $G/virtio_gpu.cc $G/f443_main.cc"
QM="-machine q35 -m 64M -nodefaults -display none -no-reboot -monitor none -device isa-debug-exit,iobase=0xf4,iosize=0x04"
KEYS=(wait:"frame 1 ready" "hmp:sendkey spc" wait:"frame 2 ready" "hmp:sendkey spc" exit)

kbuild .k443.elf $SRC > .kb.txt 2>&1 || { cat .kb.txt; status=1; }
: > record.out
rcs=""
for r in 640x480 800x600 1024x768 720x480 960x540; do
    w="${r%x*}"; h="${r#*x}"
    rm -f .ser.txt
    python3 -I ../F4-03/qmp_drive.py .ser.txt "${KEYS[@]}" -- $QEMU $QM -serial file:.ser.txt -kernel .k443.elf \
        -device virtio-gpu-pci,xres=$w,yres=$h > .drive.txt 2>&1; rc=$?
    rcs="$rcs $rc"
    # the first 128 bytes (base block) of the GET_EDID response, as hex text
    sed 's/\r$//' .ser.txt | grep '^edid 0[0-7]' | cut -d: -f2 > "edid_$r.hex"
    printf '%-9s exit %s, %s EDID bytes saved in edid_%s.hex; %s\n' "$r" "$rc" "$(wc -w < "edid_$r.hex")" "$r" \
        "$(sed 's/\r$//' .ser.txt | grep 'scanout 0:')" >> record.out
    [ "$rc" = 33 ] || status=1
done
rec record "../F4-43/f443_main.cc and virtio_gpu.cc (kernel built from the F4-43 sources)" "$QEMU_VER; $(python3 --version)" \
    "for each W x H: python3 -I ../F4-03/qmp_drive.py ser.txt wait:'frame 1 ready' 'hmp:sendkey spc' wait:'frame 2 ready' 'hmp:sendkey spc' exit -- $QEMU $QM -serial file:ser.txt -kernel k443.elf -device virtio-gpu-pci,xres=W,yres=H; grep '^edid 0[0-7]' ser.txt" \
    "${rcs# }" "note:      exit codes 33 = pass for each of the five boots" "$HW_NOTE"

hostbuild .edid_test edid_test.cc && ./.edid_test edid_640x480.hex edid_800x600.hex edid_1024x768.hex edid_720x480.hex edid_960x540.hex > edid_test.out; rc=$?
rec edid_test "edid_test.cc, edid.h" "$GXX_VER; linux-libc-dev $(dpkg-query -W -f='${Version}' linux-libc-dev 2>/dev/null) (drm_mode.h)" \
    "g++ $HOSTFLAGS edid_test.cc -o edid_test; ./edid_test edid_640x480.hex edid_800x600.hex edid_1024x768.hex edid_720x480.hex edid_960x540.hex" "$rc"
[ "$rc" = 0 ] || status=1

rm -f .k443.elf .kb.txt .ser.txt .drive.txt .edid_test
exit $status
