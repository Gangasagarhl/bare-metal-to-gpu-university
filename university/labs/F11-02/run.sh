#!/usr/bin/env bash
# F11-02 lab: enroll our own Secure Boot keys in OVMF, sign our program, rotate the signing key.
# One firmware variable store (a copy of the distribution's empty OVMF_VARS_4M.fd) is carried
# through all steps, so each step sees what the previous steps wrote:
#   keys        our four test keys (made once, kept in keys/) and their fingerprints
#   sb_setup    sbinfo, unsigned, on the empty store: setup mode, nothing enforced
#   enroll      setvars.efi writes db, KEK, PK (signed updates from mkvars.py enroll)
#   sb_unsigned the unsigned sbinfo is refused now
#   sign_db1    sign.py signs sbinfo with our db1 key;  sb_db1: it runs and lists the databases
#   sb_snakeoil sbinfo signed with the distribution's test key (valid signature, unknown signer)
#   forged      an update of db signed with db1 instead of KEK
#   replay      the enrolment's db update sent again (old time stamp)
#   rotate      db += db2, dbx += db1 certificate (signed by KEK)
#   forensic_boot, forensic_signer: the loader nobody re-signed, and who signed it
#   sb_db2      sbinfo re-signed with db2 runs; the listing shows the rotation
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
PYV="$(python3 --version); python3-cryptography $(python3 -c 'import cryptography; print(cryptography.__version__)')"
EMU="hardware:  untested on hardware; QEMU 8.2.2 q35 machine with SMM and the distribution's Secure Boot OVMF build, not a real PC"
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
UEFI_CXX="clang++ --target=x86_64-unknown-windows -std=c++20 -ffreestanding -fno-exceptions -fno-rtti -mno-red-zone -fno-stack-protector -O2 -Wall -Wextra -Wpedantic -Werror -I ."
UEFI_LD="lld-link /subsystem:efi_application /entry:efi_main /nodefaultlib /Brepro"
CODE=/usr/share/OVMF/OVMF_CODE_4M.secboot.fd
cp /usr/share/OVMF/OVMF_VARS_4M.fd $B/vars.fd
QARGS="-machine q35,smm=on -global driver=cfi.pflash01,property=secure,value=on -m 256M -display none -serial stdio -no-reboot -net none -drive if=pflash,format=raw,readonly=on,file=$CODE -drive if=pflash,format=raw,file=$B/vars.fd -drive format=raw,file=fat:rw:$B/esp -device isa-debug-exit,iobase=0xf4,iosize=0x04"
QSHOW="$QEMU -machine q35,smm=on -global driver=cfi.pflash01,property=secure,value=on -m 256M -display none -serial stdio -no-reboot -net none -drive if=pflash,format=raw,readonly=on,file=OVMF_CODE_4M.secboot.fd -drive if=pflash,format=raw,file=vars.fd -drive format=raw,file=fat:rw:esp -device isa-debug-exit,iobase=0xf4,iosize=0x04"
RAN="33 = the program ran and wrote 0x10 to isa-debug-exit"
REFUSED="124 = the harness stopped QEMU after the firmware's 'Press any key' line; the program never ran"

boot() {  # boot <name> <efi file> <listing> <expected: ran|refused>
    cp "$2" $B/esp/EFI/BOOT/BOOTX64.EFI
    python3 qemu_run.py --timeout 60 --stop-after "Press any key to enter the Boot Manager" -- $QEMU $QARGS 2>&1 \
        | grep -v -E '^\s*$' > "$1.out"; local rc=${PIPESTATUS[0]}
    local why="$RAN"; [ "$rc" = 124 ] && why="$REFUSED"
    rec "$1" "$3 (as \\EFI\\BOOT\\BOOTX64.EFI)" "$CLANGV; $QEMUV; $OVMFV" \
        "python3 qemu_run.py --timeout 60 --stop-after 'Press any key to enter the Boot Manager' -- $QSHOW" "$rc ($why)" "$EMU"
    if [ "$4" = ran ] && [ "$rc" != 33 ]; then echo "F11-02: $1 should have run (exit $rc)"; status=1; fi
    if [ "$4" = refused ] && [ "$rc" != 124 ]; then echo "F11-02: $1 should have been refused (exit $rc)"; status=1; fi
}
efi() {  # efi <source.cc> <output.efi>
    $UEFI_CXX -c "$1" -o $B/tmp.o && $UEFI_LD /out:"$2" $B/tmp.o > /dev/null || { echo "F11-02: build of $1 failed"; status=1; }
}
sign() {  # sign <in> <out> <key name>
    python3 sign.py "$1" "$2" keys/$3.crt keys/$3.key - > /dev/null || { echo "F11-02: signing failed"; status=1; }
}
updates() {  # updates <scenario> <efi out>: mkvars.py writes updates.inc, setvars.cc is rebuilt with it
    python3 mkvars.py "$1" $B/updates.inc > $B/$1.mkvars.txt || status=1
    cp $B/updates.inc updates.inc && efi setvars.cc "$2"; rm -f updates.inc
}

# keys: made once (random RSA keys), then kept in keys/ so the fingerprints in the chapter stay valid
python3 mkvars.py keys
{
    for k in PK KEK db1 db2; do
        printf '%-4s ' $k; openssl x509 -in keys/$k.crt -noout -subject -fingerprint -sha256 | tr '\n' ' ' | sed 's/sha256 Fingerprint=/fingerprint /'; echo
    done
} > keys.out 2>&1; rc=$?
rec keys "mkvars.py keys" "$PYV; $(openssl version)" "python3 mkvars.py keys; openssl x509 -in keys/<name>.crt -noout -subject -fingerprint -sha256" "$rc"

efi sbinfo.cc $B/sbinfo.efi
boot sb_setup $B/sbinfo.efi "sbinfo.cc, unsigned" ran

updates enroll $B/enroll.efi
{ echo "mkvars.py enroll:"; sed 's/^/  /' $B/enroll.mkvars.txt; } > $B/enroll.head
boot enroll $B/enroll.efi "setvars.cc built with mkvars.py enroll (unsigned: the store is still in setup mode)" ran
{ cat $B/enroll.head; cat enroll.out; } > $B/x && mv $B/x enroll.out
grep -q "SecureBoot = 1" enroll.out || status=1

boot sb_unsigned $B/sbinfo.efi "sbinfo.cc, unsigned" refused

{ python3 sign.py $B/sbinfo.efi $B/sbinfo.db1.efi keys/db1.crt keys/db1.key - && python3 sign.py --signer $B/sbinfo.db1.efi; } > sign_db1.out 2>&1; rc=$?
rec sign_db1 "sign.py" "$PYV" "python3 sign.py sbinfo.efi sbinfo.db1.efi keys/db1.crt keys/db1.key -; python3 sign.py --signer sbinfo.db1.efi" "$rc"
[ "$rc" = 0 ] || status=1
boot sb_db1 $B/sbinfo.db1.efi "sbinfo.cc, signed with db1" ran

python3 sign.py $B/sbinfo.efi $B/sbinfo.snakeoil.efi /usr/share/ovmf/PkKek-1-snakeoil.pem /usr/share/ovmf/PkKek-1-snakeoil.key snakeoil > /dev/null || status=1
boot sb_snakeoil $B/sbinfo.snakeoil.efi "sbinfo.cc, signed with the ovmf package's test key (not in our db)" refused

updates forged $B/forged.efi; sign $B/forged.efi $B/forged.db1.efi db1
boot forged $B/forged.db1.efi "setvars.cc built with mkvars.py forged, signed with db1" ran
grep -q "Security Violation" forged.out || status=1

updates replay $B/replay.efi; sign $B/replay.efi $B/replay.db1.efi db1
boot replay $B/replay.db1.efi "setvars.cc built with mkvars.py replay, signed with db1" ran
grep -q "Security Violation" replay.out || status=1

updates rotate $B/rotate.efi; sign $B/rotate.efi $B/rotate.db1.efi db1
boot rotate $B/rotate.db1.efi "setvars.cc built with mkvars.py rotate, signed with db1" ran
[ "$(grep -c -- '-> Success' rotate.out)" = 2 ] || status=1

# the forensic case: the boot program on the disk is still the one signed with db1
boot forensic_boot $B/sbinfo.db1.efi "sbinfo.cc, signed with db1 before the rotation" refused
{ echo "EFI/BOOT/BOOTX64.EFI on the disk:"; python3 sign.py --signer $B/sbinfo.db1.efi | sed 's/^/  /'; } > forensic_signer.out 2>&1; rc=$?
rec forensic_signer "sign.py --signer" "$PYV" "python3 sign.py --signer BOOTX64.EFI" "$rc"
[ "$rc" = 0 ] || status=1

sign $B/sbinfo.efi $B/sbinfo.db2.efi db2
boot sb_db2 $B/sbinfo.db2.efi "sbinfo.cc, signed with db2" ran
grep -q "dbx: 792 bytes" sb_db2.out || status=1

rm -rf "$B"
exit $status
