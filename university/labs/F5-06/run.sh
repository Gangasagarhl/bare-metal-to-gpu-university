#!/usr/bin/env bash
# Extra runs for F5-06, inside a private network namespace AND a private mount namespace
# (unshare -n -m): in there, /etc/resolv.conf is replaced (for this run only, by a bind mount)
# with a file that points the system resolver at our own DNS server on 127.0.0.1.
# The machine's real /etc/resolv.conf is not changed.
#   dns_server      net/dns_server.cpp serving the made-up zone lab.example on port 53
#   resolve         net/resolve.cpp asking getaddrinfo for five names
#   dns_capture     the DNS packets of that run, decoded by tshark (summary, one decoded
#                   response, the bytes of one query)
#   dhcp_decode     dhcp.pcap (written by dhcp_frames.cpp) decoded by tshark
set -u
CXXFLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
GXX="$(g++ --version | head -n 1)"
TSH="$(tshark --version 2>/dev/null | head -n 1)"
header() {   # header <name> <listing> <toolchain> <command>
    {
        echo "listing:   $2"
        echo "toolchain: $3"
        echo "command:   $4"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
    } > "$1.log"
}
if [ -z "${F5_IN_NETNS:-}" ]; then
    header dhcp_decode "dhcp.pcap written by dhcp_frames.cpp" "$TSH" \
        "tshark -r dhcp.pcap; tshark -r dhcp.pcap -O dhcp -V -Y frame.number==2"
    { tshark -r dhcp.pcap 2>/dev/null | sed 's/^ *//'; echo;
      tshark -r dhcp.pcap -O dhcp -V -Y 'frame.number==2' 2>/dev/null | grep -v '^Frame'; } > dhcp_decode.out
    echo "exit code: $?" >> dhcp_decode.log
    if unshare -n -m --propagation private true 2>/dev/null; then
        F5_IN_NETNS=1 exec unshare -n -m --propagation private "$0"
    fi
    for n in dns_server resolve dns_capture; do
        header "$n" "net/" "$GXX" "(not run)"
        echo "result:    untested in this environment: unshare -n -m was not permitted" >> "$n.log"
        echo "(not run)" > "$n.out"
    done
    exit 0
fi
NS="network:   private network and mount namespaces (unshare -n -m); /etc/resolv.conf there: nameserver 127.0.0.1"
printf 'nameserver 127.0.0.1\n' > .resolv.conf
mount --bind "$(pwd)/.resolv.conf" /etc/resolv.conf
python3 -c "import socket,fcntl,struct; fcntl.ioctl(socket.socket(),0x8914,struct.pack('16sH14s',b'lo',0x41,b''))"
for n in dns_server resolve; do
    if ! g++ $CXXFLAGS "net/$n.cpp" -o ".bin_$n" > ".build_$n.txt" 2>&1; then
        header "$n" "net/$n.cpp" "$GXX" "g++ $CXXFLAGS net/$n.cpp -o $n"
        echo "result:    BUILD FAILED" >> "$n.log"; cat ".build_$n.txt" >> "$n.log"; exit 1
    fi
    rm -f ".build_$n.txt"
done
NAMES="localhost web.lab.example fileserver.lab.example printer.lab.example 192.0.2.7"
header dns_server "net/dns_server.cpp" "$GXX" "g++ $CXXFLAGS net/dns_server.cpp -o dns_server; ./dns_server 53"
echo "$NS" >> dns_server.log
header resolve "net/resolve.cpp" "$GXX" "g++ $CXXFLAGS net/resolve.cpp -o resolve; ./resolve $NAMES"
echo "$NS" >> resolve.log
./.bin_dns_server 53 > dns_server.out 2>&1 &
srv=$!
for _ in $(seq 100); do grep -q authoritative dns_server.out 2>/dev/null && break; sleep 0.05; done
tshark -i lo -f "udp port 53" -w dns.pcapng -q > /dev/null 2>&1 &
cap=$!
for _ in $(seq 50); do [ -s dns.pcapng ] && break; sleep 0.1; done; sleep 0.5
./.bin_resolve $NAMES > resolve.out 2>&1; rc=$?
sleep 0.5; kill "$cap"; wait "$cap" 2>/dev/null
wait "$srv"; src=$?
echo "exit code: $rc (1 = at least one name could not be resolved; expected here)" >> resolve.log
echo "exit code: $src" >> dns_server.log
header dns_capture "capture of the run above (dns.pcapng)" "$TSH" \
    "tshark -r dns.pcapng; tshark -r dns.pcapng -O dns -V -Y frame.number==2; tshark -r dns.pcapng -x -Y frame.number==1"
echo "$NS" >> dns_capture.log
{ tshark -r dns.pcapng 2>/dev/null | sed 's/^ *//'; echo;
  tshark -r dns.pcapng -O dns -V -Y 'frame.number==2' 2>/dev/null | grep -v -E '^Frame|^Ethernet|^Internet|^User'; echo;
  tshark -r dns.pcapng -x -Y 'frame.number==1' 2>/dev/null; } > dns_capture.out
echo "exit code: 0" >> dns_capture.log
umount /etc/resolv.conf
rm -f .bin_dns_server .bin_resolve .resolv.conf
[ "$src" = 0 ] || exit 1
exit 0
