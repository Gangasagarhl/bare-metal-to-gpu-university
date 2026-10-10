#!/usr/bin/env bash
# F3-41 lab steps (milestone H4, update part): host model of signed A/B updates with power
# cuts (bootsim), the forensic fleet night (fleet), then a real A/B boot loader on the
# emulated board: both slots valid, slot B damaged, no image at all.
set -u -o pipefail
cd "$(dirname "$0")"
status=0
B=.build
rm -rf "$B"; mkdir -p "$B"
rec() {  # rec <name> <listing> <toolchain> <command> <exit code> [extra line]
    {
        echo "listing:   $2"
        echo "toolchain: $3"
        echo "command:   $4"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
        echo "exit code: $5"
        if [ $# -ge 6 ]; then echo "$6"; fi
    } > "$1.log"
}
HOSTFLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
GV="$(g++ --version | head -n 1); $(openssl version)"
ARMFLAGS="--target=thumbv7m-none-eabi -mcpu=cortex-m3 -std=c++20 -ffreestanding -fno-exceptions -fno-rtti -nostdlib -Os -g -Wall -Wextra -Wpedantic -Werror"
QEMU="qemu-system-arm -M mps2-an385 -nographic -monitor none -serial stdio -semihosting-config enable=on,target=native"
TC="$(clang++ --version | head -n 1); $(ld.lld --version | head -n 1); $(qemu-system-arm --version | head -n 1)"
HW="hardware:  untested on hardware; QEMU mps2-an385, images placed in its code memory with QEMU's generic loader device, no real flash programming"

# 1. host model: H4 acceptance checks and the power-cut sweep
{ g++ $HOSTFLAGS bootsim.cc -lcrypto -o $B/bootsim && $B/bootsim; } > bootsim.out 2>&1; rc=$?
rec bootsim "bootsim.cc ota.h" "$GV" "g++ $HOSTFLAGS bootsim.cc -lcrypto -o bootsim && ./bootsim" "$rc"
[ "$rc" = 0 ] || status=1

# 2. forensic evidence: the night of the power cut, the fleet's design and the A/B replay
g++ $HOSTFLAGS fleet.cc -lcrypto -o $B/fleet > $B/fleet_build.txt 2>&1 || { cat $B/fleet_build.txt; status=1; }
$B/fleet single > fleet_single.out 2>&1; rc=$?
rec fleet_single "fleet.cc ota.h" "$GV" "g++ $HOSTFLAGS fleet.cc -lcrypto -o fleet && ./fleet single" "$rc" "note:      simulated fleet (host model), not field data"
$B/fleet ab > fleet_ab.out 2>&1; rc=$?
rec fleet_ab "fleet.cc ota.h" "$GV" "./fleet ab" "$rc" "note:      simulated fleet (host model), not field data"

# 3. on-target: build the boot loader and two application images, check SHA-256 two ways
{
    clang++ $ARMFLAGS -c startup.cc -o $B/startup.o &&
    clang++ $ARMFLAGS -c boot.cc -o $B/boot.o &&
    ld.lld -T boot.ld --gc-sections $B/startup.o $B/boot.o -o $B/boot.elf &&
    for v in 1 2; do
        if [ $v = 1 ]; then o=0x4100; else o=0xC100; fi
        sed "s/@ORIGIN@/$o/" slot.ld.in > $B/slot$v.ld &&
        clang++ $ARMFLAGS -DAPP_VERSION=$v -c app.cc -o $B/app$v.o &&
        ld.lld -T $B/slot$v.ld --gc-sections $B/startup.o $B/app$v.o -o $B/app$v.elf &&
        llvm-objcopy -O binary $B/app$v.elf $B/app$v.bin || exit 1
    done &&
    g++ $HOSTFLAGS mkimage.cc -o $B/mkimage &&
    (cd $B && ./mkimage app1.bin 1 4100 slotA.img && ./mkimage app2.bin 2 C100 slotB.img) &&
    echo "--- cross-check with the system tool (sha256sum of the raw payloads) ---" &&
    (cd $B && sha256sum app1.bin app2.bin) &&
    echo "--- boot loader size ---" && llvm-size -A $B/boot.elf | grep -E '^(section|\.text|\.data|\.bss)'
} > images.out 2>&1; rc=$?
rec images "boot.cc app.cc image.h sha256.h mkimage.cc boot.ld slot.ld.in" "$TC; $GV" \
    "clang++ ... boot.cc, app.cc (-DAPP_VERSION=1 at 0x4100, 2 at 0xC100); llvm-objcopy -O binary; mkimage; sha256sum" "$rc"
[ "$rc" = 0 ] || status=1
a1="$(grep 'slotA.img' images.out | sed 's/.*sha256 //')"; s1="$(grep ' app1.bin' images.out | cut -d' ' -f1)"
[ -n "$a1" ] && [ "$a1" = "$s1" ] || status=1

LOAD="-device loader,file=$B/slotA.img,addr=0x4000 -device loader,file=$B/slotB.img,addr=0xC000"
# 4. both slots valid: the higher version (B) boots
timeout 20 $QEMU -kernel $B/boot.elf $LOAD > boot_ab.out 2>&1; rc=$?
rec boot_ab "boot.elf + slotA.img + slotB.img" "$(qemu-system-arm --version | head -n 1)" \
    "timeout 20 $QEMU -kernel boot.elf -device loader,file=slotA.img,addr=0x4000 -device loader,file=slotB.img,addr=0xC000" "$rc" "$HW"
grep -q "application version 2 running" boot_ab.out || status=1

# 5. slot B damaged: one byte of its code changed (as if a write had been interrupted)
cp $B/slotB.img $B/slotB_bad.img
printf '\x00' | dd of=$B/slotB_bad.img bs=1 seek=$((0x100 + 0x200)) conv=notrunc 2>/dev/null
timeout 20 $QEMU -kernel $B/boot.elf -device loader,file=$B/slotA.img,addr=0x4000 \
    -device loader,file=$B/slotB_bad.img,addr=0xC000 > boot_damaged.out 2>&1; rc=$?
rec boot_damaged "boot.elf + slotA.img + slotB.img with byte 0x300 set to 0" "$(qemu-system-arm --version | head -n 1)" \
    "same as boot_ab with slotB_bad.img" "$rc" "$HW"
grep -q "application version 1 running" boot_damaged.out || status=1

# 6. no image in either slot: the boot loader must not jump anywhere
timeout 20 $QEMU -kernel $B/boot.elf > boot_empty.out 2>&1; rc=$?
rec boot_empty "boot.elf alone" "$(qemu-system-arm --version | head -n 1)" "timeout 20 $QEMU -kernel boot.elf" \
    "$rc (1 = the boot loader reported 'no valid image' through semihosting, as designed)" "$HW"
[ "$rc" = 1 ] || status=1
rm -rf "$B"
exit $status
