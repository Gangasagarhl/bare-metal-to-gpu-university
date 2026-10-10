#!/usr/bin/env bash
# F9-69 run.sh: (1) the forensic scenario of the safety supervisor (Listing 1 runs through
# run_lab.sh with safety.in); (2) the e-stop test of the F9-67 robot image in QEMU: press the
# e-stop from the QEMU monitor while the control loop runs, then check the safe state.
set -u -o pipefail
cd "$(dirname "$0")"
L67=../F9-67
. $L67/rblib.sh
status=0
B=.build
rm -rf "$B"; mkdir -p "$B"
ok() { [ "$1" = "$2" ] || { echo "F9-69 step $3: exit $1, expected $2" >&2; status=1; }; }

# 1. forensic evidence: the supervisor with the release-3 configuration
hostbuild() { local out="$1"; shift; g++ $HOSTFLAGS "$@" -o "$out"; }
hostbuild $B/safety safety.cpp && timeout 10 $B/safety < safety_forensic.in > forensic_safety.out 2>&1; rc=$?
rec forensic_safety "safety.cpp" "$GXX_VER" "g++ $HOSTFLAGS safety.cpp -o safety; ./safety < safety_forensic.in" "$rc" \
    "stdin:     safety_forensic.in"
ok "$rc" 0 forensic_safety

# 2. the robot image of F9-67, built from its sources with its configuration
python3 $L67/mkconfig.py $L67/robot.config $B/.config $B/robot_config.h > /dev/null
kbuild $B/Image $B/o $B $L67/robot.cc > $B/kb.txt 2>&1; rc=$?
ok "$rc" 0 kbuild
python3 $L67/minidtc.py $L67/robot.dts $B/robot.dtb > /dev/null

# 3. the e-stop test: the loop may run 8000 cycles (8 s); the press comes after about 1 s
rm -f $B/mon.sock
python3 press.py $B/mon.sock 1.0 > $B/press.txt 2>&1 &
pp=$!
qrun 30 -- $ROBOT_VM -display none -serial stdio -monitor unix:$B/mon.sock,server,nowait \
    -semihosting -kernel $B/Image -dtb $B/robot.dtb -append "robot.cycles=8000" > $B/serial.txt; rc=$?
wait $pp
{ echo "--- host side"; cat $B/press.txt; echo "--- serial console of the robot image"; cat $B/serial.txt; } > estop_test.out
rec estop_test "F9-67 robot image (robot.cc, robot.dts, robot.config) and press.py" "$QEMU_VER; $XGXX_VER" \
    "$ROBOT_VM -display none -serial stdio -monitor unix:mon.sock,server,nowait -semihosting -kernel Image -dtb robot.dtb -append \"robot.cycles=8000\" & python3 press.py mon.sock 1.0" "$rc" \
    "note:      exit code 0 = e-stop seen, safe state held, and no non-zero actuator command after it" "$EMU_NOTE"
ok "$rc" 0 estop_test
grep -q "E-STOP at cycle" estop_test.out || { echo "F9-69: e-stop not seen" >&2; status=1; }

rm -rf "$B"
exit $status
