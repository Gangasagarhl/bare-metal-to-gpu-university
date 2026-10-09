#!/usr/bin/env bash
# Builds Listing 1 (layout.cpp) and the lab program for two more processors (AArch64 and
# RISC-V 64) with the cross compilers, and runs them under QEMU user-mode emulation.
set -u
for arch in aarch64 riscv64; do
    cxx="${arch}-linux-gnu-g++"
    ver="$($cxx --version | head -n 1)"
    for prog in layout layout_lab; do
        name="${prog}_${arch}"
        cmd="$cxx -std=c++20 -Wall -Wextra -Wpedantic -Werror -O0 ${prog}.cpp -o $name; qemu-$arch -L /usr/${arch}-linux-gnu ./$name"
        {
            echo "listing:   ${prog}.cpp (cross-compiled for $arch, run under QEMU user mode)"
            echo "toolchain: $ver; $(qemu-$arch --version | head -n 1)"
            echo "command:   $cmd"
            echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
            echo "machine:   $(uname -s) $(uname -m) (cloud build container), emulating $arch"
            echo "hardware:  untested on real $arch hardware: emulated with QEMU user mode (AH-26)"
        } > "$name.log"
        if ! $cxx -std=c++20 -Wall -Wextra -Wpedantic -Werror -O0 "${prog}.cpp" -o "./.bin_$name" > "$name.out" 2>&1; then
            echo "result:    BUILD FAILED" >> "$name.log"; cat "$name.out" >> "$name.log"; exit 1
        fi
        qemu-$arch -L "/usr/${arch}-linux-gnu" "./.bin_$name" > "$name.out" 2>&1
        echo "exit code: $?" >> "$name.log"
        rm -f "./.bin_$name"
    done
done
exit 0
