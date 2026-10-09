#!/usr/bin/env bash
# F4-10 run.sh: milestone C9 in QEMU (q35, e1000, user-mode networking).
#   netcheck  : host program, header layouts checked against <netinet/*.h> (run_lab.sh)
#   build     : the F4-10 kernel (default) and the forensic variant
#   net       : DHCP, ping, echo server (host client, 100 MiB), HTTP fetch, 5 % loss runs
#   capture   : tshark summary of the network capture of that run
#   net_iommu : HTTP fetch and loss runs again with intel-iommu (DMA through VT-d)
#   forensic  : the nightly upload built with -DF410_TIMER_BUG; forensic_capture = its capture
set -u -o pipefail
cd "$(dirname "$0")"
. ../F4-01/lablib.sh
status=0
SRC="../F4-01/boot.S ../F4-01/kbase.cc ../F4-01/div64.cc ../F4-02/pci.cc ../F4-02/acpi.cc \
../F4-08/isr256.S ../F4-08/intr.cc ../F4-08/msi.cc ../F4-08/vtd.cc ../F4-08/dma.cc \
e1000.cc net.cc tcp.cc f410_main.cc"
TSHARK_VER="$(tshark --version 2>/dev/null | head -n 1)"
rm -rf .www; mkdir .www; cp www/index.html .www/
python3 -c "import random; open('.www/big.bin','wb').write(random.Random(9).randbytes(262144))"
PORT="$(python3 -c 'import socket; s=socket.socket(); s.bind(("127.0.0.1",0)); print(s.getsockname()[1])')"
NETDEV="user,id=n0,hostfwd=tcp:127.0.0.1:$PORT-:7,guestfwd=tcp:10.0.2.100:80-cmd:python3 httpd_stdio.py .www .host.txt,guestfwd=tcp:10.0.2.100:9-cmd:python3 sink.py .host.txt"
SHOWN="-device e1000,netdev=n0,mac=52:54:00:12:34:56 -netdev 'user,id=n0,hostfwd=tcp:127.0.0.1:<port>-:7,guestfwd=tcp:10.0.2.100:80-cmd:python3 httpd_stdio.py www host.txt,guestfwd=tcp:10.0.2.100:9-cmd:python3 sink.py host.txt' -object filter-dump,id=f0,netdev=n0,file=cap.pcap"

netboot() {  # $1 out, $2 kernel, $3 pcap, $4 extra
    rm -f "$3" .host.txt
    timeout 500 $QEMU -machine q35 $QCOMMON -serial stdio -kernel "$2" $4 -device e1000,netdev=n0,mac=52:54:00:12:34:56 \
        -netdev "$NETDEV" -object filter-dump,id=f0,netdev=n0,file="$3" > "$1" 2>&1
    local rc=$?
    sed -i 's/\r$//' "$1"
    [ -f .host.txt ] && { echo "== host-side servers (httpd_stdio.py, sink.py) =="; cat .host.txt; } >> "$1"
    return $rc
}

kbuild k410.elf $SRC > .kb.txt 2>&1; rc=$?
KEXTRA="-DF410_TIMER_BUG" kbuild k410f.elf $SRC >> .kb.txt 2>&1 || rc=1
{ cat .kb.txt; size -A k410.elf | grep -E '^(section|\.text|\.rodata|\.data|\.bss|Total)'; } > build.out
rec build "$SRC kernel.ld" "$GXX_VER; $LD_VER" \
    "g++ $KFLAGS -c <each file>; ld -m elf_i386 -nostdlib -static -T kernel.ld -o k410.elf *.o (and again with -DF410_TIMER_BUG)" "$rc"
[ "$rc" = 0 ] || status=1

python3 echo_client.py "$PORT" 104857600 net.out > .echo.txt 2>&1 &
ec=$!
netboot net.out k410.elf .cap.pcap ""; rc=$?
wait $ec
{ echo "== host echo client =="; cat .echo.txt; } >> net.out
rec net "f410_main.cc (kernel k410.elf); host: echo_client.py, httpd_stdio.py, sink.py" "$QEMU_VER" \
    "$QEMU -machine q35 $QCOMMON -serial stdio -kernel k410.elf $SHOWN; python3 echo_client.py <port> 104857600" "$rc" \
    "note:      exit code 33 = pass; lines starting with 'host' come from the host-side programs" "$HW_NOTE"
[ "$rc" = 33 ] || status=1
grep -q 'identical: True' .echo.txt || status=1

{ echo "== packets, malformed packets, TCP analysis flags =="
  printf 'frames in capture: '; tshark -r .cap.pcap 2>/dev/null | wc -l
  printf 'malformed packets: '; tshark -r .cap.pcap -Y '_ws.malformed' 2>/dev/null | wc -l
  printf 'bad IPv4/TCP checksums (validation on): '
  tshark -r .cap.pcap -o ip.check_checksum:TRUE -o tcp.check_checksum:TRUE -Y 'ip.checksum.status == 0 or tcp.checksum.status == 0' 2>/dev/null | wc -l
  for f in tcp.analysis.retransmission tcp.analysis.fast_retransmission tcp.analysis.duplicate_ack \
           tcp.analysis.lost_segment tcp.analysis.out_of_order; do
      printf '%-38s %s\n' "$f" "$(tshark -r .cap.pcap -Y "$f" 2>/dev/null | wc -l)"; done
  for p in 7 80 9; do
      printf 'port %-3s retransmissions %-5s duplicate ACKs %-5s zero-window or window-full %s\n' "$p" \
        "$(tshark -r .cap.pcap -Y "tcp.port == $p and tcp.analysis.retransmission" 2>/dev/null | wc -l)" \
        "$(tshark -r .cap.pcap -Y "tcp.port == $p and tcp.analysis.duplicate_ack" 2>/dev/null | wc -l)" \
        "$(tshark -r .cap.pcap -Y "tcp.port == $p and (tcp.analysis.zero_window or tcp.analysis.window_full)" 2>/dev/null | wc -l)"; done
  echo "== the first 12 frames =="
  tshark -r .cap.pcap 2>/dev/null | head -n 12 | sed 's/^ *//'
  echo "== TCP conversations =="
  tshark -r .cap.pcap -q -z conv,tcp 2>/dev/null | sed -n '/<->/p' | sed 's/^ *//'
} > capture.out; rc=$?
rec capture "tshark on cap.pcap (QEMU filter-dump of the net run)" "$TSHARK_VER" \
    "tshark -r cap.pcap with display filters _ws.malformed, checksum status, tcp.analysis.*; -z conv,tcp" "$rc"

python3 echo_client.py "$PORT" 65536 net_iommu.out > .echo.txt 2>&1 &
ec=$!
netboot net_iommu.out k410.elf .icap.pcap "-device intel-iommu"; rc=$?
wait $ec
{ echo "== host echo client =="; cat .echo.txt; } >> net_iommu.out
rec net_iommu "f410_main.cc (kernel k410.elf) with intel-iommu" "$QEMU_VER" \
    "$QEMU -machine q35 $QCOMMON -serial stdio -kernel k410.elf -device intel-iommu $SHOWN; python3 echo_client.py <port> 65536" "$rc" \
    "note:      exit code 33 = pass" "$HW_NOTE"
[ "$rc" = 33 ] || status=1

netboot forensic.out k410f.elf .fcap.pcap ""; rc=$?
rec forensic "f410_main.cc and tcp.cc built with -DF410_TIMER_BUG (kernel k410f.elf)" "$QEMU_VER" \
    "$QEMU -machine q35 $QCOMMON -serial stdio -kernel k410f.elf $SHOWN" "$rc" \
    "note:      exit code 3 = the kernel reported FAILED: expected for this forensic build" "$HW_NOTE"
[ "$rc" = 3 ] || status=1
{ echo "== TCP analysis of the upload connection (port 9) =="
  for f in tcp.analysis.retransmission tcp.analysis.fast_retransmission tcp.analysis.duplicate_ack \
           tcp.analysis.lost_segment tcp.analysis.ack_lost_segment; do
      printf '%-38s %s\n' "$f" "$(tshark -r .fcap.pcap -Y "tcp.port == 9 and $f" 2>/dev/null | wc -l)"; done
  echo "== the last 16 frames of the upload connection (relative sequence numbers, time from start) =="
  tshark -r .fcap.pcap -Y 'tcp.port == 9' -T fields -e frame.number -e frame.time_relative -e ip.src \
      -e tcp.flags.str -e tcp.seq -e tcp.len -e tcp.ack -e _ws.expert.message -E separator=' ' 2>/dev/null | tail -n 16
  echo "== frames flagged 'previous segment not captured' =="
  tshark -r .fcap.pcap -Y 'tcp.port == 9 and tcp.analysis.lost_segment' -T fields -e frame.number \
      -e frame.time_relative -e tcp.seq -e tcp.len -E separator=' ' 2>/dev/null
} > forensic_capture.out; rc=$?
rec forensic_capture "tshark on cap.pcap of the forensic run" "$TSHARK_VER" \
    "tshark -r cap.pcap -Y 'tcp.port == 9 and tcp.analysis.*'; -T fields -e frame.number -e frame.time_relative -e ip.src -e tcp.flags.str -e tcp.seq -e tcp.len -e tcp.ack -e _ws.expert.message" "$rc"

rm -rf .host.txt .www k410.elf k410f.elf .kb.txt .echo.txt .cap.pcap .icap.pcap .fcap.pcap
exit $status
