#!/usr/bin/env bash
# build_all.sh - MP1 starter: build every kernel the regression matrix boots, into one folder.
#   build_all.sh <build dir> [extra compiler flags for the two MP1 kernels, e.g. -DMP1_MAX_CPUS=2]
# The MP1 kernels are new; every other kernel is built from the learner's earlier labs with
# the commands those labs use, from their own folders, without copying or editing a file:
#   x86-64  : F3-18 boot path + F4-23 neutral tests + F3-24 madt.h + mp1_core.cc mp1_x86.cc
#   aarch64 : F4-24 boot path + F4-23 neutral tests + F4-26 virtio_mmio.cc + mp1_core.cc mp1_a64.cc
#   reused  : OS303 teaching kernel (F3-26/build_kernel.sh, B9-B13 tests on the command line),
#             DR402 D1 kernel (F4-24) and D3 kernel (F4-26), with DR402's dlab.sh kbuild.
set -euo pipefail
here="$(cd "$(dirname "$0")" && pwd)"
labs="$(cd "$here/.." && pwd)"
out="$(mkdir -p "$1" && cd "$1" && pwd)"
extra="${2:-}"

# ---- x86-64 MP1 kernel: F3-18's flags (kbuild.sh) and linker script
X86FLAGS="-std=c++20 -ffreestanding -fno-exceptions -fno-rtti -fno-threadsafe-statics \
-fno-stack-protector -fno-pic -fno-omit-frame-pointer -mno-red-zone -mcmodel=kernel \
-mgeneral-regs-only -O2 -g -Wall -Wextra -Wpedantic -Werror"
X86INC="-I$here -I$labs/F3-18 -I$labs/F3-24 -I$labs/F4-23"
X86SRC="$labs/F3-18/boot.S $labs/F3-18/cxxrt.cc $labs/F3-18/serial.cc $labs/F3-18/kprint.cc \
$labs/F3-18/panic.cc $labs/F4-23/neutral_tests.cc $here/mp1_core.cc $here/mp1_x86.cc"
obj="$out/.obj_x86"; rm -rf "$obj"; mkdir -p "$obj"
objs=()
for s in $X86SRC; do
    o="$obj/$(basename "$s").o"
    g++ $X86FLAGS $X86INC $extra -c "$s" -o "$o"
    objs+=("$o")
done
ld -nostdlib -static -z max-page-size=0x1000 --no-warn-rwx-segments -T "$labs/F3-18/linker.ld" \
    -o "$out/mp1_x86.elf" "${objs[@]}"
objcopy -O binary "$out/mp1_x86.elf" "$out/mp1_x86.bin"
rm -rf "$obj"

# ---- AArch64 MP1 kernel: DR402's kbuild (F4-24 linker script, F3-18 files copied unchanged)
. "$labs/F4-23/dlab.sh"
cd "$out"
kbuild aarch64 mp1_a64 "-I$here -I$labs/F4-26 $extra" \
    "$labs/F4-24/boot.S" "$labs/F4-24/vectors.S" "$labs/F4-24/trap.cc" "$labs/F4-24/pl011.cc" \
    "$labs/F4-24/psci.cc" "$labs/F4-23/neutral_tests.cc" "$labs/F4-26/virtio_mmio.cc" \
    "$here/mp1_core.cc" "$here/mp1_a64.cc"

# ---- reused, unchanged: DR402 D1 (F4-24) and D3 (F4-26), as their run.sh build them
kbuild aarch64 d1 "" "$labs/F4-24/boot.S" "$labs/F4-24/vectors.S" "$labs/F4-24/trap.cc" \
    "$labs/F4-24/pl011.cc" "$labs/F4-24/psci.cc" "$labs/F4-24/d1_main.cc" \
    "$labs/F4-23/neutral_tests.cc"
KLDS="$labs/F4-25/linker.ld" kbuild aarch64 d3 "-I$labs/F4-25 -I$labs/F4-26" \
    "$labs/F4-24/boot.S" "$labs/F4-24/vectors.S" "$labs/F4-24/trap.cc" "$labs/F4-24/pl011.cc" \
    "$labs/F4-24/psci.cc" "$labs/F4-25/mmu.cc" "$labs/F4-25/gic.cc" "$labs/F4-26/smp.S" \
    "$labs/F4-26/virtio_mmio.cc" "$labs/F4-26/d3_main.cc"

# ---- reused, unchanged: the OS303 teaching kernel and its user programs (F3-26 ... F3-30)
"$labs/F3-26/build_kernel.sh" "$out/os303" > /dev/null
rm -f "$out"/os303/*.o
