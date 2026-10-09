#!/usr/bin/env bash
# F4-41 lab: nested paging and a real guest. The F4-39 kernel, built unchanged, runs as a
# guest of the F4-40 hypervisor extended with nested page tables and virtual interrupts.
#   build           the guest kernel and the hypervisor that embeds it
#   npt_probe       does QEMU translate guest-physical addresses when guest paging is off?
#   guest_detect / guest_hlt / guest_poll / guest_mono    the F4-39 tests, now as a guest
#   guest_npf       the guest reads guest-physical 1 GiB, which is not mapped
#   forensic_entry32  the same hypervisor entering the guest at its 32-bit entry point
set -u
cd "$(dirname "$0")"
. ../F4-39/dr404lib.sh
status=0
L=../F4-39; H=../F4-40
GSRC="$L/boot.S $L/kio.cc $L/intr.S $L/intr.cc $L/hvdetect.cc $L/goodguest.cc"
HSRC="$L/boot.S $L/kio.cc $L/intr.S $L/intr.cc $H/vmrun.S $H/hv.cc npt.cc vintr.cc guest_image.S f441_main.cc"
PSRC="$L/boot.S $L/kio.cc $L/intr.S $L/intr.cc $H/vmrun.S $H/hv.cc npt_probe.cc"
INC="-I$PWD -I$PWD/$H -DGUEST_ELF=\"$PWD/goodguest.elf\""
{
    kbuild goodguest.elf $GSRC &&
    KEXTRA="$INC" kbuild hv41.elf $HSRC &&
    KEXTRA="$INC" kbuild probe.elf $PSRC &&
    KEXTRA="$INC -DF441_ENTRY32" kbuild hv41e.elf $HSRC
} > build.out 2>&1
rc=$?
size goodguest.elf hv41.elf probe.elf >> build.out 2>&1
rec build "$HSRC; guest: $GSRC; probe: npt_probe.cc" "$GXX_VER" \
    "../F4-39/build_kernel.sh goodguest.elf <guest sources>; KEXTRA='-I. -I../F4-40 -DGUEST_ELF=\"goodguest.elf\"' ../F4-39/build_kernel.sh hv41.elf <hypervisor sources> (and probe.elf; hv41e.elf with -DF441_ENTRY32)" "$rc"
[ "$rc" = 0 ] || exit 1
CPU=qemu64,+svm,+npt
qrun npt_probe "npt_probe.cc (kernel probe.elf)" 1 probe32.elf $CPU 128M "probe" || status=1
LST="npt.cc, vintr.cc, f441_main.cc, ../F4-40/hv.cc (kernel hv41.elf; guest: the F4-39 kernel)"
qrun guest_detect "$LST" 1 hv4132.elf $CPU 128M "test=run guest:test=detect" || status=1
qrun guest_hlt "$LST, cputime.py" 1 hv4132.elf $CPU 128M "test=run guest:test=idle idle=hlt seconds=2" cputime || status=1
qrun guest_poll "$LST, cputime.py" 1 hv4132.elf $CPU 128M "test=run guest:test=idle idle=poll seconds=2" cputime || status=1
qrun guest_mono "$LST" 1 hv4132.elf $CPU 128M "test=run guest:test=mono reads=10000000" || status=1
qrun guest_npf "$LST" 1 hv4132.elf $CPU 128M "test=run expect=npf guest:test=touch addr=1073741824" || status=1

# The forensic build, with QEMU's own log of translated blocks and interrupts.
name=forensic_entry32
args=($QEMU $QCOMMON -cpu $CPU -m 128M -kernel hv41e32.elf -append "test=run guest:test=detect" -d in_asm,int,nochain -D .qemu.log)
timeout 120 "${args[@]}" > .serial.raw 2>&1; rc=$?
{
    echo "== serial console =="
    tr -d '\r' < .serial.raw | sed -n '/DR404 /,$p' | sed '1s/^.*\(DR404 \)/\1/'
    echo "== QEMU exit status: $rc =="
    echo "== QEMU log (-d in_asm,int): the first blocks translated after the first vmrun, decoded with objdump (32-bit below 0x1000c0) =="
    awk '/^vmrun!/{go=1} go' .qemu.log | grep -m 4 -A 3 '^IN:' | grep -E '^0x|OBJD' | paste - - |
    while read -r addr _ hex; do
        addr=${addr%:}
        echo "block at $addr:"
        python3 -I -c 'import sys; sys.stdout.buffer.write(bytes.fromhex(sys.argv[1]))' "$hex" > .blk.bin
        mode=i386:x86-64; [ $((addr)) -lt $((0x1000c0)) ] && mode=i386   # boot.S's 32-bit part
        objdump -D -b binary -m $mode --adjust-vma="$addr" .blk.bin | grep -E '^ +[0-9a-f]+:' | sed 's/^/   /'
    done
    echo "== QEMU log: vmexit lines and the end of the log =="
    grep -m 3 '^vmexit' .qemu.log
    grep -E '^check_exception|^Triple fault' .qemu.log | tail -n 4
    echo "== symbols of the two kernels (nm) =="
    echo "hypervisor hv41e.elf:"; nm hv41e.elf | grep -E ' (_start|__bss_start|__bss_end|long_mode)$' | sed 's/^/   /'
    echo "guest goodguest.elf:";  nm goodguest.elf | grep -E ' (_start|__bss_start|__bss_end|long_mode)$' | sed 's/^/   /'
    echo "== _start of each kernel (32-bit code), objdump -m i386 =="
    for k in hv41e.elf goodguest.elf; do
        echo "$k:"
        objdump -d -m i386 --start-address=0x10000c --stop-address=0x10002a "$k" | grep -E '^ +[0-9a-f]+:' | sed 's/^/   /'
    done
} > "$name.out"
verdict="expected"; [ "$rc" != 0 ] && { verdict="NOT the expected 0"; status=1; }
rec "$name" "npt.cc built with -DF441_ENTRY32 (kernel hv41e.elf)" "$QEMU_VER; $(objdump --version | head -n 1)" \
    "${args[*]}; then awk/grep/objdump on the QEMU log (see run.sh)" \
    "$rc (the machine reset (QEMU -no-reboot); $verdict)" "$HW_NOTE"
rm -f .serial.raw .qemu.log .blk.bin goodguest.elf goodguest32.elf hv41.elf hv4132.elf probe.elf probe32.elf hv41e.elf hv41e32.elf
exit $status
