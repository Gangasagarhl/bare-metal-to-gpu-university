#!/usr/bin/env bash
# build_kernel.sh <out.elf> <source files...> - builds one DR404 lab kernel (64-bit,
# freestanding) and also writes <out>32.elf, the same bytes with 32-bit ELF headers,
# because QEMU's Multiboot loader accepts only 32-bit ELF files.
set -euo pipefail
here="$(cd "$(dirname "$0")" && pwd)"
out="$1"; shift
KFLAGS=(-std=c++20 -O2 -ffreestanding -fno-exceptions -fno-rtti -fno-stack-protector
        -fno-pic -fno-pie -mno-red-zone -mgeneral-regs-only -fno-threadsafe-statics
        -fno-tree-loop-distribute-patterns -Wall -Wextra -Werror "-I$here" ${KEXTRA:-})
odir="$(mktemp -d)"
objs=()
for src in "$@"; do
    obj="$odir/$(basename "$src").o"
    g++ "${KFLAGS[@]}" -c "$src" -o "$obj"
    objs+=("$obj")
done
ld -nostdlib -static -z max-page-size=0x1000 -z noexecstack --no-warn-rwx-segments \
   -T "$here/kernel.ld" -o "$out" "${objs[@]}"
objcopy -O elf32-i386 "$out" "${out%.elf}32.elf"
rm -rf "$odir"
