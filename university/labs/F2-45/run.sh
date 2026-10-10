#!/usr/bin/env bash
# F2-45 lab: PE/COFF -- the PE/COFF half of curriculum milestone P2, plus a real UEFI boot.
#   step build    : the same C++ file as a COFF object, a PE32+ EFI application and an ELF object
#   step compare  : pe_read versus llvm-readobj on the image and the object file
#   step pe_sample: pe_read's output and the first bytes of the image
#   step relocs   : the relocations of the COFF object next to those of the ELF object
#   step boot     : OVMF (UEFI firmware) in QEMU loads and starts the application
#   step forensic_fixed : the same application linked with /fixed (no base relocations)
set -u
cd "$(dirname "$0")" || exit 2
LAB="$(pwd)"
status=0
TOOL="$(clang++ --version | head -n 1); $(lld-link --version | head -n 1); $(llvm-readobj --version | sed -n 's/^ *LLVM version/LLVM version/p' | head -n 1)"
QV="$(qemu-system-x86_64 --version | head -n 1)"
W="${LAB}/.work"; rm -rf "$W"; mkdir -p "$W"
CXXF="-std=c++20 -ffreestanding -fno-exceptions -fno-rtti -mno-red-zone -O2 -Wall -Wextra -Werror"
OVMF_CODE=/usr/share/OVMF/OVMF_CODE_4M.fd
OVMF_VARS=/usr/share/OVMF/OVMF_VARS_4M.fd

begin() {   # begin <name> <listing> <command summary> [extra toolchain] [hardware note]
    NAME="$1"; LAST=0; BAD=0
    OUT="${LAB}/${NAME}.out"; LOG="${LAB}/${NAME}.log"; : > "$OUT"
    {
        echo "listing:   $2"
        echo "toolchain: $TOOL${4:-}"
        echo "command:   $3"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
        if [ -n "${5:-}" ]; then echo "hardware:  $5"; fi
    } > "$LOG"
    cd "$W" || exit 2
}
c() {       # c '<command>' [expected exit code]
    echo "\$ $1" >> "$OUT"
    bash -c "$1" >> "$OUT" 2>&1
    LAST=$?
    if [ "$LAST" != "${2:-0}" ]; then BAD=$((BAD + 1)); fi
    if [ "$LAST" != 0 ]; then echo "(exit code: $LAST)" >> "$OUT"; fi
}
finish() {
    sed -i -e "s#${W}/##g" -e "s#${LAB}/##g" "$OUT"
    echo "exit code: $LAST" >> "$LOG"
    [ -n "${1:-}" ] && echo "$1" >> "$LOG"
    if [ "$BAD" != 0 ]; then echo "result:    STEP FAILED ($BAD command(s) did not give the expected exit code)" >> "$LOG"; status=1; fi
    cd "$LAB" || exit 2
}
# boot_esp <efi file> <timeout s>: put the file at \EFI\BOOT\BOOTX64.EFI on a FAT image and
# start QEMU with OVMF; our app writes to port 0xe9 (debug.txt), the firmware's console
# goes to the serial port (serial.txt).
boot_esp() {
    rm -f esp.img debug.txt serial.txt
    mkfs.fat -C esp.img 4096 > /dev/null
    mmd -i esp.img ::/EFI ::/EFI/BOOT
    mcopy -i esp.img "$1" ::/EFI/BOOT/BOOTX64.EFI
    cp "$OVMF_VARS" vars.fd
    timeout "$2" qemu-system-x86_64 -machine q35 -nodefaults -display none -no-reboot \
        -drive if=pflash,format=raw,readonly=on,file="$OVMF_CODE" -drive if=pflash,format=raw,file=vars.fd \
        -drive file=esp.img,format=raw,if=none,id=d0 -device virtio-blk-pci,drive=d0 \
        -serial file:serial.txt -debugcon file:debug.txt -device isa-debug-exit,iobase=0xf4,iosize=0x04
}
BOOTCMD="mkfs.fat -C esp.img 4096; mmd/mcopy app.efi to ::/EFI/BOOT/BOOTX64.EFI; qemu-system-x86_64 -machine q35 -nodefaults -display none -no-reboot -drive if=pflash,format=raw,readonly=on,file=$OVMF_CODE -drive if=pflash,format=raw,file=vars.fd -drive file=esp.img,format=raw,if=none,id=d0 -device virtio-blk-pci,drive=d0 -serial file:serial.txt -debugcon file:debug.txt -device isa-debug-exit,iobase=0xf4,iosize=0x04"
export -f boot_esp; export OVMF_CODE OVMF_VARS

# Step 1: one source file, three outputs.
begin build "efi/app.cpp" "clang++ --target=x86_64-unknown-windows $CXXF -c app.cpp; lld-link ...; clang++ --target=x86_64-unknown-none-elf ... -c"
c "clang++ --target=x86_64-unknown-windows $CXXF -c ${LAB}/efi/app.cpp -o app.obj"
c "lld-link /subsystem:efi_application /entry:efi_main /nodefaultlib /Brepro /out:app.efi app.obj"
c "clang++ --target=x86_64-unknown-none-elf $CXXF -c ${LAB}/efi/app.cpp -o app.o"
c "file app.obj app.efi app.o"
c "ls -l app.obj app.efi app.o | awk '{print \$5, \$9}'"
finish

# Step 2: pe_read must agree with llvm-readobj (P2's acceptance test, PE/COFF half).
g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined "${LAB}/pe/pe_read.cpp" -o "$W/pe_read" || status=1
begin compare "pe/pe_read.cpp canon_readobj.sh" "for f in app.efi app.obj: diff <(canon_readobj.sh f) <(pe_read f) (hex digits compared in lower case)" "; $(g++ --version | head -n 1)"
for f in app.efi app.obj; do
    c "diff <(${LAB}/canon_readobj.sh $f | tr A-F a-f) <(./pe_read $f | tr A-F a-f) && echo \"MATCH $f: \$(./pe_read $f | wc -l) lines\""
done
finish

# Step 3: what pe_read prints for the image, and the raw bytes it starts from.
begin pe_sample "pe/pe_read.cpp" "./pe_read app.efi; od -A x -t x1 on the first 64 bytes and on the 24 bytes at e_lfanew"
c "./pe_read app.efi"
c "od -A x -t x1 -N 64 app.efi"
c "od -A x -t x1 -j \$(od -A n -t u4 -j 60 -N 4 app.efi) -N 24 app.efi"
finish

# Step 4: relocations of the same code in COFF and in ELF.
begin relocs "efi/app.cpp" "llvm-readobj -r app.obj; readelf -rW app.o; llvm-readobj --coff-basereloc app.efi" "; $(readelf --version | head -n 1)"
c "llvm-readobj -r app.obj"
c "readelf -rW app.o"
c "llvm-objdump -d --no-show-raw-insn -r app.obj | sed -n '/<efi_main>:/,/^\$/p' | head -n 12"
c "llvm-readobj --coff-basereloc app.efi | sed -n '/BaseReloc/,\$p'"
finish

# Step 5: the firmware loads and starts the application (exit 67 = 0x21 << 1 | 1, by design).
begin boot "efi/app.cpp" "$BOOTCMD" "; $QV; OVMF from /usr/share/OVMF (OVMF_CODE_4M.fd)" "emulated PC (QEMU q35 + OVMF); untested on a physical UEFI PC"
c "boot_esp app.efi 60" 67
c "cat debug.txt"
c "sed 's/\\x1b\\[[0-9;=]*[A-Za-z]//g' serial.txt | tr -d '\\r' | grep -E 'BdsDxe' || true"
finish "note:      exit code 67 is expected: the app wrote 0x21 to the isa-debug-exit port and QEMU exits with (0x21 << 1) | 1"

# Step 6 (forensic evidence): the same object linked with /fixed.
begin forensic_fixed "efi/app.cpp" "lld-link ... /fixed /out:fixed.efi app.obj; pe_read; boot as in step boot (25 s limit)" "; $QV" "emulated PC (QEMU q35 + OVMF); untested on a physical UEFI PC"
c "lld-link /subsystem:efi_application /entry:efi_main /nodefaultlib /Brepro /fixed /out:fixed.efi app.obj"
c "./pe_read fixed.efi | grep -E '^(Characteristics|ImageBase|DllCharacteristics|BaseReloc|Section)'"
c "./pe_read app.efi | grep -E '^(Characteristics|ImageBase|DllCharacteristics|BaseReloc|Section)'"
c "boot_esp fixed.efi 25" 124
c "cat debug.txt"
c "sed 's/\\x1b\\[[0-9;=]*[A-Za-z]//g' serial.txt | tr -d '\\r' | grep -E 'BdsDxe|Shell>|UEFI Interactive' | sed 's/Shell>.*/Shell>/' || true"
finish "note:      exit code 124 is expected: nothing stops QEMU, so the 25 s time limit ends it"
rm -rf "$W"
exit $status
