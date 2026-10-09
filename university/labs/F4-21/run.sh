#!/usr/bin/env bash
# F4-21 run.sh: HID over I2C, the laptop touchpad, without a laptop.
#   extract    hid_extract.py: report descriptors found by shape inside the QEMU program file
#   hid_dump   hid_parse.cc + hid_dump.cc: parse each one, self-check report lengths
#              (expected exit 1: the self-check flags one descriptor, see the chapter)
#   touchpad   i2c_hid_sim.cc: probe, reset, power on, 200 ticks of a level-triggered interrupt
#   forensic   the same, built with -DF421_NO_READ: the handler never reads the input register
set -u -o pipefail
cd "$(dirname "$0")"
. ../F4-14/dr401lib.sh
status=0

python3 hid_extract.py "$(command -v $QEMU)" > extract.out 2>&1; rc=$?
rec extract "hid_extract.py" "$PY_VER; input: $QEMU_VER program file" "python3 hid_extract.py \$(command -v $QEMU)" "$rc"
[ "$rc" = 0 ] || status=1
grep -v '^#' extract.out > .desc.txt

hostbuild .hid_dump hid_dump.cc hid_parse.cc && ./.hid_dump < .desc.txt > hid_dump.out 2>&1; rc=$?
rec hid_dump "hid_dump.cc + hid_parse.cc" "$GXX_VER" "g++ $HOSTFLAGS hid_dump.cc hid_parse.cc -o hid_dump; ./hid_dump < (descriptor lines of extract.out)" "$rc" \
    "note:      exit code 1 expected: the length self-check flags the descriptor at offset 13113840"
[ "$rc" = 1 ] || status=1

hostbuild .sim i2c_hid_sim.cc hid_parse.cc && ./.sim .desc.txt > touchpad.out 2>&1; rc=$?
rec touchpad "i2c_hid_sim.cc + hid_parse.cc" "$GXX_VER" "g++ $HOSTFLAGS i2c_hid_sim.cc hid_parse.cc -o i2c_hid_sim; ./i2c_hid_sim descriptors" "$rc" \
    "hardware:  untested on hardware: a software model of an I2C bus and a HID-over-I2C device; no laptop in the build"
[ "$rc" = 0 ] || status=1

g++ $HOSTFLAGS -DF421_NO_READ i2c_hid_sim.cc hid_parse.cc -o .simf && ./.simf .desc.txt > forensic.out 2>&1; rc=$?
rec forensic "i2c_hid_sim.cc built with -DF421_NO_READ" "$GXX_VER" "g++ $HOSTFLAGS -DF421_NO_READ i2c_hid_sim.cc hid_parse.cc -o sim_f; ./sim_f descriptors" "$rc" \
    "note:      exit code 1 expected: interrupts on ticks with no touch"
[ "$rc" = 1 ] || status=1

rm -f .hid_dump .sim .simf .desc.txt
exit $status
