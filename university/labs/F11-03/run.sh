#!/usr/bin/env bash
# F11-03 lab: measured boot and attestation.
# Part 1, the university's TPM model (no real TPM, no swtpm, no tpm2-tools in this build):
#   measured   six scenarios (tpm_model.h + measured.cc, OpenSSL for the ECDSA attestation key)
#   crosscheck Python's hashlib recomputes the healthy boot's PCR values from the same strings
#   forensic   the evidence pack of the forensic lab
# Part 2, real firmware measuring into a stand-in TPM: OVMF (Secure Boot build) on QEMU's tpm-tis
# device, whose "emulator" backend is mock_tpm.py (copied from DR302's F4-13 lab). evlog.efi reads
# the firmware's event log through the TCG2 protocol and replays it; the mock records every extend.
#   fw_empty     empty key store (setup mode), unsigned evlog
#   fw_enrolled  F11-02's keys enrolled (F11-02's mkvars.py and setvars.cc), evlog signed with db1
#   fw_rotated   F11-02's rotation applied on top, evlog signed with db2
#   pcr_compare  replay vs the mock TPM's PCRs; PCR 7 across the three; PCR 4 vs the Authenticode hash
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
FLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
TC="$(g++ --version | head -n 1); $(openssl version)"
NOTE="hardware:  untested on hardware and with no TPM: a model of PCRs, quotes and sealing, not the TCG formats or commands"
g++ $FLAGS measured.cc -lcrypto -o $B/measured > $B/build.txt 2>&1 || { cat $B/build.txt; status=1; }
$B/measured demo > measured.out 2>&1; rc=$?
rec measured "tpm_model.h measured.cc" "$TC" "g++ $FLAGS measured.cc -lcrypto -o measured && ./measured demo" "$rc" "$NOTE"
[ "$rc" = 0 ] || status=1
$B/measured forensic > forensic.out 2>&1; rc=$?
rec forensic "tpm_model.h measured.cc" "$TC" "./measured forensic" "$rc" "$NOTE"
[ "$rc" = 0 ] || status=1

python3 - measured.out > crosscheck.out 2>&1 <<'PY'
import hashlib, re, sys
H = lambda b: hashlib.sha256(b).digest()
pcr = {i: bytes(32) for i in range(10)}
def extend(i, text):
    pcr[i] = H(pcr[i] + H(text.encode()))
extend(0, "firmware build 2026.04")
extend(7, "SecureBoot = 1"); extend(7, "db: SS301 lab signing key 2026"); extend(7, "dbx: (empty)")
extend(1, "Boot0001 = disk")
for i in (0, 1, 4, 7):
    extend(i, "end of firmware measurements")
extend(4, "loader v3 (signed by db1)")
extend(8, "kernel 1.8")
text = open(sys.argv[1]).read().split("2. Secure Boot")[0]
ok = True
for i in (0, 1, 4, 7, 8):
    got = re.search(r"PCR %d = ([0-9a-f]{16})" % i, text).group(1)
    want = pcr[i].hex()[:16]
    print("PCR %d: measured.out %s..., Python hashlib %s... %s" % (i, got, want, "equal" if got == want else "DIFFERENT"))
    ok = ok and got == want
sys.exit(0 if ok else 1)
PY
rc=$?
rec crosscheck "(no listing: an inline Python replay, shown in run.sh)" "$(python3 --version)" "python3 - measured.out" "$rc"
[ "$rc" = 0 ] || status=1

# ---- part 2: OVMF measuring into mock_tpm.py ----
F2=../F11-02
QEMU=qemu-system-x86_64
QEMUV="$($QEMU --version | head -n 1)"
OVMFV="OVMF from the ovmf package $(dpkg-query -W -f '${Version}' ovmf)"
CLANGV="$(clang++ --version | head -n 1); $(lld-link --version | head -n 1)"
FWHW="hardware:  untested on hardware and with no real TPM: QEMU 8.2.2 q35 + tpm-tis, TPM played by mock_tpm.py (no keys, quotes or sealing)"
UEFI_CXX="clang++ --target=x86_64-unknown-windows -std=c++20 -ffreestanding -fno-exceptions -fno-rtti -mno-red-zone -fno-stack-protector -O2 -Wall -Wextra -Wpedantic -Werror -I $F2 -I ."
UEFI_LD="lld-link /subsystem:efi_application /entry:efi_main /nodefaultlib /Brepro"
mkdir -p $B/esp/EFI/BOOT
efi() { $UEFI_CXX -c "$1" -o $B/tmp.o && $UEFI_LD /out:"$2" $B/tmp.o > /dev/null || { echo "F11-03: build of $1 failed"; status=1; }; }
QBASE="-machine q35,smm=on -global driver=cfi.pflash01,property=secure,value=on -m 256M -display none -serial stdio -no-reboot -net none -drive if=pflash,format=raw,readonly=on,file=/usr/share/OVMF/OVMF_CODE_4M.secboot.fd -drive format=raw,file=fat:rw:$B/esp -device isa-debug-exit,iobase=0xf4,iosize=0x04"
TPMARGS="-chardev socket,id=chrtpm,path=$B/ctrl.sock -tpmdev emulator,id=tpm0,chardev=chrtpm -device tpm-tis,tpmdev=tpm0"
plain_boot() {  # plain_boot <efi> <vars file>: no TPM, output discarded (key changes only)
    cp "$1" $B/esp/EFI/BOOT/BOOTX64.EFI
    python3 $F2/qemu_run.py --timeout 60 --stop-after "Press any key to enter the Boot Manager" -- $QEMU $QBASE \
        -drive if=pflash,format=raw,file="$2" > $B/plain.txt 2>&1
    local rc=$?; [ "$rc" = 33 ] || { echo "F11-03: key-change boot of $1 exited $rc"; cat $B/plain.txt; status=1; }
}
tpm_boot() {  # tpm_boot <name> <efi> <vars file> <what>
    cp "$2" $B/esp/EFI/BOOT/BOOTX64.EFI
    rm -f $B/ctrl.sock $B/mock.txt
    python3 mock_tpm.py "$PWD/$B/ctrl.sock" $B/mock.txt &
    local mpid=$!
    for i in $(seq 50); do [ -S $B/ctrl.sock ] && break; python3 -c 'import time; time.sleep(0.1)'; done
    python3 $F2/qemu_run.py --timeout 60 --stop-after "Press any key to enter the Boot Manager" -- $QEMU $QBASE \
        -drive if=pflash,format=raw,file="$3" $TPMARGS 2>&1 | grep -v -E '^\s*$|^BdsDxe' > "$1.out"; local rc=${PIPESTATUS[0]}
    wait $mpid 2>/dev/null
    { echo "== mock_tpm.py, after QEMU exited: the PCRs the TPM holds =="; grep -A9 'final SHA-256' $B/mock.txt | sed 's/^mock-tpm: //'; } >> "$1.out"
    rec "$1" "evlog.cc ($4)" "$CLANGV; $QEMUV; $OVMFV; $(python3 --version)" \
        "python3 mock_tpm.py ctrl.sock mock.txt & ; python3 qemu_run.py --timeout 60 -- $QEMU -machine q35,smm=on -global driver=cfi.pflash01,property=secure,value=on -m 256M -display none -serial stdio -no-reboot -net none -drive if=pflash,format=raw,readonly=on,file=OVMF_CODE_4M.secboot.fd -drive if=pflash,format=raw,file=vars.fd -drive format=raw,file=fat:rw:esp -device isa-debug-exit,iobase=0xf4,iosize=0x04 $TPMARGS" \
        "$rc (33 = evlog ran to the end and wrote 0x10 to isa-debug-exit)" "$FWHW"
    [ "$rc" = 33 ] || { echo "F11-03: $1 exited $rc"; status=1; }
}
efi evlog.cc $B/evlog.efi
cp $F2/setvars.cc $B/setvars.cc
for sc in enroll rotate; do
    python3 $F2/mkvars.py $sc $B/updates.inc > /dev/null && efi $B/setvars.cc $B/$sc.efi || status=1
done
python3 $F2/sign.py $B/evlog.efi $B/evlog.db1.efi $F2/keys/db1.crt $F2/keys/db1.key - > /dev/null || status=1
python3 $F2/sign.py $B/evlog.efi $B/evlog.db2.efi $F2/keys/db2.crt $F2/keys/db2.key - > /dev/null || status=1
python3 $F2/sign.py $B/rotate.efi $B/rotate.db1.efi $F2/keys/db1.crt $F2/keys/db1.key - > /dev/null || status=1

cp /usr/share/OVMF/OVMF_VARS_4M.fd $B/vars_empty.fd
tpm_boot fw_empty $B/evlog.efi $B/vars_empty.fd "unsigned; empty key store"
cp /usr/share/OVMF/OVMF_VARS_4M.fd $B/vars.fd
plain_boot $B/enroll.efi $B/vars.fd
tpm_boot fw_enrolled $B/evlog.db1.efi $B/vars.fd "signed with F11-02's db1; F11-02's keys enrolled"
plain_boot $B/rotate.db1.efi $B/vars.fd
tpm_boot fw_rotated $B/evlog.db2.efi $B/vars.fd "signed with F11-02's db2; F11-02's rotation applied"

python3 - fw_empty.out fw_enrolled.out fw_rotated.out \
    "$(python3 $F2/sign.py --hash $B/evlog.efi)" "$(python3 $F2/sign.py --hash $B/evlog.db1.efi)" "$(python3 $F2/sign.py --hash $B/evlog.db2.efi)" \
    > pcr_compare.out 2>&1 <<'PY'
import re, sys
names = ["fw_empty", "fw_enrolled", "fw_rotated"]
files, hashes = sys.argv[1:4], sys.argv[4:7]
ok = True
table = {}
print("1. Does replaying the firmware's event log give the PCR values the TPM holds?")
for name, f in zip(names, files):
    text = open(f).read()
    replay = dict(re.findall(r"^\s+PCR (\d) ([0-9a-f]{64})$", text, re.M))
    tpm = dict(re.findall(r"^\s+PCR\s+(\d) ([0-9a-f]{64})$", text.split("== mock_tpm.py")[1], re.M))
    same = all(replay[str(i)] == tpm[str(i)] for i in range(8))
    ok = ok and same
    table[name] = replay
    print("   %-12s PCR 0-7 replayed = PCR 0-7 in the TPM: %s" % (name, "yes, all eight" if same else "NO"))
print("2. PCR values (first 16 hex digits) in the three runs")
print("   PCR  fw_empty          fw_enrolled       fw_rotated")
for i in range(8):
    row = [table[n][str(i)][:16] for n in names]
    print("   %d    %s  %s  %s%s" % (i, row[0], row[1], row[2], "" if len(set(row)) == 1 else "   <- differs"))
print("3. The last PCR 4 event (the booted program) vs sign.py --hash of the file booted")
for name, f, h in zip(names, files, hashes):
    ev = re.findall(r"^\s+\d+\s+4\s+EV_EFI_BOOT_SERVICES_APPLICATION\s+([0-9a-f]{16})", open(f).read(), re.M)
    match = bool(ev) and h.startswith(ev[-1])
    ok = ok and match
    print("   %-12s event %s...  Authenticode SHA-256 %s...  %s" % (name, ev[-1] if ev else "none", h[:16], "equal" if match else "DIFFERENT"))
sys.exit(0 if ok else 1)
PY
rc=$?
rec pcr_compare "(no listing: an inline Python comparison, shown in run.sh; F11-02's sign.py --hash)" "$(python3 --version)" \
    "python3 - fw_empty.out fw_enrolled.out fw_rotated.out <three Authenticode hashes>" "$rc"
[ "$rc" = 0 ] || status=1

rm -rf "$B"
exit $status
