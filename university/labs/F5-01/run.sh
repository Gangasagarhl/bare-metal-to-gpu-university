#!/usr/bin/env bash
# Extra runs for F5-01: one real UDP datagram on the loopback interface, captured.
# Everything runs inside a private network namespace (unshare -n) so that the capture sees
# only this lab's traffic and no other program on the machine is disturbed.
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
        echo "network:   private network namespace (unshare -n), loopback interface only"
    } > "$1.log"
}
if [ -z "${F5_IN_NETNS:-}" ]; then
    if unshare -n true 2>/dev/null; then
        F5_IN_NETNS=1 exec unshare -n "$0"
    fi
    for n in send_one send_one_capture lo_mtu; do
        header "$n" "net/send_one.cpp" "$GXX" "(not run)"
        echo "result:    untested in this environment: unshare -n (a private network namespace) was not permitted" >> "$n.log"
        echo "(not run: no private network namespace available)" > "$n.out"
    done
    exit 0
fi
# inside the namespace: switch the loopback interface on (SIOCSIFFLAGS with IFF_UP|IFF_RUNNING)
python3 -c "import socket,fcntl,struct; fcntl.ioctl(socket.socket(),0x8914,struct.pack('16sH14s',b'lo',0x41,b''))"
status=0

cmd="g++ $CXXFLAGS net/send_one.cpp -o send_one"
header send_one "net/send_one.cpp" "$GXX" "$cmd"
if ! g++ $CXXFLAGS net/send_one.cpp -o .bin_send_one > send_one.out 2>&1; then
    echo "result:    BUILD FAILED" >> send_one.log; exit 1
fi
tshark -i lo -f "udp port 5000" -w send_one.pcapng -q > /dev/null 2>&1 &
cap=$!
for _ in $(seq 50); do [ -s send_one.pcapng ] && break; sleep 0.1; done; sleep 0.5
./.bin_send_one > send_one.out 2>&1; rc=$?
echo "exit code: $rc" >> send_one.log
sleep 0.5; kill "$cap"; wait "$cap" 2>/dev/null
[ "$rc" = 0 ] || status=1

header send_one_capture "capture of the run above (send_one.pcapng)" "$TSH" \
    "tshark -i lo -f 'udp port 5000' -w send_one.pcapng; then tshark -r send_one.pcapng; tshark -r send_one.pcapng -x"
{ tshark -r send_one.pcapng 2>/dev/null | sed 's/^ *//'; echo; tshark -r send_one.pcapng -x 2>/dev/null; } > send_one_capture.out
echo "exit code: $?" >> send_one_capture.log
header lo_mtu "(none: a value the kernel reports)" "Linux $(uname -r)" "cat /sys/class/net/lo/mtu"
echo "loopback interface MTU (bytes): $(cat /sys/class/net/lo/mtu)" > lo_mtu.out
echo "exit code: $?" >> lo_mtu.log
rm -f .bin_send_one
exit $status
