#!/usr/bin/env bash
# DS401 F5-35 extra steps (after run_lab.sh has built and run the .cpp listings, which
# wrote roce.pcap and incast.pcap):
#   dissect          tshark -V decoding of roce.pcap (Listing 1's three frames)
#   decodes          tshark's own table of what it decodes as InfiniBand transport
#   incast_capture   tshark field listing of incast.pcap (forensic evidence)
#   nak_detail       tshark's decoding of frame 14 of incast.pcap
set -u
cd "$(dirname "$0")"
status=0
tver="$(tshark --version 2>/dev/null | head -n 1)"
log()   # name listing command rc
{
    {
        echo "listing:   $2"
        echo "toolchain: $tver"
        echo "command:   $3"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
        echo "hardware:  untested on hardware: the frames were built by the lab's own program, not sent by an RDMA NIC"
        echo "exit code: $4"
    } > "$1.log"
}
if [ ! -f roce.pcap ] || [ ! -f incast.pcap ]; then
    echo "run.sh: capture files missing (run via run_lab.sh)"; exit 1
fi
# -V prints every layer; the frame-metadata layer is cut away to keep the evidence short
tshark -r roce.pcap -V -o gui.column.format:"No.,%m" 2>/dev/null \
    | awk '/^Frame [0-9]+:/{print; skip=1; next} /^[A-Z]/{skip=0} !skip' > dissect.out; rc=${PIPESTATUS[0]}
log dissect roce_packet.cpp "tshark -r roce.pcap -V   (frame-metadata block removed with awk)" $rc
[ $rc = 0 ] || status=1

tshark -G decodes 2>/dev/null | grep -i infiniband > decodes.out; rc=$?
log decodes "(none: tshark's built-in table)" "tshark -G decodes | grep -i infiniband" $rc
[ $rc = 0 ] || status=1

tshark -r incast.pcap -T fields -E header=y -E separator='|' \
    -e frame.number -e frame.time_relative -e ip.src -e ip.dst -e ip.dsfield.dscp \
    -e infiniband.bth.opcode -e infiniband.bth.psn -e infiniband.aeth.syndrome 2>/dev/null \
    | awk -F'|' '{printf "%-12s %-18s %-9s %-9s %-14s %-20s %-19s %s\n", $1, $2, $3, $4, $5, $6, $7, $8}' \
    > incast_capture.out; rc=${PIPESTATUS[0]}
log incast_capture incast.cpp "tshark -r incast.pcap -T fields -E header=y -e frame.number -e frame.time_relative -e ip.src -e ip.dst -e ip.dsfield.dscp -e infiniband.bth.opcode -e infiniband.bth.psn -e infiniband.aeth.syndrome   (columns aligned with awk)" $rc
[ $rc = 0 ] || status=1
tshark -r incast.pcap -Y 'frame.number == 14' -O infiniband 2>/dev/null \
    | awk '/^Frame [0-9]+:/{print; skip=1; next} /^[A-Z]/{skip=0} !skip' > nak_detail.out; rc=${PIPESTATUS[0]}
log nak_detail incast.cpp "tshark -r incast.pcap -Y 'frame.number == 14' -O infiniband   (frame-metadata block removed with awk)" $rc
[ $rc = 0 ] || status=1
rm -f roce.pcap incast.pcap
exit $status
