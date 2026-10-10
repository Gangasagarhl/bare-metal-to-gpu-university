#!/usr/bin/env bash
# F5-28 lab: the shape of a (virtual) server as the firmware describes it to the OS.
# 1. build numa.cc as a UEFI application (same route as F3-10/F3-12);
# 2. boot it on a two-socket, two-node QEMU machine and on a one-node machine;
# 3. check the printed tables against the QEMU command lines;
# 4. forensic evidence: the same machine with all memory attached to node 0.
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
CODE=/usr/share/OVMF/OVMF_CODE_4M.fd
EMU="hardware:  untested on hardware; QEMU 8.2.2 q35 machine with the distribution's OVMF, not a real server"
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

# 0. QEMU's own description of its NUMA options (the options used below)
$QEMU -help | grep -E "^-numa " > qemu_numa_help.out; rc=$?
rec qemu_numa_help "(no listing: QEMU's own help text)" "$QEMUV" "$QEMU -help | grep -E '^-numa '" "$rc"

# 1. build
$UEFI_CXX -c numa.cc -o $B/numa.o > $B/build.txt 2>&1 &&
    $UEFI_LD /out:$B/esp/EFI/BOOT/BOOTX64.EFI $B/numa.o >> $B/build.txt 2>&1 || { cat $B/build.txt; status=1; }

boot() {  # boot <name> <machine options...>
    local name="$1"; shift
    cp /usr/share/OVMF/OVMF_VARS_4M.fd $B/vars.fd
    python3 ../F3-10/qemu_run.py --timeout 90 -- $QEMU -machine q35 "$@" -display none -serial stdio \
        -no-reboot -net none -drive if=pflash,format=raw,readonly=on,file=$CODE \
        -drive if=pflash,format=raw,file=$B/vars.fd -drive format=raw,file=fat:rw:$B/esp \
        -device isa-debug-exit,iobase=0xf4,iosize=0x04 > "$name.out" 2>&1
    local rc=$?
    rec "$name" "numa.cc console.hpp efi.hpp (F3-10)" "$CLANGV; $QEMUV; $OVMFV" \
        "$UEFI_CXX -c numa.cc && $UEFI_LD /out:BOOTX64.EFI numa.o; python3 ../F3-10/qemu_run.py --timeout 90 -- $QEMU -machine q35 $* -display none -serial stdio -no-reboot -net none -drive if=pflash,...OVMF_CODE_4M.fd -drive if=pflash,...OVMF_VARS_4M.fd(copy) -drive format=raw,file=fat:rw:esp -device isa-debug-exit,iobase=0xf4,iosize=0x04" \
        "$rc (33 = the application wrote 0x10 to isa-debug-exit after printing everything)" "$EMU"
    [ "$rc" = 33 ] || status=1
}

# 2. two sockets, two NUMA nodes of 256 MiB each, distance 21 between them
TWO="-smp 4,sockets=2,cores=2,threads=1 -m 512M \
-object memory-backend-ram,id=m0,size=256M -object memory-backend-ram,id=m1,size=256M \
-numa node,nodeid=0,cpus=0-1,memdev=m0 -numa node,nodeid=1,cpus=2-3,memdev=m1 \
-numa dist,src=0,dst=1,val=21"
boot numa_2s $TWO
#    the same processors and memory without any -numa option
boot numa_flat -smp 4,sockets=2,cores=2,threads=1 -m 512M

# 3. compare with the command lines
{
    c=$(grep -c "^  CPU   APIC ID [01] -> node 0$" numa_2s.out); d=$(grep -c "^  CPU   APIC ID [23] -> node 1$" numa_2s.out)
    echo "CPUs 0-1 on node 0 and 2-3 on node 1 (cpus=0-1 / cpus=2-3): $([ "$c$d" = 22 ] && echo PASS || echo FAIL)"
    grep -q "^  node 0: 10 21$" numa_2s.out && grep -q "^  node 1: 21 10$" numa_2s.out && r=PASS || r=FAIL
    echo "SLIT rows 10 21 / 21 10 (-numa dist,...,val=21): $r"
    n1=$(sed -n 's/^node 1: 2 CPUs, \([0-9]*\) KiB of memory$/\1/p' numa_2s.out)
    echo "node 1 memory $n1 KiB = 256 MiB (memdev m1): $([ "$n1" = 262144 ] && echo PASS || echo FAIL)"
    n0=$(sed -n 's/^node 0: 2 CPUs, \([0-9]*\) KiB of memory$/\1/p' numa_2s.out)
    echo "node 0 memory $n0 KiB: less than 262144 by $((262144 - ${n0:-0})) KiB (see the gap in its MEM lines)"
    s=$(grep -c "^  type 4  processor socket" numa_2s.out)
    echo "SMBIOS type 4 structures: $s (sockets=2): $([ "$s" = 2 ] && echo PASS || echo FAIL)"
    grep -q "^no SRAT" numa_flat.out && r=PASS || r=FAIL
    echo "without -numa: no SRAT, one node assumed: $r"
    grep -q "checksum BAD" numa_2s.out && r=FAIL || r=PASS
    echo "no table with a bad checksum: $r"
} > check_numa.out
rec check_numa "(no listing: grep and sed on numa_2s.out and numa_flat.out)" "$(grep --version | head -n 1)" \
    "checks written in run.sh step 3" "0"
grep -q FAIL check_numa.out && status=1

# 4. forensic evidence: same sockets and memory size, all memory attached to node 0
boot forensic_numa -smp 4,sockets=2,cores=2,threads=1 -m 512M \
    -object memory-backend-ram,id=m0,size=512M \
    -numa node,nodeid=0,cpus=0-1,memdev=m0 -numa node,nodeid=1,cpus=2-3 \
    -numa dist,src=0,dst=1,val=21

rm -rf "$B"
exit $status
