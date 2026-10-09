#!/usr/bin/env bash
# F1-46: a q35 PC with an xHCI controller, a USB keyboard on port 1 and a hub on
# port 2 with a tablet and a mouse behind it. Only the SeaBIOS firmware runs; it
# enumerates the keyboard. QEMU records the keyboard's USB traffic to a pcap file.
set -u -o pipefail
cd "$(dirname "$0")"
status=0
rec() {  # rec <name> <listing> <toolchain> <command> <exit code> [extra line]
    {
        echo "listing:   $2"
        echo "toolchain: $3"
        echo "command:   $4"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
        echo "exit code: $5"
        if [ $# -ge 6 ]; then echo "$6"; fi
    } > "$1.log"
    [ "$5" = 0 ] || status=1
}
QX="$(qemu-system-x86_64 --version | head -n 1)"
DEV="-device qemu-xhci,id=xhci -device usb-kbd,bus=xhci.0,port=1,pcap=.kbd.pcap -device usb-hub,bus=xhci.0,port=2,id=hub -device usb-tablet,bus=xhci.0,port=2.1 -device usb-mouse,bus=xhci.0,port=2.2"
rm -f .kbd.pcap
python3 -I hmp.py 3 "info usb" qemu-system-x86_64 -machine q35 -display none -serial null -nographic \
    -no-reboot $DEV > qemu_info_usb.out 2>&1; rc=$?
rec qemu_info_usb hmp.py "$QX" \
    "python3 -I hmp.py 3 \"info usb\" qemu-system-x86_64 -machine q35 -display none -serial null -nographic -no-reboot $DEV" \
    "$rc" "note:      only the SeaBIOS firmware ran, for about 3 s"
TV="$(tshark -v 2>/dev/null | head -n 1)"
tshark -r .kbd.pcap > .cap.txt 2>/dev/null; rc=$?; head -n 20 .cap.txt | sed "s/^ *//" > usb_capture.out
rec usb_capture "(capture of the run above)" "$TV" "tshark -r kbd.pcap | head -n 20" "$rc" \
    "note:      first 20 frames of the keyboard's capture; the polling continues until QEMU stops"
{ python3 -I pcap_extract.py .kbd.pcap 1 setup && python3 -I pcap_extract.py .kbd.pcap 2; } > .dev.hex && { python3 -I pcap_extract.py .kbd.pcap 5 setup && python3 -I pcap_extract.py .kbd.pcap 6; } > .cfg.hex; rc=$?
{ echo "frame 1 SETUP, frame 2 data (device descriptor):"; cat .dev.hex; echo "frame 5 SETUP, frame 6 data (configuration descriptor set):"; cat .cfg.hex; } > usb_bytes.out
rec usb_bytes pcap_extract.py "Python $(python3 -c 'import sys; print(sys.version.split()[0])')" \
    "python3 -I pcap_extract.py kbd.pcap N [setup]  for frames 1 (setup), 2, 5 (setup), 6" "$rc"
F="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
g++ $F usb_desc.cc -o .bin_desc || status=1
cat .dev.hex .cfg.hex | ./.bin_desc > usb_desc.out 2>&1; rc=$?
rec usb_desc usb_desc.cc "$(g++ --version | head -n 1)" \
    "g++ $F usb_desc.cc -o usb_desc && cat dev.hex cfg.hex | ./usb_desc" "$rc"
rm -f .bin_desc .kbd.pcap .dev.hex .cfg.hex .cap.txt
exit $status
