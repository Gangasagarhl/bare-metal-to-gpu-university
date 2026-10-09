#!/usr/bin/env bash
# F3-16 lab: Secure Boot and measured boot, as far as this build container can show them.
#   1. sbstate.efi reports the Secure Boot variables under three firmware set-ups:
#      the plain OVMF build, the Secure Boot build with no keys, and the Secure Boot build with
#      the distribution's pre-enrolled key stores (where our unsigned program is refused);
#   2. llvm-readobj: does our .efi file carry a signature (certificate table)?
#   3. measure: SHA-256 digests and PCR-style extends of three boot components, cross-checked
#      with sha256sum and Python's hashlib. No TPM is used (none is available here).
#   4. sign.py: our own Authenticode signature with the distribution's test key; the firmware with
#      that key enrolled runs the signed program and refuses the unsigned and a tampered copy.
# Every step writes <name>.out (real output) and <name>.log (run record).
set -u -o pipefail
cd "$(dirname "$0")"
status=0
B=.build
rm -rf "$B"; mkdir -p "$B/esp/EFI/BOOT"
QEMU=qemu-system-x86_64
QEMUV="$($QEMU --version | head -n 1)"
OVMFV="OVMF from the ovmf package $(dpkg-query -W -f '${Version}' ovmf)"
CLANGV="$(clang++ --version | head -n 1); $(lld-link --version | head -n 1)"
EMU="hardware:  untested on hardware; QEMU 8.2.2 q35 machine with the distribution's OVMF builds, no TPM, not a real PC"
rec() {  # rec <name> <listing> <toolchain> <command> <exit code text> [extra line]
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
UEFI_CXX="clang++ --target=x86_64-unknown-windows -std=c++20 -ffreestanding -fno-exceptions -fno-rtti -mno-red-zone -fno-stack-protector -O2 -Wall -Wextra -Wpedantic -Werror -I ../F3-10"
UEFI_LD="lld-link /subsystem:efi_application /entry:efi_main /nodefaultlib"
$UEFI_CXX -c sbstate.cc -o $B/sbstate.o && $UEFI_LD /Brepro /out:$B/esp/EFI/BOOT/BOOTX64.EFI $B/sbstate.o > /dev/null || status=1

# 1. three firmware set-ups, same disk
run_case() {  # run_case <name> <code file> <vars file> <machine options> <explanation>
    cp "$3" $B/vars.fd
    python3 ../F3-10/qemu_run.py --timeout 60 --stop-after "Press any key to enter the Boot Manager" -- $QEMU $4 -m 256M \
        -display none -serial stdio -no-reboot -net none \
        -drive if=pflash,format=raw,readonly=on,file="$2" -drive if=pflash,format=raw,file=$B/vars.fd \
        -drive format=raw,file=fat:rw:$B/esp -device isa-debug-exit,iobase=0xf4,iosize=0x04 2>&1 \
        | grep -v -E '^\s*$' > "$1.out"; rc=${PIPESTATUS[0]}
    rec "$1" "sbstate.cc (as \\EFI\\BOOT\\BOOTX64.EFI)" "$CLANGV; $QEMUV; $OVMFV" \
        "python3 ../F3-10/qemu_run.py --timeout 60 --stop-after 'Press any key to enter the Boot Manager' -- $QEMU $4 -m 256M -display none -serial stdio -no-reboot -net none -drive if=pflash,format=raw,readonly=on,file=$(basename "$2") -drive if=pflash,format=raw,file=$(basename "$3")(copy) -drive format=raw,file=fat:rw:esp -device isa-debug-exit,iobase=0xf4,iosize=0x04" \
        "$rc ($5)" "$EMU"
}
run_case sb_plain /usr/share/OVMF/OVMF_CODE_4M.fd /usr/share/OVMF/OVMF_VARS_4M.fd "-machine q35" \
    "33 = sbstate ran and wrote 0x10 to isa-debug-exit"
SMM="-machine q35,smm=on -global driver=cfi.pflash01,property=secure,value=on"
run_case sb_nokeys /usr/share/OVMF/OVMF_CODE_4M.secboot.fd /usr/share/OVMF/OVMF_VARS_4M.fd "$SMM" \
    "33 = sbstate ran and wrote 0x10 to isa-debug-exit"
run_case sb_enrolled /usr/share/OVMF/OVMF_CODE_4M.secboot.fd /usr/share/OVMF/OVMF_VARS_4M.ms.fd "$SMM" \
    "124 = the harness stopped QEMU after the firmware's 'Press any key' line; sbstate never ran"
grep -q "SecureBoot" sb_plain.out || status=1
grep -q "Access Denied" sb_enrolled.out || status=1

# 2. is our program signed? (a PE/COFF signature lives in the certificate table data directory)
cp $B/esp/EFI/BOOT/BOOTX64.EFI $B/unsigned.efi
llvm-readobj --file-headers $B/unsigned.efi 2>&1 | grep -E 'CertificateTable|Subsystem:' > signature.out; rc=${PIPESTATUS[0]}
READOBJV="llvm-readobj ($(llvm-readobj --version | head -n 1 | sed 's/^ *//'))"
rec signature "sbstate.efi (unsigned build of sbstate.cc)" "$READOBJV" \
    "llvm-readobj --file-headers sbstate.efi | grep -E 'CertificateTable|Subsystem:'" "$rc"

# 4. our own signature: sign.py signs sbstate.efi with the distribution's test ("snakeoil") key,
#    whose certificate the OVMF_VARS_4M.snakeoil.fd variable store has enrolled. The private key
#    file ships with the ovmf package, protected by the passphrase "snakeoil".
KEY=/usr/share/ovmf/PkKek-1-snakeoil.key
CERT=/usr/share/ovmf/PkKek-1-snakeoil.pem
{
    python3 sign.py $B/unsigned.efi $B/signed.efi $CERT $KEY snakeoil &&
    echo "the test certificate:" && openssl x509 -in $CERT -noout -subject -enddate -fingerprint -sha256 | sed 's/^/  /' &&
    echo "llvm-readobj of the signed file:" &&
    llvm-readobj --file-headers $B/signed.efi | grep -E 'CertificateTable'
} > sign.out 2>&1; rc=$?
rec sign "sign.py" "$(python3 --version); python3-cryptography $(python3 -c 'import cryptography; print(cryptography.__version__)'); $(openssl version); $READOBJV" \
    "python3 sign.py sbstate.efi signed.efi PkKek-1-snakeoil.pem PkKek-1-snakeoil.key snakeoil; openssl x509 -in PkKek-1-snakeoil.pem -noout -subject -enddate -fingerprint -sha256; llvm-readobj --file-headers signed.efi | grep CertificateTable" "$rc"
[ "$rc" = 0 ] || status=1
# the tampered copy: one byte of the program's message text changed after signing ("F3-16" -> "F3-17")
python3 -c "import sys; d=bytearray(open(sys.argv[1],'rb').read()); i=d.index(b'F3-16 sbstate: Secure'); d[i+4]=ord('7'); open(sys.argv[2],'wb').write(d)" \
    $B/signed.efi $B/tampered.efi
SNAKE=/usr/share/OVMF/OVMF_VARS_4M.snakeoil.fd
for v in unsigned signed tampered; do
    cp $B/$v.efi $B/esp/EFI/BOOT/BOOTX64.EFI
    if [ $v = signed ]; then why="33 = sbstate ran and wrote 0x10 to isa-debug-exit"; else why="124 = the harness stopped QEMU after the firmware's 'Press any key' line; sbstate never ran"; fi
    run_case sb_snakeoil_$v /usr/share/OVMF/OVMF_CODE_4M.secboot.fd $SNAKE "$SMM" "$why"
done
grep -q "Access Denied" sb_snakeoil_unsigned.out || status=1
grep -q "SecureBoot = 1" sb_snakeoil_signed.out || status=1
grep -q "Access Denied" sb_snakeoil_tampered.out || status=1
{
    echo "Authenticode SHA-256 (sign.py --hash):"
    for v in unsigned signed tampered; do printf '  %-9s %s\n' $v "$(python3 sign.py --hash $B/$v.efi)"; done
    echo "plain SHA-256 of the whole file (sha256sum):"
    for v in unsigned signed tampered; do printf '  %-9s %s\n' $v "$(sha256sum $B/$v.efi | cut -d' ' -f1)"; done
    echo "bytes that differ between signed.efi and tampered.efi (cmp -l: offset, octal values):"
    { cmp -l $B/signed.efi $B/tampered.efi || true; } | sed 's/^/  /'
} > forensic_hashes.out 2>&1; rc=$?
rec forensic_hashes "sign.py --hash; sha256sum; cmp" "$(python3 --version); $(sha256sum --version | head -n 1); $(cmp --version | head -n 1)" \
    "python3 sign.py --hash <file>; sha256sum <file>; cmp -l signed.efi tampered.efi (for unsigned.efi, signed.efi, tampered.efi)" "$rc"

# 3. measured-boot arithmetic on three real files: firmware image, a UEFI program, a kernel
g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined measure.cc -o $B/measure || status=1
KFLAGS="-std=c++20 -ffreestanding -fno-exceptions -fno-rtti -nostdlib -fno-pic -fno-pie -mcmodel=kernel -mno-red-zone -mgeneral-regs-only -fno-stack-protector -O2"
g++ $KFLAGS -I ../F3-14 -c ../F3-14/kernel.cc -o $B/kernel.o && ld.lld -T ../F3-14/kernel.ld -z max-page-size=4096 -nostdlib -static -o $B/kernel.elf $B/kernel.o || status=1
cp /usr/share/OVMF/OVMF_CODE_4M.fd $B/firmware.fd
cp $B/unsigned.efi $B/loader.efi
(cd $B && ./measure firmware.fd loader.efi kernel.elf) > measure.out 2>&1; rc=$?
rec measure measure.cc "$(g++ --version | head -n 1)" \
    "g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined measure.cc -o measure; ./measure firmware.fd loader.efi kernel.elf" "$rc"
[ "$rc" = 0 ] || status=1
{
    echo "sha256sum (GNU coreutils) of the same files:"
    (cd $B && sha256sum firmware.fd loader.efi kernel.elf)
    echo "Python hashlib, extending 32 zero bytes with each digest in order:"
    (cd $B && python3 -c "
import hashlib, sys
pcr = bytes(32)
for name in sys.argv[1:]:
    pcr = hashlib.sha256(pcr + hashlib.sha256(open(name, 'rb').read()).digest()).digest()
    print('after', name, pcr.hex())
" firmware.fd loader.efi kernel.elf)
} > crosscheck.out 2>&1; rc=$?
rec crosscheck "(no listing: sha256sum and an inline Python extend loop, shown in run.sh)" \
    "$(sha256sum --version | head -n 1); $(python3 --version)" "sha256sum firmware.fd loader.efi kernel.elf; python3 -c '<extend loop>' firmware.fd loader.efi kernel.elf" "$rc"
for f in firmware.fd loader.efi kernel.elf; do
    d=$(cd $B && sha256sum $f | cut -d' ' -f1)
    grep -q "^$d  $f" measure.out || status=1
done
last_c=$(grep "^after" crosscheck.out | tail -n 1 | awk '{print $3}')
grep -q "after extend 3       $last_c" measure.out || status=1

# 5. reproducibility: link the same object twice, one second apart, without and with /Brepro
{
    $UEFI_LD /out:$B/r1.efi $B/sbstate.o > /dev/null && sleep 1.1 && $UEFI_LD /out:$B/r2.efi $B/sbstate.o > /dev/null &&
    echo "two links of the same sbstate.o, 1.1 s apart:" &&
    { sha256sum $B/r1.efi $B/r2.efi | sed "s#$B/##"; } &&
    echo "differing bytes (cmp -l: offset, octal values):" &&
    { cmp -l $B/r1.efi $B/r2.efi || true; } &&
    { llvm-readobj --file-headers $B/r1.efi | grep TimeDateStamp; } &&
    $UEFI_LD /Brepro /out:$B/p1.efi $B/sbstate.o > /dev/null && sleep 1.1 && $UEFI_LD /Brepro /out:$B/p2.efi $B/sbstate.o > /dev/null &&
    echo "the same two links with /Brepro:" &&
    { sha256sum $B/p1.efi $B/p2.efi | sed "s#$B/##"; } &&
    { cmp $B/p1.efi $B/p2.efi && echo "identical"; }
} > repro.out 2>&1; rc=$?
rec repro "sbstate.cc (object file from step 1)" "$CLANGV; $READOBJV" \
    "$UEFI_LD /out:r1.efi sbstate.o; sleep 1.1; (same) /out:r2.efi; sha256sum; cmp -l; then the same with /Brepro" "$rc"
[ "$rc" = 0 ] || status=1

rm -rf "$B"
exit $status
