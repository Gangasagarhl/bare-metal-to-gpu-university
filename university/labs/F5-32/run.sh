#!/usr/bin/env bash
# F5-32 lab: what a guest can see of its hypervisor, and what acceleration costs.
# 1. build guest_cpuid.cc (UEFI) and work_host.cc (host, -O2);
# 2. boot the guest under QEMU TCG, then with the hypervisor CPUID bit removed;
# 3. try KVM (this container has no /dev/kvm: the failure is the expected, recorded result);
# 4. forensic evidence: "-accel kvm -accel tcg" (falls back) with host time stamps, plus the
#    same work measured directly on this machine.
# Every step writes <name>.out (real output) and <name>.log (run record).
set -u -o pipefail
cd "$(dirname "$0")"
status=0
B=.build
rm -rf "$B"; mkdir -p "$B/esp/EFI/BOOT"
QEMU=qemu-system-x86_64
QEMUV="$($QEMU --version | head -n 1)"
CLANGV="$(clang++ --version | head -n 1); $(lld-link --version | head -n 1)"
OVMFV="OVMF from the ovmf package $(dpkg-query -W -f '${Version}' ovmf)"
GXXV="$(g++ --version | head -n 1)"
CODE=/usr/share/OVMF/OVMF_CODE_4M.fd
EMU="hardware:  untested on hardware; QEMU 8.2.2 q35 machine with the distribution's OVMF inside this container (itself a virtual machine)"
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
HOST_CXX="g++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror"

# 1. build
$UEFI_CXX -c guest_cpuid.cc -o $B/g.o > $B/build.txt 2>&1 &&
    $UEFI_LD /out:$B/esp/EFI/BOOT/BOOTX64.EFI $B/g.o >> $B/build.txt 2>&1 &&
    $HOST_CXX work_host.cc -o $B/work_host >> $B/build.txt 2>&1 || { cat $B/build.txt; status=1; }

boot() {  # boot <name> <expected exit> <stamp: yes/no> <qemu options...>
    local name="$1" want="$2" stamp="$3"; shift 3
    local st=""; [ "$stamp" = yes ] && st="--stamp"
    cp /usr/share/OVMF/OVMF_VARS_4M.fd $B/vars.fd
    python3 ../F3-10/qemu_run.py --timeout 120 $st -- $QEMU -machine q35 "$@" -m 256M -display none \
        -serial stdio -no-reboot -net none -drive if=pflash,format=raw,readonly=on,file=$CODE \
        -drive if=pflash,format=raw,file=$B/vars.fd -drive format=raw,file=fat:rw:$B/esp \
        -device isa-debug-exit,iobase=0xf4,iosize=0x04 > "$name.out" 2>&1
    local rc=$?
    local meaning="33 = the program finished and wrote 0x10 to isa-debug-exit"
    [ "$want" = 1 ] && meaning="1 = QEMU refused to start; expected here because this container has no /dev/kvm"
    rec "$name" "guest_cpuid.cc console.hpp efi.hpp (F3-10)" "$CLANGV; $QEMUV; $OVMFV" \
        "$UEFI_CXX -c guest_cpuid.cc && $UEFI_LD /out:BOOTX64.EFI g.o; python3 ../F3-10/qemu_run.py --timeout 120 $st -- $QEMU -machine q35 $* -m 256M -display none -serial stdio -no-reboot -net none -drive if=pflash,...OVMF_CODE_4M.fd -drive if=pflash,...OVMF_VARS_4M.fd(copy) -drive format=raw,file=fat:rw:esp -device isa-debug-exit,iobase=0xf4,iosize=0x04" \
        "$rc ($meaning)" "$EMU"
    [ "$rc" = "$want" ] || status=1
}

# 2. TCG, and TCG without the hypervisor bit
boot guest_tcg 33 no
boot guest_nohv 33 no -cpu qemu64,hypervisor=off
# 3. KVM only
boot accel_kvm 1 no -accel kvm
# 4. forensic: KVM requested with TCG as a fallback, time-stamped; and the same work on the host
boot forensic_accel 33 yes -accel kvm -accel tcg
$B/work_host > work_host.out 2>&1; rc=$?
rec work_host "work_host.cc" "$GXXV" "$HOST_CXX work_host.cc -o work_host && ./work_host" "$rc" \
    "note:      a measurement in this shared cloud container (itself a KVM guest), not a property of any processor"
[ "$rc" = 0 ] || status=1
{
    g=$(sed -n 's/.*guest: integer work end, result \(0x[0-9a-f]*\).*/\1/p' forensic_accel.out)
    h=$(sed -n 's/^host: integer work result \(0x[0-9a-f]*\),.*/\1/p' work_host.out)
    echo "integer: guest result $g, host result $h: $([ -n "$g" ] && [ "$g" = "$h" ] && echo 'PASS (same computation)' || echo FAIL)"
    g=$(sed -n 's/.*guest: floating-point work end, result bits \(0x[0-9a-f]*\).*/\1/p' forensic_accel.out)
    h=$(sed -n 's/^host: floating-point work result bits \(0x[0-9a-f]*\),.*/\1/p' work_host.out)
    echo "floating point: guest bits $g, host bits $h: $([ -n "$g" ] && [ "$g" = "$h" ] && echo 'PASS (bit-identical)' || echo FAIL)"
    grep -q 'signature "TCGTCGTCGTCG"' guest_tcg.out && r=PASS || r=FAIL
    echo "TCG guest reports signature TCGTCGTCGTCG: $r"
    grep -q "hypervisor present) = 0" guest_nohv.out && r=PASS || r=FAIL
    echo "hypervisor=off hides the bit: $r"
} > check_guest.out
rec check_guest "(no listing: grep and sed on the outputs above)" "$(grep --version | head -n 1)" "checks written in run.sh step 4" "0"
grep -q FAIL check_guest.out && status=1

rm -rf "$B"
exit $status
