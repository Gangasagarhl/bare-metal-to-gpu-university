#!/usr/bin/env bash
# F5-46 run.sh: three nodes boot the DS403 kernel over the network (iPXE + TFTP from QEMU's
# user-mode network), then share files through DFS: node 1 serves, nodes 2 and 3 use them.
#   1. build       kernel f546.elf (F5-44 base + dfs_server.cc dfs_client.cc f546_main.cc)
#   2. dfs         the netbooted cluster; node 1 (the server) halts at tick 900
#   3. netboot     what was on the boot network of node 2 (tshark on QEMU's packet dump)
#   4. accept      checks
#   5. forensic    the same run with node 2's cache set to "naive"
set -u -o pipefail
cd "$(dirname "$0")"
. ../F5-44/lablib.sh
status=0
K=../F5-44
KSRC="$K/boot.S $K/k.cc $K/e1000.cc $K/msg.cc $K/member.cc dfs_server.cc dfs_client.cc f546_main.cc"
B=.build
rm -rf "$B"; mkdir -p "$B/tftp"

{ kbuild "$B/tftp/f546.elf" $KSRC && hostbuild "$K/.labswitch" "$K/labswitch.cc"; } > build.out 2>&1; rc=$?
size -A "$B/tftp/f546.elf" | grep -E '^(section|\.text|\.rodata|\.data|\.bss|Total)' >> build.out
rec build "F5-44: boot.S k.cc e1000.cc msg.cc member.cc; F5-46: dfs_server.cc dfs_client.cc f546_main.cc" "$GXX_VER; $LD_VER" \
    "g++ $KFLAGS -c <each file>; ld -m elf_i386 -nostdlib -static -T kernel.ld -o tftp/f546.elf *.o" "$rc"
[ "$rc" = 0 ] || status=1

run_cluster() {  # run_cluster <tag> <cache mode of node 2>
    local tag="$1"
    CL=("node=1 nodes=3 role=server life=900" "node=2 nodes=3 role=writer server=1 cache=$2 life=1500" "node=3 nodes=3 role=appender server=1 life=1500")
    NETBOOT="$B/tftp" cluster "$tag" 60000 "$B/tftp/f546.elf" -- "${CL[@]}"
    for i in 1 2 3; do
        mv "${tag}_n$i.txt" "${tag}_n$i.out"
        rc="$(awk -v n=n$i '$1==n {print $2}' "${tag}_rc.txt")"
        rec "${tag}_n$i" "f546_main.cc dfs_server.cc dfs_client.cc (kernel f546.elf, loaded by iPXE over TFTP)" "$QEMU_VER; iPXE ROM shipped with the distribution's QEMU" \
            "$(NETBOOT=$B/tftp qemu_node $i "$CLUSTER_BASE" "$B/tftp/f546.elf" "${CL[$((i - 1))]}" | sed "s#$B/##g")" "$rc" \
            "note:      boot script n$i.ipxe = '#!ipxe', 'kernel f546.elf ${CL[$((i - 1))]}', 'boot' (iPXE fetches f546.elf from the same TFTP server)" \
            "note:      exit code 33 = the kernel wrote 0x10 to isa-debug-exit (its planned halt)" "$HW_NOTE"
        [ "$rc" = 33 ] || status=1
    done
    mv "${tag}_switch.txt" "${tag}_switch.out"
    rec "${tag}_switch" "labswitch.cc (F5-44)" "$GXX_VER" "labswitch 3 $CLUSTER_BASE 60000" 0 "$HW_NOTE"
    rm -f "${tag}_rc.txt"
}

# 2. the shared-file run
run_cluster dfs valid

# 3. the boot network of node 2, decoded by tshark
{
    echo "node 2 boot network (QEMU filter-dump of the virtio-net boot NIC), decoded by tshark:"
    tshark -r dfs_boot_n2.pcap -Y 'dhcp || tftp.opcode == 1 || tftp.opcode == 6' 2>/dev/null | sed 's/^ *//'
    echo "TFTP data packets: $(tshark -r dfs_boot_n2.pcap -Y 'tftp.opcode == 3' 2>/dev/null | wc -l)"
    echo "size of f546.elf on the TFTP server: $(stat -c %s "$B/tftp/f546.elf") bytes"
} > netboot.out
rec netboot "QEMU -object filter-dump (packet capture of the boot NIC)" "$(tshark --version 2>/dev/null | head -n 1)" \
    "tshark -r dfs_boot_n2.pcap -Y 'dhcp || tftp.opcode == 1 || tftp.opcode == 6'; tshark ... -Y 'tftp.opcode == 3' | wc -l" 0 "$HW_NOTE"

# 4. acceptance checks
{
    for i in 1 2 3; do
        grep -q 'loaded by "iPXE' dfs_n$i.out && echo "node $i was loaded by iPXE over the network: PASS" || echo "node $i boot loader: FAIL"
    done
    grep -q 'notes  qpath=2 version=2 length=40  "hello from node 2\\nand hello from node 3\\n"' dfs_n1.out \
        && echo "server's copy of notes holds both nodes' lines: PASS" || echo "server's copy: FAIL"
    sed -n '/read notes again: 40 bytes/,+2p' dfs_n2.out | grep -q 'and hello from node 3' \
        && echo "node 2 reads node 3's line after the change (cache revalidated): PASS" || echo "node 2 re-read: FAIL"
    grep -q 'create notes again: file exists' dfs_n3.out && echo "second create of the same name is refused: PASS" || echo "create twice: FAIL"
    grep -q 'read notes after the server left: error -110' dfs_n2.out && echo "with the server gone, a read fails with an error (no hang): PASS" || echo "server gone: FAIL"
} > accept.out
grep -q FAIL accept.out && { rc=1; status=1; } || rc=0
rec accept "run.sh step 4 (grep of dfs_n1.out ... dfs_n3.out)" "$(grep --version | head -n 1)" "see run.sh" "$rc"

# 5. forensic evidence: node 2 trusts its cache forever
run_cluster stale naive
rm -f dfs_boot_n1.pcap dfs_boot_n3.pcap stale_boot_n*.pcap dfs_boot_n2.pcap
rm -rf "$B" "$K/.labswitch"
exit $status
