#!/usr/bin/env bash
# F5-31 lab: boot two virtual servers over the network (UEFI PXE: DHCP + TFTP) and collect
# their SMBIOS inventory. QEMU's user-mode network ("slirp") provides the DHCP and TFTP servers.
# 1. build netboot.cc and put it in the TFTP directory as BOOTX64.EFI;
# 2. PXE-boot server A and server B (different MAC, processors, memory and SMBIOS strings);
# 3. build the inventory table and compare it with the QEMU options;
# 4. forensic evidence: a server whose DHCP answer names a file the TFTP server does not have.
# Every step writes <name>.out (real output) and <name>.log (run record).
set -u -o pipefail
cd "$(dirname "$0")"
status=0
B=.build
rm -rf "$B"; mkdir -p "$B/tftp"
QEMU=qemu-system-x86_64
QEMUV="$($QEMU --version | head -n 1)"
CLANGV="$(clang++ --version | head -n 1); $(lld-link --version | head -n 1)"
OVMFV="OVMF from the ovmf package $(dpkg-query -W -f '${Version}' ovmf)"
PYV="$(python3 --version)"
CODE=/usr/share/OVMF/OVMF_CODE_4M.fd
EMU="hardware:  untested on hardware; QEMU 8.2.2 q35 machines with the distribution's OVMF and QEMU's built-in DHCP/TFTP, not real servers or a real network"
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

# 0. QEMU's own description of the options used below (user network with DHCP/TFTP, SMBIOS)
{
    $QEMU -help | sed -n '/^-netdev user,/,/^-netdev tap/p' | grep -v "^-netdev tap"
    $QEMU -help | grep -E -A2 "^-smbios type=(1|3)\["
} > qemu_net_help.out; rc=$?
rec qemu_net_help "(no listing: QEMU's own help text)" "$QEMUV" "$QEMU -help | sed/grep: the -netdev user entry and the -smbios type=1 and type=3 entries" "$rc"

# 1. build
$UEFI_CXX -c netboot.cc -o $B/netboot.o > $B/build.txt 2>&1 &&
    $UEFI_LD /out:$B/tftp/BOOTX64.EFI $B/netboot.o >> $B/build.txt 2>&1 || { cat $B/build.txt; status=1; }

server() {  # server <name> <expected exit> <bootfile> <extra qemu_run.py options> -- <qemu options...>
    local name="$1" want="$2" file="$3" runopt="$4"; shift 5
    cp /usr/share/OVMF/OVMF_VARS_4M.fd $B/vars.fd
    python3 ../F3-10/qemu_run.py --timeout 150 $runopt -- $QEMU -machine q35 "$@" -display none \
        -serial stdio -no-reboot -drive if=pflash,format=raw,readonly=on,file=$CODE \
        -drive if=pflash,format=raw,file=$B/vars.fd \
        -netdev user,id=n0,tftp=$B/tftp,bootfile=$file \
        -device isa-debug-exit,iobase=0xf4,iosize=0x04 > "$name.out" 2>&1
    local rc=$?
    local meaning="33 = netboot.efi finished and wrote 0x10 to isa-debug-exit"
    [ "$want" = 124 ] && meaning="124 = stopped by qemu_run.py after the second failed boot option (expected: this is the broken server)"
    rec "$name" "netboot.cc console.hpp efi.hpp (F3-10)" "$CLANGV; $QEMUV; $OVMFV" \
        "$UEFI_CXX -c netboot.cc && $UEFI_LD /out:tftp/BOOTX64.EFI netboot.o; python3 ../F3-10/qemu_run.py --timeout 150 $runopt -- $QEMU -machine q35 $* -display none -serial stdio -no-reboot -drive if=pflash,...OVMF_CODE_4M.fd -drive if=pflash,...OVMF_VARS_4M.fd(copy) -netdev user,id=n0,tftp=tftp,bootfile=$file -device isa-debug-exit,iobase=0xf4,iosize=0x04" \
        "$rc ($meaning)" "$EMU"
    [ "$rc" = "$want" ] || status=1
}

# 2. two servers
server netboot_a 33 BOOTX64.EFI "" -- -smp 2,sockets=2 -m 512M \
    -device virtio-net-pci,netdev=n0,mac=52:54:00:00:07:01,bootindex=1 \
    -smbios type=1,manufacturer=DS304-Lab,product=Rack-Server,serial=SRV-A-0001,uuid=00112233-4455-6677-8899-aabbccddeeff \
    -smbios type=3,serial=CH-A-01,asset=RACK1-U07
server netboot_b 33 BOOTX64.EFI "" -- -smp 4,sockets=1,cores=4 -m 1G \
    -device virtio-net-pci,netdev=n0,mac=52:54:00:00:08:01,bootindex=1 \
    -smbios type=1,manufacturer=DS304-Lab,product=Rack-Server,serial=SRV-B-0002,uuid=0f1e2d3c-4b5a-6978-8796-a5b4c3d2e1f0 \
    -smbios type=3,serial=CH-B-02,asset=RACK1-U08

# 3. inventory table and checks
python3 report.py server-a=netboot_a.out server-b=netboot_b.out > inventory.out 2>&1; rc=$?
rec inventory "report.py" "$PYV" "python3 report.py server-a=netboot_a.out server-b=netboot_b.out" "$rc"
[ "$rc" = 0 ] || status=1
{
    for s in a b; do
        grep -q "^BdsDxe: starting Boot0001 \"UEFI PXEv4" netboot_$s.out && r=PASS || r=FAIL
        echo "server $s started from the UEFI PXEv4 boot option: $r"
    done
    grep -q "MAC address 525400000701" netboot_a.out && grep -q "MAC address 525400000801" netboot_b.out && r=PASS || r=FAIL
    echo "device path MAC nodes equal the -device ...,mac= options: $r"
    grep -q "^system.serial  *SRV-A-0001  *SRV-B-0002" inventory.out && r=PASS || r=FAIL
    echo "serial numbers equal the -smbios type=1,serial= options: $r"
    grep -q "^cpu.sockets  *2  *1" inventory.out && grep -q "^memory.mib  *512  *1024" inventory.out && r=PASS || r=FAIL
    echo "sockets 2/1 and memory 512/1024 MiB equal -smp/-m: $r"
    u=$(sed -n 's/^inventory: system.uuid=//p' netboot_a.out)
    echo "server A uuid option 00112233-4455-6677-8899-aabbccddeeff, raw SMBIOS bytes $u ($([ "$u" = 00112233445566778899aabbccddeeff ] && echo 'same order' || echo 'NOT the same order: see F5-31, Common mistakes'))"
} > check_netboot.out
rec check_netboot "(no listing: grep and sed on the outputs above)" "$(grep --version | head -n 1)" "checks written in run.sh step 3" "0"
grep -q FAIL check_netboot.out && status=1

# 4. forensic evidence: the DHCP answer names "bootx64.efi"; the TFTP directory holds "BOOTX64.EFI"
{
    echo "\$ ls -l tftp/   (the TFTP server's directory)"
    ls -l $B/tftp | tail -n +2 | awk '{print $1, $5, $NF}'
    echo
    echo "QEMU network options for the new server (the DHCP and TFTP server configuration):"
    echo "  -netdev user,id=n0,tftp=tftp,bootfile=bootx64.efi"
    echo "  -device virtio-net-pci,netdev=n0,mac=52:54:00:00:09:01,bootindex=1"
} > forensic_config.out
rec forensic_config "(no listing: ls of the TFTP directory and the options used in run.sh step 4)" "$(ls --version | head -n 1)" "ls -l tftp; echo options" "0"
server forensic_pxe 124 bootx64.efi "--stamp --stop-after Boot0002" -- -m 256M \
    -device virtio-net-pci,netdev=n0,mac=52:54:00:00:09:01,bootindex=1

rm -rf "$B"
exit $status
