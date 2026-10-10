#!/usr/bin/env bash
# Runs for F3-08: a guided tour of the uni-rv kernel (the stand-in for xv6 in this build),
# its forensic build, the lab's "why a system call?" experiment and the lab's reference solution.
set -u
cc=riscv64-linux-gnu-g++
ccver="$($cc --version | head -n 1)"
qver="$(qemu-system-riscv64 --version | head -n 1)"
kflags="-march=rv64gc -mabi=lp64d -mcmodel=medany -ffreestanding -nostdlib -fno-exceptions -fno-rtti -O2 -g -Wall -Wextra -Werror -std=c++20 -static -Wl,--build-id=none -Wl,--no-warn-rwx-segments"
qemu="qemu-system-riscv64 -machine virt -bios none -nographic"
header() {   # header <name> <listing> <command> <toolchain>
    {
        echo "listing:   $2"
        echo "toolchain: $4"
        echo "command:   $3"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container); emulated machine: QEMU virt, RISC-V"
    } > "$1.log"
}
status=0
run_kernel() {   # run_kernel <name> <source> <defines> <meaning of the exit code>
    header "$1" "entry.S + trap.S + $2 ${3:-(no extra defines)}" "$cc $kflags $3 -T kernel.ld entry.S trap.S $2 -o $1.elf && timeout 10 $qemu -kernel $1.elf" "$ccver; $qver"
    if ! $cc $kflags $3 -T kernel.ld entry.S trap.S "$2" -o "$1.elf" > "$1.build.txt" 2>&1; then
        echo "result:    BUILD FAILED" >> "$1.log"; cat "$1.build.txt" >> "$1.log"; status=1; return
    fi
    rm -f "$1.build.txt"
    timeout 10 $qemu -kernel "$1.elf" > "$1.out" 2>&1 < /dev/null; rc=$?
    echo "exit code: $rc ($4)" >> "$1.log"
}
run_kernel tour kernel.cc "" "0 = powered off by the kernel after the process exited"
run_kernel table_bug kernel.cc "-DBUGGY_TABLE" "0 = powered off by the kernel after the process exited"
run_kernel hartid_direct kernel.cc "-DREAD_HARTID_DIRECTLY" "1 = powered off by the kernel after killing the process"
run_kernel solution_hartid solution_hartid.cc "" "0 = powered off by the kernel after the process exited"

# The tour's map: every function of the kernel image, in address order (nm), and file sizes.
header symbols "nm on the tour build's ELF file: code symbols (T/t), in address order" "riscv64-linux-gnu-nm -n -C tour.elf | grep -E ' [Tt] '" "$(riscv64-linux-gnu-nm --version | head -n 1)"
riscv64-linux-gnu-nm -n -C tour.elf | grep -E ' [Tt] ' > symbols.out; echo "exit code: 0" >> symbols.log
header sizes "line counts of the kernel's source files" "wc -l entry.S trap.S kernel.ld kernel.cc" "$(wc --version | head -n 1)"
wc -l entry.S trap.S kernel.ld kernel.cc > sizes.out; echo "exit code: 0" >> sizes.log
rm -f tour.elf table_bug.elf hartid_direct.elf solution_hartid.elf
exit $status
