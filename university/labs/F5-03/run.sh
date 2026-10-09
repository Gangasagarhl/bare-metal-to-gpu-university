#!/usr/bin/env bash
# Extra runs for F5-03: decode the hand-built ARP frames with tshark, and list the IPv4
# addresses of a private network namespace (where only the loopback interface exists).
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
    header arp_decode "arp.pcap written by arp_request.cpp" "$TSH" "tshark -r arp.pcap -O arp -V"
    tshark -r arp.pcap -O arp -V 2>/dev/null | grep -v '^Frame' > arp_decode.out
    echo "exit code: ${PIPESTATUS[0]}" >> arp_decode.log
    header printer_capture "printer_case.pcap written by printer_case.cpp (simulated frames)" "$TSH" \
        "tshark -r printer_case.pcap"
    tshark -r printer_case.pcap 2>/dev/null | sed 's/^ *//' > printer_capture.out
    echo "exit code: ${PIPESTATUS[0]}" >> printer_capture.log
    if unshare -n true 2>/dev/null; then
        F5_IN_NETNS=1 unshare -n "$0" || exit 1
    else
        header show_addrs "net/show_addrs.cpp" "$GXX" "(not run)"
        echo "result:    untested in this environment: unshare -n was not permitted" >> show_addrs.log
        echo "(not run)" > show_addrs.out
    fi
    exit 0
fi
# inside a private network namespace: switch loopback on, then list addresses
python3 -c "import socket,fcntl,struct; fcntl.ioctl(socket.socket(),0x8914,struct.pack('16sH14s',b'lo',0x41,b''))"
header show_addrs "net/show_addrs.cpp" "$GXX" "g++ $CXXFLAGS net/show_addrs.cpp -o show_addrs"
echo "network:   private network namespace (unshare -n), loopback interface only" >> show_addrs.log
if ! g++ $CXXFLAGS net/show_addrs.cpp -o .bin_show_addrs > show_addrs.out 2>&1; then
    echo "result:    BUILD FAILED" >> show_addrs.log; exit 1
fi
./.bin_show_addrs > show_addrs.out 2>&1; rc=$?
echo "exit code: $rc" >> show_addrs.log
rm -f .bin_show_addrs
exit "$rc"
