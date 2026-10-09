#!/usr/bin/env bash
# Extra runs for F5-07 (measurement). run_lab.sh has already built and run the listings with
# the course flags (AddressSanitizer + UBSan, no optimisation). Here:
#   machine          what the numbers were measured on (printed by the machine itself)
#   udp_rtt_O2       udp_rtt.cpp built with -O2 and no sanitizers, run 3 times
#   tcp_throughput_O2  tcp_throughput.cpp built with -O2, 64 MiB per run
#   small_requests_capture  small_requests.cpp (-O2) run in a private network namespace while
#                    tshark captures it; the first packets of the default-options run
set -u
GXX="$(g++ --version | head -n 1)"
TSH="$(tshark --version 2>/dev/null | head -n 1)"
OPT="-std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror"
header() {   # header <name> <listing> <toolchain> <command>
    {
        echo "listing:   $2"
        echo "toolchain: $3"
        echo "command:   $4"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container, shared with other jobs)"
    } > "$1.log"
}
if [ -z "${F5_IN_NETNS:-}" ]; then
    status=0
    header machine "(none: facts printed by the machine)" "coreutils" "nproc; grep 'model name' /proc/cpuinfo | sort | uniq -c; uname -r"
    { echo "logical CPUs: $(nproc)"; grep 'model name' /proc/cpuinfo | sort | uniq -c | sed 's/^ *//';
      echo "kernel: $(uname -r)"; } > machine.out
    echo "exit code: 0" >> machine.log
    for n in udp_rtt tcp_throughput small_requests; do
        if ! g++ $OPT "$n.cpp" -o ".bin_${n}_O2" > ".build_$n.txt" 2>&1; then
            header "${n}_O2" "$n.cpp" "$GXX" "g++ $OPT $n.cpp -o ${n}_O2"
            echo "result:    BUILD FAILED" >> "${n}_O2.log"; cat ".build_$n.txt" >> "${n}_O2.log"; exit 1
        fi
        rm -f ".build_$n.txt"
    done
    header udp_rtt_O2 "udp_rtt.cpp (optimised, no sanitizers)" "$GXX" "g++ $OPT udp_rtt.cpp -o udp_rtt_O2; ./udp_rtt_O2 three times"
    : > udp_rtt_O2.out
    for i in 1 2 3; do echo "run $i:" >> udp_rtt_O2.out; ./.bin_udp_rtt_O2 >> udp_rtt_O2.out 2>&1 || status=1; done
    echo "exit code: $status" >> udp_rtt_O2.log
    header tcp_throughput_O2 "tcp_throughput.cpp (optimised, no sanitizers)" "$GXX" "g++ $OPT tcp_throughput.cpp -o tcp_throughput_O2; ./tcp_throughput_O2 64"
    ./.bin_tcp_throughput_O2 64 > tcp_throughput_O2.out 2>&1; rc=$?
    echo "exit code: $rc" >> tcp_throughput_O2.log; [ "$rc" = 0 ] || status=1
    if unshare -n true 2>/dev/null; then
        F5_IN_NETNS=1 unshare -n "$0" || status=1
    else
        header small_requests_capture "small_requests.cpp" "$TSH" "(not run)"
        echo "result:    untested in this environment: unshare -n was not permitted" >> small_requests_capture.log
        echo "(not run)" > small_requests_capture.out
    fi
    rm -f .bin_udp_rtt_O2 .bin_tcp_throughput_O2 .bin_small_requests_O2
    exit $status
fi
python3 -c "import socket,fcntl,struct; fcntl.ioctl(socket.socket(),0x8914,struct.pack('16sH14s',b'lo',0x41,b''))"
header small_requests_capture "small_requests.cpp (optimised), captured in a private network namespace" "$TSH; $GXX" \
    "g++ $OPT small_requests.cpp -o small_requests_O2; tshark -i lo -f tcp -w nagle.pcapng; ./small_requests_O2; tshark -r nagle.pcapng -t dd (time since the previous packet)"
echo "network:   private network namespace (unshare -n), loopback interface only" >> small_requests_capture.log
tshark -i lo -f "tcp" -s 128 -w nagle.pcapng -q > /dev/null 2>&1 &
cap=$!
for _ in $(seq 50); do [ -s nagle.pcapng ] && break; sleep 0.1; done; sleep 0.5
./.bin_small_requests_O2 > .nagle_run.txt 2>&1; rc=$?
sleep 0.5; kill "$cap"; wait "$cap" 2>/dev/null
{ echo "== program output"; cat .nagle_run.txt;
  echo "== capture, default-options connection, packets 1 to 16 (second column: time since the previous packet)";
  tshark -r nagle.pcapng -t dd -Y 'frame.number <= 16' 2>/dev/null | sed 's/^ *//' | cut -c1-140; } > small_requests_capture.out
echo "exit code: $rc" >> small_requests_capture.log
rm -f .nagle_run.txt nagle.pcapng
exit $rc
