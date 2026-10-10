#!/usr/bin/env bash
# F4-12 run.sh: milestone C11 in QEMU (q35, intel-hda + hda-output, "wav" audio backend).
#   build     : the F4-12 kernel (default) and the forensic variant
#   audio     : codec walk and 2 s of a 440 Hz / 660 Hz stereo tone, recorded to a WAV file
#   wav       : the recording measured on the host (wavcheck.py: FFT peak per channel)
#   audio_iommu, wav_iommu : the same with intel-iommu (all controller DMA through VT-d)
#   forensic  : the kernel built with -DF412_RATE_BUG; forensic_wav = its recording
set -u -o pipefail
cd "$(dirname "$0")"
. ../F4-01/lablib.sh
status=0
SRC="../F4-01/boot.S ../F4-01/kbase.cc ../F4-01/div64.cc ../F4-02/pci.cc ../F4-02/acpi.cc \
../F4-08/isr256.S ../F4-08/intr.cc ../F4-08/msi.cc ../F4-08/vtd.cc ../F4-08/dma.cc hda.cc f412_main.cc"
NUMPY_VER="$(python3 -c 'import sys, numpy; print("Python", sys.version.split()[0], "numpy", numpy.__version__)')"
snd() { echo "-device intel-hda -device hda-output,audiodev=snd0 -audiodev wav,id=snd0,path=$1"; }

kbuild k412.elf $SRC > .kb.txt 2>&1; rc=$?
KEXTRA="-DF412_RATE_BUG" kbuild k412f.elf $SRC >> .kb.txt 2>&1 || rc=1
{ cat .kb.txt; size -A k412.elf | grep -E '^(section|\.text|\.rodata|\.data|\.bss|Total)'; } > build.out
rec build "$SRC kernel.ld" "$GXX_VER; $LD_VER" \
    "g++ $KFLAGS -c <each file>; ld -m elf_i386 -nostdlib -static -T kernel.ld -o k412.elf *.o (and again with -DF412_RATE_BUG)" "$rc"
[ "$rc" = 0 ] || status=1

one() {  # $1 name, $2 kernel, $3 extra QEMU options, $4 wav name, $5 expected kernel rc, $6 expected wav rc, $7 note
    qboot "$1.out" 60 q35 "$2" $3 $(snd ".$4.wav"); local rc=$?
    rec "$1" "f412_main.cc (kernel $2)" "$QEMU_VER" \
        "$QEMU -machine q35 $QCOMMON -serial stdio -kernel $2 $3 $(snd "$4.wav")" "$rc" \
        "note:      exit code 33 = pass (the kernel finished playing)" "$HW_NOTE"
    [ "$rc" = "$5" ] || status=1
    python3 wavcheck.py ".$4.wav" --expect 440,660 > "$4.out" 2>&1; rc=$?
    rec "$4" "wavcheck.py on the recording of the $1 run" "$NUMPY_VER" "python3 wavcheck.py $4.wav --expect 440,660" "$rc" "$7"
    [ "$rc" = "$6" ] || status=1
}
one audio k412.elf "" wav 33 0 "note:      exit code 0 = both channels within 1 % of 440 / 660 Hz"
one audio_iommu k412.elf "-device intel-iommu" wav_iommu 33 0 "note:      exit code 0 = both channels within 1 % of 440 / 660 Hz"
one forensic k412f.elf "" forensic_wav 33 1 "note:      exit code 1 = a channel is off by more than 1 %: expected for the forensic build"

rm -f k412.elf k412f.elf .kb.txt .wav.wav .wav_iommu.wav .forensic_wav.wav
exit $status
