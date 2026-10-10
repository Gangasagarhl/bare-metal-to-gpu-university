#!/usr/bin/env bash
# F1-51 run.sh: look at real Ethernet frames with tshark (Wireshark's command-line tool).
#   step frame_decode  : tshark decodes the two frames Listing 1 wrote and checks their FCS
#   step crc_check     : an independent CRC-32 (Python's zlib) on the standard check string
#   step qemu_capture  : frames a real driver (the iPXE boot ROM) sent and received through
#                        QEMU's emulated Intel e1000 NIC, captured by QEMU into a pcap file
#   step loopback      : a capture on this machine's loopback interface (no NIC involved)
set -u -o pipefail
cd "$(dirname "$0")"
TV="$(tshark --version 2>/dev/null | head -n 1)"
QV="$(qemu-system-x86_64 --version | head -n 1)"
rec() {
    {
        echo "listing:   run.sh (step $1)"
        echo "toolchain: $2"
        echo "command:   $3"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
        echo "exit code: $4"
        if [ $# -ge 5 ]; then echo "$5"; fi
    } > "$1.log"
}
status=0
clean() { grep -v '^Running as user "root"'; }

# --- frame_decode (frame.pcap was written by Listing 1, which run_lab.sh ran first)
C="tshark -r frame.pcap -o eth.fcs:Always -o eth.check_fcs:TRUE -V -O eth,arp"
tshark -r frame.pcap -o eth.fcs:Always -o eth.check_fcs:TRUE -V -O eth,arp 2>&1 | clean > frame_decode.out
rc=$?; rec frame_decode "$TV" "$C" "$rc"; [ "$rc" -eq 0 ] || status=1

# --- crc_check
C='python3 -c "import zlib; print(hex(zlib.crc32(b\"123456789\")))"'
python3 -c 'import zlib; print(hex(zlib.crc32(b"123456789")))' > crc_check.out 2>&1
rc=$?; rec crc_check "$(python3 --version 2>&1)" "$C" "$rc"; [ "$rc" -eq 0 ] || status=1

# --- qemu_capture: the iPXE ROM on QEMU's e1000 asks for an address with DHCP
rm -f .net.pcap
C="qemu-system-x86_64 -machine q35 -nodefaults -display none -serial none -monitor none -no-reboot -boot order=n -netdev user,id=n0 -device e1000,netdev=n0,mac=52:54:00:12:34:56 -object filter-dump,id=f1,netdev=n0,file=net.pcap  (stopped after 20 s by timeout)"
timeout 20 qemu-system-x86_64 -machine q35 -nodefaults -display none -serial none -monitor none -no-reboot \
    -boot order=n -netdev user,id=n0 -device e1000,netdev=n0,mac=52:54:00:12:34:56 \
    -object filter-dump,id=f1,netdev=n0,file=.net.pcap > /dev/null 2>&1
qrc=$?
{
    echo "\$ tshark -r net.pcap"
    tshark -r .net.pcap 2>&1 | clean
    n=$(tshark -r .net.pcap -Y arp -T fields -e frame.number 2>/dev/null | head -n 1)
    echo
    echo "\$ tshark -r net.pcap -Y frame.number==$n -V -O eth"
    tshark -r .net.pcap -Y "frame.number==$n" -V -O eth 2>&1 | clean | sed -n '1,40p'
} > qemu_capture.out
rc=$?
rec qemu_capture "$QV; $TV" "$C; then tshark -r net.pcap" "$qrc" \
    "note:      exit code 124 is expected: QEMU was stopped by the 20 s time limit after the boot ROM gave up
hardware:  emulated NIC (QEMU e1000 model) on QEMU's user-mode network; no physical NIC, PHY or cable involved"
[ "$qrc" -eq 124 ] || status=1
[ -s qemu_capture.out ] || status=1

# --- loopback: three UDP datagrams to this machine itself
rm -f .lo.pcap
C="tshark -i lo -a duration:5 -w lo.pcap, while python3 sends three UDP datagrams to 127.0.0.1 port 9999"
( sleep 2; python3 -c 'import socket
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
for i in range(3):
    s.sendto(b"hello HW205 %d" % i, ("127.0.0.1", 9999))' ) &
timeout 15 tshark -i lo -a duration:5 -w .lo.pcap > /dev/null 2>&1
crc=$?
wait
{
    echo "\$ tshark -r lo.pcap -Y udp"
    tshark -r .lo.pcap -Y udp 2>&1 | clean
    echo
    echo "\$ tshark -r lo.pcap -Y udp -V -O eth   (output cut after the first frame)"
    tshark -r .lo.pcap -Y udp -V -O eth 2>&1 | clean | awk 'NR>1 && /^Frame /{exit} {print}' | sed -n '1,20p'
} > loopback.out
rec loopback "$TV; $(python3 --version 2>&1)" "$C" "$crc" \
    "note:      capture on the loopback interface was permitted in this container; timestamps differ on every run
hardware:  no NIC: loopback frames never leave the kernel"
[ "$crc" -eq 0 ] || status=1
rm -f .net.pcap .lo.pcap frame.pcap
exit $status
