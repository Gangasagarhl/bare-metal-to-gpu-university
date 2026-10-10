#!/usr/bin/env bash
# F3-50 run.sh: (1) nolibc_hello.cc built for x86-64, AArch64 and RISC-V 64 and run (natively and
# under QEMU user mode); (2) the system-call instruction in each binary (objdump); (3) the system
# calls made by the program without and with the C library (strace); (4) ENOSYS as a probe:
# clone3 made to fail, the C library falls back.
set -u -o pipefail
cd "$(dirname "$0")"
. ./oslib.sh
FREE="-std=c++20 -O2 -ffreestanding -fno-exceptions -fno-rtti -fno-stack-protector -fno-pie -nostdlib -static -no-pie $WFLAGS"
rm -rf .o; mkdir -p .o

# 1. build and run on three architectures
g++ $FREE nolibc_hello.cc -o .o/h_x86 > nolibc_build.out 2>&1 &&
aarch64-linux-gnu-g++ $FREE nolibc_hello.cc -o .o/h_a64 >> nolibc_build.out 2>&1 &&
riscv64-linux-gnu-g++ $FREE nolibc_hello.cc -o .o/h_rv >> nolibc_build.out 2>&1; rc=$?
[ -s nolibc_build.out ] || echo "(no messages: no warnings, no errors)" > nolibc_build.out
rec nolibc_build "nolibc_hello.cc" "$GXX_VER; $A64_VER; $RV_VER" "<g++|aarch64-linux-gnu-g++|riscv64-linux-gnu-g++> $FREE nolibc_hello.cc -o hello_<arch>" "$rc"
expect "$rc" 0 nolibc_build
./.o/h_x86 > nolibc_x86.out 2>&1; rc=$?
rec nolibc_x86 "nolibc_hello.cc (x86-64)" "$GXX_VER" "./hello_x86" "$rc"
expect "$rc" 0 nolibc_x86
qemu-aarch64 ./.o/h_a64 > nolibc_a64.out 2>&1; rc=$?
rec nolibc_a64 "nolibc_hello.cc (AArch64)" "$A64_VER; $QA64_VER" "qemu-aarch64 ./hello_a64" "$rc" "$HW_EMU"
expect "$rc" 0 nolibc_a64
qemu-riscv64 ./.o/h_rv > nolibc_rv.out 2>&1; rc=$?
rec nolibc_rv "nolibc_hello.cc (RISC-V 64)" "$RV_VER; $QRV_VER" "qemu-riscv64 ./hello_rv" "$rc" "$HW_EMU"
expect "$rc" 0 nolibc_rv

ends() { local a; a="$(cat)"; echo "$a" | head -n 2; echo "  [...]"; echo "$a" | tail -n 2; }
# 2. the system-call instruction and the register that carries the number, per architecture
{ echo "== x86-64 (objdump -d, function sys3 inlined into cmain; first and last lines with the number register or the instruction) =="
  objdump -d --no-show-raw-insn .o/h_x86 | grep -E 'syscall|mov +\$0x(1|3c),%(eax|r8d)' | ends
  echo "== AArch64 =="
  aarch64-linux-gnu-objdump -d --no-show-raw-insn .o/h_a64 | grep -E 'svc|mov[[:space:]]+x8' | ends
  echo "== RISC-V 64 =="
  riscv64-linux-gnu-objdump -d --no-show-raw-insn .o/h_rv | grep -E 'ecall|li[[:space:]]+a7' | ends
} | sed -E 's/^ +([0-9a-f]+):/  \1:/' > instr.out 2>&1; rc=$?
rec instr "nolibc_hello.cc binaries" "$BINUTILS_VER (and the aarch64/riscv64 cross objdump of the same version)" "objdump -d --no-show-raw-insn <binary> | grep <instruction and number register>" "$rc"
expect "$rc" 0 instr

# 3. system calls with and without the C library
g++ -std=c++20 -O2 $WFLAGS -static hello_libc.cc -o .o/hello_static && g++ -std=c++20 -O2 $WFLAGS hello_libc.cc -o .o/hello_dynamic; rc=$?
expect "$rc" 0 hello_build
{ for p in h_x86 hello_static hello_dynamic; do
    strace -qq -o .o/st.txt ./.o/$p > /dev/null; n=$(wc -l < .o/st.txt)
    echo "== $p: $n system calls, in order of first use =="
    sed -E 's/\(.*//' .o/st.txt | awk '!seen[$0]++' | tr '\n' ' ' | fold -s -w 96; echo
  done; } > syscalls.out 2>&1; rc=$?
rec syscalls "nolibc_hello.cc; hello_libc.cc built -static and dynamically" "$STRACE_VER; $GXX_VER; $LDD_VER" \
    "strace -qq -o trace.txt ./<program> > /dev/null; count lines; list names in order of first use" "$rc"
expect "$rc" 0 syscalls

# 4. ENOSYS as a probe: make clone3 fail and watch the C library fall back
g++ -std=c++20 -O2 $WFLAGS -pthread thread_probe.cc -o .o/thread_probe; rc=$?
expect "$rc" 0 thread_build
strace -f -qq -o .o/st2.txt -e trace=clone,clone3 -e inject=clone3:error=ENOSYS ./.o/thread_probe > .o/tp.txt 2>&1; rc=$?
{ echo "== program output =="; cat .o/tp.txt
  echo "== strace (process ids removed; long argument lists shortened to [...]) =="
  sed -E 's/^[0-9]+ +//; s/\{flags=[^}]*\}/{[...]}/; s/child_stack=0x[0-9a-f]+, flags=[A-Z_|]+/[...]/; s/= [0-9]+$/= <thread id>/' .o/st2.txt; } > enosys_probe.out
rec enosys_probe "thread_probe.cc" "$STRACE_VER; $GXX_VER; $LDD_VER" \
    "strace -f -qq -o trace.txt -e trace=clone,clone3 -e inject=clone3:error=ENOSYS ./thread_probe" "$rc" \
    "note:      strace's inject option makes the kernel's answer to clone3 look like ENOSYS; the program was not changed"
expect "$rc" 0 enosys_probe

# 5. a call that does not enter the kernel: clock_gettime through the vDSO
g++ -std=c++20 -O2 $WFLAGS clock_loop.cc -o .o/clock_loop; rc=$?
expect "$rc" 0 clock_build
{ ./.o/clock_loop
  strace -qq -o .o/st3.txt ./.o/clock_loop > /dev/null
  echo "clock_gettime system calls seen by strace: $(grep -c '^clock_gettime' .o/st3.txt)"
  echo "objects mapped into the process whose name contains vdso: $(grep -c vdso /proc/self/maps)  (in the grep process itself, from its /proc/self/maps)"
} > vdso.out 2>&1; rc=$?
rec vdso "clock_loop.cc" "$STRACE_VER; $GXX_VER; $LDD_VER" "./clock_loop; strace -qq -o trace.txt ./clock_loop; grep -c '^clock_gettime' trace.txt" "$rc"
expect "$rc" 0 vdso

rm -rf .o
exit $status
