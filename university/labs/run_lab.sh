#!/usr/bin/env bash
# Lab Engineer runner: builds and runs every listing of one chapter's lab folder
# and records what really happened (guide AH-24, AH-25).
#
#   university/labs/run_lab.sh university/labs/F0-29
#
# For every <name>.cpp in the folder:
#   - compiles with the university's host toolchain and flags (CXXFLAGS below);
#   - if <name>.expect-fail exists, the compile is EXPECTED to fail: the compiler's
#     messages are saved as the output (used by chapters about reading errors);
#   - otherwise runs the program, feeding <name>.in on stdin if present,
#     with a time limit (<name>.timeout seconds, default 10);
#   - writes <name>.out  : everything the compiler (on failure) or program printed
#            <name>.log  : command, toolchain version line, date, exit code
# For every <name>.cu (CUDA) and <name>.hip (HIP) the same is done with nvcc / hipcc.
# The build container has the compilers but NO GPU, so the run is attempted and its
# real output (the runtime's "no device" error) is recorded, and the log says
# "hardware: untested on hardware" (guide AH-26).
# If the folder has an executable run.sh (QEMU boots, gdb sessions, multi-step labs),
# it is run last; it must write its own <name>.out / <name>.log in the same format.
set -u
dir="${1:?usage: run_lab.sh <lab folder>}"
CXX="${CXX:-g++}"
CXXFLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
cd "$dir" || exit 2
status=0
shopt -s nullglob
for src in *.cpp; do
    name="${src%.cpp}"
    bin="./.bin_${name}"
    ver="$($CXX --version | head -n 1)"
    cmd="$CXX $CXXFLAGS $src -o ${name}"
    {
        echo "listing:   $src"
        echo "toolchain: $ver"
        echo "command:   $cmd"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
    } > "${name}.log"
    if [ -f "${name}.expect-fail" ]; then
        if $CXX $CXXFLAGS "$src" -o "$bin" > "${name}.out" 2>&1; then
            echo "result:    UNEXPECTED SUCCESS (compile was expected to fail)" >> "${name}.log"; status=1
        else
            echo "result:    compile failed as expected (messages saved in ${name}.out)" >> "${name}.log"
        fi
        # make the saved messages independent of the machine's folder names
        sed -i "s#$(pwd)/##g" "${name}.out"
        rm -f "$bin"; continue
    fi
    if ! $CXX $CXXFLAGS "$src" -o "$bin" > "${name}.build.txt" 2>&1; then
        echo "result:    BUILD FAILED" >> "${name}.log"; cat "${name}.build.txt" >> "${name}.log"; status=1
        rm -f "${name}.build.txt"; continue
    fi
    rm -f "${name}.build.txt"
    t=10; [ -f "${name}.timeout" ] && t="$(cat "${name}.timeout")"
    if [ -f "${name}.in" ]; then
        timeout "$t" "$bin" < "${name}.in" > "${name}.out" 2>&1; rc=$?
    else
        timeout "$t" "$bin" < /dev/null > "${name}.out" 2>&1; rc=$?
    fi
    sed -i "s#$(pwd)/##g" "${name}.out"
    if [ "$rc" = 124 ]; then
        echo "exit code: 124 (stopped by the ${t} s time limit)" >> "${name}.log"
    else
        echo "exit code: $rc" >> "${name}.log"
    fi
    if [ -f "${name}.in" ]; then echo "stdin:     ${name}.in" >> "${name}.log"; fi
    rm -f "$bin"
done
for src in *.cu *.hip; do
    name="${src%.*}"
    bin="./.bin_${name}"
    if [ "${src##*.}" = "cu" ]; then
        tc="nvcc"; ver="$(nvcc --version | tail -n 2 | head -n 1)"
        cmd="nvcc -std=c++17 -O2 -lineinfo -Werror all-warnings $src -o ${name}"
        bcmd=(nvcc -std=c++17 -O2 -lineinfo -Werror all-warnings "$src" -o "$bin")
        hw="untested on hardware: the build container has no NVIDIA GPU (AH-26); the build is real, the run shows the runtime's own error"
    else
        tc="hipcc"; ver="$(hipcc --version 2>/dev/null | head -n 1) (offload target gfx90a)"
        cmd="hipcc -std=c++17 -O2 --offload-arch=gfx90a $src -o ${name}"
        bcmd=(hipcc -std=c++17 -O2 --offload-arch=gfx90a "$src" -o "$bin")
        hw="untested on hardware: the build container has no AMD GPU (AH-26); the build is real, the run shows the runtime's own error"
    fi
    {
        echo "listing:   $src"
        echo "toolchain: $ver"
        echo "command:   $cmd"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
        echo "hardware:  $hw"
    } > "${name}.log"
    if [ -f "${name}.expect-fail" ]; then
        if "${bcmd[@]}" > "${name}.out" 2>&1; then
            echo "result:    UNEXPECTED SUCCESS (compile was expected to fail)" >> "${name}.log"; status=1
        else
            echo "result:    compile failed as expected (messages saved in ${name}.out)" >> "${name}.log"
        fi
        sed -i "s#$(pwd)/##g" "${name}.out"; rm -f "$bin"; continue
    fi
    if ! "${bcmd[@]}" > "${name}.build.txt" 2>&1; then
        echo "result:    BUILD FAILED" >> "${name}.log"; cat "${name}.build.txt" >> "${name}.log"; status=1
        rm -f "${name}.build.txt"; continue
    fi
    rm -f "${name}.build.txt"
    timeout 20 "$bin" < /dev/null > "${name}.out" 2>&1; rc=$?
    sed -i "s#$(pwd)/##g" "${name}.out"
    echo "exit code: $rc" >> "${name}.log"
    rm -f "$bin"
done
if [ -x ./run.sh ]; then
    ./run.sh || status=1
fi
exit $status
