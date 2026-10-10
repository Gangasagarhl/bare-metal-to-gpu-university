#!/usr/bin/env bash
# kbuild.sh - OS302 shared kernel build (used by the run.sh of F3-18 ... F3-25).
#   kbuild.sh <out-name> "<extra flags>" <source files ...>
# Compiles each .cc / .S file freestanding, links with F3-18/linker.ld into <out-name>.elf,
# and makes the flat image <out-name>.bin that QEMU's Multiboot loader (-kernel) accepts.
set -eu
here="$(cd "$(dirname "$0")" && pwd)"
out="$1"; extra="$2"; shift 2
CXXFLAGS="-std=c++20 -ffreestanding -fno-exceptions -fno-rtti -fno-threadsafe-statics \
-fno-stack-protector -fno-pic -fno-omit-frame-pointer -mno-red-zone -mcmodel=kernel \
-mgeneral-regs-only -O2 -g -Wall -Wextra -Wpedantic -Werror -I$here $(for d in "$here"/../F3-*; do printf -- "-I%s " "$d"; done) $extra"
objs=()
odir=".obj_$(basename "$out")"
rm -rf "$odir"; mkdir -p "$odir"
for src in "$@"; do
    o="$odir/$(basename "$src").o"
    g++ $CXXFLAGS -c "$src" -o "$o"
    objs+=("$o")
done
ld -nostdlib -static -z max-page-size=0x1000 --no-warn-rwx-segments -T "$here/linker.ld" -o "$out.elf" "${objs[@]}"
objcopy -O binary "$out.elf" "$out.bin"
rm -rf "$odir"
