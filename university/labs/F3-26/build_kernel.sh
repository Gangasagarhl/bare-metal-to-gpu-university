#!/usr/bin/env bash
# build_kernel.sh <output dir> [extra compiler flags, e.g. -DBUG_...]
# Builds the OS303 teaching kernel from the five lab folders F3-26 ... F3-30 and the
# user programs (u_*.cc) that the kernel loads as Multiboot modules.
set -euo pipefail
here="$(cd "$(dirname "$0")" && pwd)"
labs="$(dirname "$here")"
out="$1"; shift
mkdir -p "$out"
dirs=("$labs/F3-26" "$labs/F3-27" "$labs/F3-28" "$labs/F3-29" "$labs/F3-30")
inc=(); for d in "${dirs[@]}"; do inc+=("-I$d"); done
KFLAGS=(-std=c++20 -O2 -g -ffreestanding -fno-exceptions -fno-rtti -fno-stack-protector
        -fno-pic -fno-pie -mno-red-zone -mgeneral-regs-only -fno-omit-frame-pointer
        -fno-threadsafe-statics -fno-tree-loop-distribute-patterns --param=min-pagesize=0
        -Wall -Wextra -Werror "${inc[@]}" "$@")
objs=()
for d in "${dirs[@]}"; do
    for src in "$d"/*.cc "$d"/*.S; do
        [ -e "$src" ] || continue
        base="$(basename "$src")"
        case "$base" in u_*|crt0.S|ulib.cc) continue ;; esac     # user programs are built below
        obj="$out/$(basename "$d")_${base}.o"
        g++ "${KFLAGS[@]}" -c "$src" -o "$obj"
        objs+=("$obj")
    done
done
ld -nostdlib -static -z max-page-size=0x1000 -z noexecstack -T "$here/kernel.ld" -o "$out/kernel.elf" "${objs[@]}"
# QEMU's Multiboot loader accepts only 32-bit ELF files: same bytes, 32-bit headers.
objcopy -O elf32-i386 "$out/kernel.elf" "$out/kernel32.elf"

# User programs: freestanding, static, linked into the user half (F3-29, F3-30).
UFLAGS=(-std=c++20 -O2 -g -ffreestanding -fno-exceptions -fno-rtti -fno-stack-protector
        -fno-pic -fno-pie -no-pie -static -nostdlib -mgeneral-regs-only -mcmodel=large
        -Wall -Wextra -Werror "-I$labs/F3-29" "$@"
        -Wl,-Ttext-segment=0x8000400000 -Wl,--build-id=none)
for d in "${dirs[@]}"; do
    for src in "$d"/u_*.cc; do
        [ -e "$src" ] || continue
        name="$(basename "${src%.cc}")"
        g++ "${UFLAGS[@]}" "$labs/F3-29/crt0.S" "$labs/F3-29/ulib.cc" "$src" -o "$out/$name.elf"
    done
done
