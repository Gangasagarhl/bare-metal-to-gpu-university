#!/usr/bin/env bash
# Runs for F3-07: the machine's memory map, the uni-rv system-call kernel in QEMU (normal,
# forensic and extension builds), the user program's machine code, and strace of Listing 1.
set -u
cc=riscv64-linux-gnu-g++
ccver="$($cc --version | head -n 1)"
qver="$(qemu-system-riscv64 --version | head -n 1)"
gxx="$(g++ --version | head -n 1)"
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

# 1. the emulated machine's memory map, as QEMU itself reports it
header memmap "QEMU monitor command 'info mtree' on the virt machine (stopped before the first instruction), selected lines" \
    "$qemu -S -qmp unix:qmp.sock,server,nowait; python3 qmp_cmd.py qmp.sock 'info mtree' | grep ..." "$qver; $(python3 --version)"
rm -f qmp.sock
timeout 5 $qemu -S -qmp unix:qmp.sock,server,nowait > /dev/null 2>&1 < /dev/null &
qpid=$!
sleep 1
python3 qmp_cmd.py qmp.sock "info mtree" | grep -E 'riscv_virt_board\.(ram|mrom)|sifive\.test|aclint|plic|: serial' | awk '!seen[$0]++' | sed 's/^ *//' > memmap.out
python3 qmp_cmd.py qmp.sock "quit" > /dev/null 2>&1
wait $qpid
echo "exit code: 0" >> memmap.log
rm -f qmp.sock

# 2. the kernel, three builds
run_kernel() {   # run_kernel <name> <defines> <description>
    header "$1" "entry.S + trap.S + kernel.cc ${2:-(no extra defines)}" "$cc $kflags $2 -T kernel.ld entry.S trap.S kernel.cc -o $1.elf && $qemu -kernel $1.elf" "$ccver; $qver"
    if ! $cc $kflags $2 -T kernel.ld entry.S trap.S kernel.cc -o "$1.elf" > "$1.build.txt" 2>&1; then
        echo "result:    BUILD FAILED" >> "$1.log"; cat "$1.build.txt" >> "$1.log"; status=1; return
    fi
    rm -f "$1.build.txt"
    timeout 10 $qemu -kernel "$1.elf" > "$1.out" 2>&1 < /dev/null; rc=$?
    echo "exit code: $rc ($3)" >> "$1.log"
}
run_kernel syscalls "" "0 = the kernel powered off after the process exited"
run_kernel device_fault "-DTOUCH_DEVICE" "1 = the kernel powered off after killing the process"
run_kernel leak "-DBUGGY_CHECK" "0 = the kernel powered off after the process exited"

# 3. evidence for the forensic lab: where the kernel's secret lives, from the ELF symbol table
header leak_nm "nm on the forensic build's ELF file" "riscv64-linux-gnu-nm -C leak.elf | grep -E 'kernel_note|user_start|user_end'" "$(riscv64-linux-gnu-nm --version | head -n 1)"
riscv64-linux-gnu-nm -C leak.elf | grep -E 'kernel_note|user_start|user_end' > leak_nm.out; echo "exit code: 0" >> leak_nm.log

# 4. the user program's machine code: a7 = number, a0/a1 = arguments, then ecall
header user_disasm "objdump of user_main (normal build)" "riscv64-linux-gnu-objdump -d --no-show-raw-insn syscalls.elf, function user_main" "$(riscv64-linux-gnu-objdump --version | head -n 1)"
riscv64-linux-gnu-objdump -d --no-show-raw-insn syscalls.elf | sed -n '/<user_main>:/,/^$/p' > user_disasm.out
echo "exit code: 0" >> user_disasm.log
rm -f syscalls.elf device_fault.elf leak.elf

# 5. Listing 1 under strace (plain build, so no sanitizer calls are mixed in).
#    Run twice: standard output to a regular file, then to /dev/null.
st="$(strace -V | head -n 1)"
g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -O2 raw_syscall.cpp -o raw_plain || exit 1
for target in file devnull; do
    if [ "$target" = file ]; then dest=raw_stdout.txt; else dest=/dev/null; fi
    header "raw_strace_$target" "raw_syscall.cpp (plain build) under strace, standard output sent to $dest; lines after the program's first write" \
        "g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -O2 raw_syscall.cpp -o raw_plain && strace -e signal=none -o trace.txt ./raw_plain > $dest" "$st; $gxx"
    strace -e signal=none -o trace.txt ./raw_plain > "$dest"; echo "exit code: $?" >> "raw_strace_$target.log"
    sed -n '/^write(1, "1. printf/,$p' trace.txt > "raw_strace_$target.out"
    rm -f trace.txt
done
rm -f raw_plain raw_stdout.txt
exit $status
