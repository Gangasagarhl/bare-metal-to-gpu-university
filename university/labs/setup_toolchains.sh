#!/usr/bin/env bash
# The Lab Engineer's toolchain record (guide AH-11, dossier part E).
# This is exactly what was installed in the Ubuntu 24.04 build container before the
# labs were run. The versions each lab actually used are printed in its <name>.log.
#
# NOT available in the build container (so the matching labs are marked
# "untested on hardware", guide AH-26): an NVIDIA GPU, an AMD GPU, development
# boards, a robot kit, a drone kit, a second machine, RDMA NICs, internet access
# to vendor documentation.
set -e
apt-get update
DEBIAN_FRONTEND=noninteractive apt-get install -y -q \
    qemu-system-x86 qemu-system-arm qemu-system-misc ovmf qemu-efi-aarch64 \
    gcc-aarch64-linux-gnu g++-aarch64-linux-gnu gcc-riscv64-linux-gnu g++-riscv64-linux-gnu \
    mtools gdisk dosfstools
DEBIAN_FRONTEND=noninteractive apt-get install -y -q --no-install-recommends \
    nvidia-cuda-toolkit hipcc
DEBIAN_FRONTEND=noninteractive apt-get install -y -q --no-install-recommends \
    iverilog verilator yosys valgrind strace ltrace openmpi-bin libopenmpi-dev tshark \
    linux-tools-common elfutils binutils-aarch64-linux-gnu binutils-riscv64-linux-gnu \
    sqlite3 python3-numpy
# Already present in the image: g++ 13, clang/clang++, cmake, nasm, ld.lld, gdb, python3.
