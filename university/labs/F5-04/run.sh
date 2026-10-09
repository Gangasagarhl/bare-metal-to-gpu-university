#!/usr/bin/env bash
# Extra runs for F5-04, all inside a private network namespace (unshare -n), so the captures
# contain only this lab's traffic and the packet-dropping rule touches no other program:
#   tcp_talk        one short TCP conversation, captured (tcp_capture, tcp_fields)
#   refused         TCP and UDP towards a port where nobody listens, captured (refused_capture)
#   download_runs   the "slow download" experiment: 4 runs of download.cpp
#   slow_capture    the evidence pack of the forensic lab (capture of the small-buffer + loss run)
set -u
CXXFLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
GXX="$(g++ --version | head -n 1)"
TSH="$(tshark --version 2>/dev/null | head -n 1)"
NAMES="tcp_talk tcp_capture tcp_fields refused refused_capture download_runs slow_capture"
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
    for n in $NAMES; do
        header "$n" "net/" "$GXX" "(not run)"
        echo "result:    untested in this environment: unshare -n was not permitted" >> "$n.log"
        echo "(not run)" > "$n.out"
    done
    exit 0
fi
python3 -c "import socket,fcntl,struct; fcntl.ioctl(socket.socket(),0x8914,struct.pack('16sH14s',b'lo',0x41,b''))"
status=0
build() {   # build <name>: net/<name>.cpp -> .bin_<name>
    if ! g++ $CXXFLAGS "net/$1.cpp" -o ".bin_$1" > ".build_$1.txt" 2>&1; then
        header "$1" "net/$1.cpp" "$GXX" "g++ $CXXFLAGS net/$1.cpp -o $1"
        echo "result:    BUILD FAILED" >> "$1.log"; cat ".build_$1.txt" >> "$1.log"; exit 1
    fi
    rm -f ".build_$1.txt"
}
capture_start() {   # capture_start <file> <filter>
    tshark -i lo -f "$2" -s 128 -w "$1" -q > /dev/null 2>&1 &
    CAP=$!
    for _ in $(seq 50); do [ -s "$1" ] && break; sleep 0.1; done; sleep 0.5
}
capture_stop() { sleep 0.5; kill "$CAP"; wait "$CAP" 2>/dev/null; }

# 1. one TCP conversation
build tcp_talk
header tcp_talk "net/tcp_talk.cpp" "$GXX" "g++ $CXXFLAGS net/tcp_talk.cpp -o tcp_talk; ./tcp_talk"
capture_start tcp_talk.pcapng "tcp port 7000"
./.bin_tcp_talk > tcp_talk.out 2>&1; rc=$?
capture_stop
echo "exit code: $rc" >> tcp_talk.log; [ "$rc" = 0 ] || status=1
header tcp_capture "capture of tcp_talk (tcp_talk.pcapng)" "$TSH" "tshark -r tcp_talk.pcapng"
tshark -r tcp_talk.pcapng 2>/dev/null | sed 's/^ *//' > tcp_capture.out
echo "exit code: ${PIPESTATUS[0]}" >> tcp_capture.log
header tcp_fields "capture of tcp_talk (tcp_talk.pcapng)" "$TSH" \
    "tshark -r tcp_talk.pcapng -T fields -E header=y -e frame.number -e tcp.srcport -e tcp.dstport -e tcp.flags.str -e tcp.seq -e tcp.ack -e tcp.len -e tcp.seq_raw -e tcp.window_size_value"
tshark -r tcp_talk.pcapng -T fields -E header=y -e frame.number -e tcp.srcport -e tcp.dstport \
    -e tcp.flags.str -e tcp.seq -e tcp.ack -e tcp.len -e tcp.seq_raw -e tcp.window_size_value \
    2>/dev/null > tcp_fields.out
echo "exit code: $?" >> tcp_fields.log

# 2. nobody listening
build refused
header refused "net/refused.cpp" "$GXX" "g++ $CXXFLAGS net/refused.cpp -o refused; ./refused"
capture_start refused.pcapng "port 7001 or icmp"
./.bin_refused > refused.out 2>&1; rc=$?
capture_stop
echo "exit code: $rc" >> refused.log; [ "$rc" = 0 ] || status=1
header refused_capture "capture of refused (refused.pcapng)" "$TSH" "tshark -r refused.pcapng"
tshark -r refused.pcapng 2>/dev/null | sed 's/^ *//' > refused_capture.out
echo "exit code: ${PIPESTATUS[0]}" >> refused_capture.log

# 3. the slow download: two client settings x with or without dropping packets
build download
DROP="iptables -A INPUT -p tcp --sport 41000 -m length --length 200:65535 -m statistic --mode nth --every 40 --packet 0 -j DROP"
header download_runs "net/download.cpp" "$GXX" "g++ $CXXFLAGS net/download.cpp -o download; four runs, see below"
{
    echo "run A: ./download          (no packets dropped)"
    ./.bin_download
    echo "run B: ./download small    (no packets dropped)"
    ./.bin_download small
    $DROP
    echo "rule added: $DROP"
    echo "run C: ./download          (every 40th data packet from the server dropped)"
    ./.bin_download
    echo "run D: ./download small    (every 40th data packet from the server dropped; captured)"
} > download_runs.out 2>&1
capture_start slow.pcapng "tcp port 41000"
./.bin_download small >> download_runs.out 2>&1; rc=$?
capture_stop
echo "dropped by the rule in runs C and D: $(iptables -L INPUT -v -n | awk '/DROP/ {print $1}') packets" >> download_runs.out
iptables -F INPUT
echo "exit code: $rc" >> download_runs.log; [ "$rc" = 0 ] || status=1

header slow_capture "capture of run D (slow.pcapng, first 128 bytes of each packet)" "$TSH" \
    "tshark -r slow.pcapng with the display filters shown in the output"
{
    echo "== packets in the capture: $(tshark -r slow.pcapng 2>/dev/null | wc -l)"
    echo "== packets Wireshark marks as retransmissions: $(tshark -r slow.pcapng -Y tcp.analysis.retransmission 2>/dev/null | wc -l)"
    echo "== packets Wireshark marks as zero window: $(tshark -r slow.pcapng -Y tcp.analysis.zero_window 2>/dev/null | wc -l)"
    echo "== duration of the capture (s): $(tshark -r slow.pcapng -T fields -e frame.time_relative 2>/dev/null | tail -n 1)"
    echo "== window advertised by the client (bytes): how many packets carried each value"
    tshark -r slow.pcapng -Y 'tcp.dstport == 41000' -T fields -e tcp.window_size 2>/dev/null \
        | sort -n | uniq -c | sort -rn | head -n 6
    echo "== largest amount of data in flight seen by Wireshark (bytes)"
    tshark -r slow.pcapng -T fields -e tcp.analysis.bytes_in_flight 2>/dev/null | sort -n | tail -n 1
    echo "== every retransmission: frame, time (s), seq, length, time since the original (s)"
    tshark -r slow.pcapng -Y tcp.analysis.retransmission -T fields -e frame.number \
        -e frame.time_relative -e tcp.seq -e tcp.len -e tcp.analysis.rto 2>/dev/null > .retx.txt
    cat .retx.txt
    awk -F'\t' '{ s += $5; if ($5 > m) m = $5 } END { printf "sum of the last column: %.6f s; largest: %.6f s\n", s, m }' .retx.txt
    rm -f .retx.txt
    echo "== an excerpt of the capture: frames 40 to 60"
    tshark -r slow.pcapng -Y 'frame.number >= 40 and frame.number <= 60' 2>/dev/null \
        | sed 's/^ *//' | cut -c1-150
} > slow_capture.out
echo "exit code: 0" >> slow_capture.log

rm -f .bin_tcp_talk .bin_refused .bin_download slow.pcapng
exit $status
