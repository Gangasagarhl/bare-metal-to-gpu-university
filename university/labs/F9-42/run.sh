#!/usr/bin/env bash
# Extra runs for F9-42 (same compiler and flags as run_lab.sh):
#   tape_test  bringup.cpp on tape_test.in: forensic evidence (odometry twice the tape measure)
set -u
GXX="$(g++ --version | head -n 1)"
CXXFLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
status=0
runcase() {   # runcase <name> <source.cpp> <input>
    local name="$1" src="$2" in="$3"
    {
        echo "listing:   $src"
        echo "toolchain: $GXX"
        echo "command:   g++ $CXXFLAGS $src -o ${src%.cpp}; ./${src%.cpp} < $in"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
    } > "$name.log"
    if g++ $CXXFLAGS "$src" -o ".bin_$name" > .build.txt 2>&1; then
        timeout 10 "./.bin_$name" < "$in" > "$name.out" 2>&1; rc=$?
        echo "exit code: $rc" >> "$name.log"; echo "stdin:     $in" >> "$name.log"
        [ "$rc" = 0 ] || status=1
    else
        echo "result:    BUILD FAILED" >> "$name.log"; cat .build.txt >> "$name.log"; status=1
    fi
    rm -f .build.txt ".bin_$name"
}
runcase tape_test bringup.cpp tape_test.in
# launch_syntax: Python's own syntax check of robot.launch.py, then an import attempt of the ROS 2
# launch module (expected to fail: ROS 2 is not installed). Untested as a launch file.
PYV="$(python3 --version 2>&1)"
{
    echo "listing:   robot.launch.py"
    echo "toolchain: $PYV"
    echo "command:   python3 -m py_compile robot.launch.py; python3 -c 'import launch'"
    echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
} > launch_syntax.log
{
    if python3 -m py_compile robot.launch.py; then echo "py_compile: robot.launch.py: syntax OK"; else echo "py_compile: FAILED"; fi
    python3 -c 'import launch' 2>&1 | tail -n 1
} > launch_syntax.out 2>&1
rm -rf __pycache__
echo "exit code: 0 (syntax check passed; the import failure is expected without ROS 2)" >> launch_syntax.log
echo "hardware:  untested: no ROS 2 installation in the build container; run with the ROS 2 launch command in a sourced workspace" >> launch_syntax.log
grep -q "syntax OK" launch_syntax.out || status=1
exit $status
