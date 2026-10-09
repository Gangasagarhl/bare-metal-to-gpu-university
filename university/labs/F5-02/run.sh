#!/usr/bin/env bash
# Extra runs for F5-02: decode the hand-built frames with tshark (Wireshark's command-line
# tool), and decode the real kernel-built datagram captured in F5-01 for comparison.
# run_lab.sh has already built and run encapsulate.cpp and broken_frame.cpp (they wrote the
# .pcap files).
set -u
TSH="$(tshark --version 2>/dev/null | head -n 1)"
CHK="-o ip.check_checksum:TRUE -o udp.check_checksum:TRUE"
header() {   # header <name> <listing> <command>
    {
        echo "listing:   $2"
        echo "toolchain: $TSH"
        echo "command:   $3"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
    } > "$1.log"
}
decode() {   # decode <name> <pcap> <listing>
    header "$1" "$3" "tshark -r $2 $CHK -O eth,ip,udp -V   (lines of the Frame block removed by grep)"
    tshark -r "$2" $CHK -O eth,ip,udp -V 2>/dev/null | grep -v -E '^Frame|^    \[|Expert Info|^        \[|^            \[' > "$1.out"
    echo "exit code: ${PIPESTATUS[0]}" >> "$1.log"
}
status=0
[ -f handmade.pcap ] && [ -f broken.pcap ] || { echo "missing pcap files"; exit 1; }
decode handmade_decode handmade.pcap "handmade.pcap written by encapsulate.cpp"
decode broken_decode broken.pcap "broken.pcap written by broken_frame.cpp"

header demux "handmade.pcap" "tshark -r handmade.pcap -T fields -E header=y -e eth.type -e ip.proto -e udp.dstport -e data.data"
tshark -r handmade.pcap -T fields -E header=y -e eth.type -e ip.proto -e udp.dstport -e data.data > demux.out 2>/dev/null
echo "exit code: $?" >> demux.log

# lab steps 3 and 5: two variants of encapsulate.cpp made with sed (destination port 53;
# UDP checksum left at zero), built with the course flags, and decoded
GXX="$(g++ --version | head -n 1)"
CXXFLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
variant() {   # variant <name> <sed expression>
    sed -e "$2" -e "s/handmade.pcap/$1.pcap/g" encapsulate.cpp > ".$1.cpp"
    if g++ $CXXFLAGS ".$1.cpp" -o ".bin_$1" > /dev/null 2>&1 && ./".bin_$1" > /dev/null 2>&1; then
        header "$1_decode" "encapsulate.cpp changed with: sed -e '$2' (built with $GXX, course flags)" \
            "tshark -r $1.pcap $CHK -V   (the UDP and following layers shown)"
        tshark -r "$1.pcap" $CHK -V 2>/dev/null | sed -n '/^User Datagram/,$p' | grep -v -E 'Timestamps|Time since|Stream index' > "$1_decode.out"
        echo "exit code: ${PIPESTATUS[0]}" >> "$1_decode.log"
    else
        header "$1_decode" "encapsulate.cpp variant" "(build or run failed)"
        echo "result:    BUILD FAILED" >> "$1_decode.log"; status=1
    fi
    rm -f ".$1.cpp" ".bin_$1" "$1.pcap"
}
variant port53 's/put16(u, 5000);  /put16(u, 53);    /'
variant zero_checksum 's/    set16(u, 6, inetChecksum(pseudo));//'

if [ -f ../F5-01/send_one.pcapng ]; then
    header kernel_decode "../F5-01/send_one.pcapng (the real datagram of F5-01, built by the Linux kernel)" \
        "tshark -r ../F5-01/send_one.pcapng $CHK -O eth,ip,udp -V   (Frame block removed)"
    tshark -r ../F5-01/send_one.pcapng $CHK -O eth,ip,udp -V 2>/dev/null | grep -v -E '^Frame|^    \[Time|^    \[Stream|^    \[Timestamps|^        \[Time' > kernel_decode.out
    echo "exit code: ${PIPESTATUS[0]}" >> kernel_decode.log
else
    header kernel_decode "../F5-01/send_one.pcapng" "(not run)"
    echo "result:    untested: run university/labs/run_lab.sh university/labs/F5-01 first" >> kernel_decode.log
    echo "(not run)" > kernel_decode.out
fi
exit $status
