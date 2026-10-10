#!/usr/bin/env bash
# F2-48 lab: freestanding C++ and curriculum milestone P1.
#   step p1_clone       : P1 acceptance tests on a fresh git clone of the p1/ skeleton
#   step p1_inspect     : readelf / nm on the freestanding object (no host C library)
#   step errors         : what the freestanding target refuses (exceptions, RTTI, C++ headers)
#   step guard          : function-local statics need __cxa_guard_* unless told otherwise
#   step missing_runtime: linking the kernel without runtime.cpp
#   step kernel         : the kernel with runtime.cpp and .init_array, booted in QEMU
#   step forensic_old   : the same objects with an old linker script (global not constructed)
set -u
cd "$(dirname "$0")" || exit 2
LAB="$(pwd)"
status=0
TOOL="$(clang++ --version | head -n 1); $(ld.lld --version | head -n 1)"
QV="$(qemu-system-x86_64 --version | head -n 1)"
W="${LAB}/.work"; rm -rf "$W"; mkdir -p "$W"
CXXF="--target=i386-unknown-none-elf -std=c++20 -ffreestanding -fno-exceptions -fno-rtti -fno-pic -O2 -Wall -Wextra -Werror"
QEMU="qemu-system-x86_64 -display none -serial none -no-reboot -debugcon stdio -device isa-debug-exit,iobase=0xf4,iosize=0x04"

begin() {   # begin <name> <listing> <command summary> [extra toolchain] [hardware]
    NAME="$1"; LAST=0; BAD=0
    OUT="${LAB}/${NAME}.out"; LOG="${LAB}/${NAME}.log"; : > "$OUT"
    {
        echo "listing:   $2"
        echo "toolchain: $TOOL${4:-}"
        echo "command:   $3"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
        if [ -n "${5:-}" ]; then echo "hardware:  $5"; fi
    } > "$LOG"
    cd "$W" || exit 2
}
c() {
    echo "\$ $1" >> "$OUT"
    bash -c "$1" >> "$OUT" 2>&1
    LAST=$?
    if [ "$LAST" != "${2:-0}" ]; then BAD=$((BAD + 1)); fi
    if [ "$LAST" != 0 ]; then echo "(exit code: $LAST)" >> "$OUT"; fi
}
finish() {
    sed -i -e "s#${W}/##g" -e "s#${LAB}/##g" -e 's#/tmp/[A-Za-z0-9._-]*/#/tmp/<tmp>/#g' "$OUT"
    echo "exit code: $LAST" >> "$LOG"
    [ -n "${1:-}" ] && echo "$1" >> "$LOG"
    if [ "$BAD" != 0 ]; then echo "result:    STEP FAILED ($BAD command(s) did not give the expected exit code)" >> "$LOG"; status=1; fi
    cd "$LAB" || exit 2
}

# Step 1: P1 acceptance -- a clean clone builds with one command; the host test passes.
begin p1_clone "p1/Makefile p1/kernel/empty.cpp p1/tests/test_trivial.cpp p1/tools/toolchain.sh" "git init + commit of p1/, git clone into an empty folder, make" "; $(git --version); $(make --version | head -n 1)"
c "cp -r ${LAB}/p1 origin && cd origin && git init -q && git add . && git -c user.name=lab -c user.email=lab@invalid commit -q -m 'P1 skeleton' && git log --oneline | sed 's/^[0-9a-f]* /<commit> /'"
c "git clone -q origin clone && cd clone && find . -path ./.git -prune -o -type f -print | sort"
c "cd clone && make"
c "cd clone && make clean && make target && ls build/target"
finish "hardware:  the 'second machine' is a second folder in the same container (a real second machine was not available)"

# Step 2: P1 acceptance -- the freestanding object is ELF64 x86-64 with no host C library.
begin p1_inspect "p1/kernel/empty.cpp" "readelf -h, readelf -d, nm, objdump -d on build/target/empty.o of the clone"
c "readelf -hW clone/build/target/empty.o | grep -E 'Class|Machine|Type|OS/ABI'"
c "readelf -dW clone/build/target/empty.o"
c "nm clone/build/target/empty.o"
c "echo \"undefined symbols: \$(nm -u clone/build/target/empty.o | wc -l)\""
c "objdump -d --no-show-raw-insn clone/build/target/empty.o | sed -n '/<p1_answer>:/,\$p'"
finish

# Step 3: what the freestanding target refuses to compile.
begin errors "errors/throw.cpp errors/rtti.cpp errors/headers.cpp" "clang++ $CXXF -c on each file (each compile is expected to fail)"
c "clang++ $CXXF -c ${LAB}/errors/throw.cpp -o throw.o" 1
c "clang++ $CXXF -c ${LAB}/errors/rtti.cpp -o rtti.o" 1
c "clang++ $CXXF -c ${LAB}/errors/headers.cpp -o headers.o" 1
c "g++ -std=c++20 -fno-exceptions -c ${LAB}/errors/throw.cpp -o throw_gcc.o" 1
finish "note:      every compile in this step is expected to fail; the step passes when each one does"

# Step 4: guard variables for function-local statics.
begin guard "errors/guard.cpp" "clang++ $CXXF -c guard.cpp; nm -C; again with -fno-threadsafe-statics"
c "clang++ $CXXF -c ${LAB}/errors/guard.cpp -o guard.o && nm -C guard.o"
c "clang++ $CXXF -fno-threadsafe-statics -c ${LAB}/errors/guard.cpp -o guard2.o && nm -C guard2.o"
finish

K="${LAB}/kernel"
# Step 5: the kernel without its runtime support file.
begin missing_runtime "kernel/kmain.cpp kernel/boot.S" "ld.lld -m elf_i386 -T link.ld boot.o kmain.o (runtime.o left out; expected to fail)"
c "clang++ $CXXF -c $K/kmain.cpp -o kmain.o"
c "clang --target=i386-unknown-none-elf -c $K/boot.S -o boot.o"
c "nm -C kmain.o | grep ' U '"
c "ld.lld -m elf_i386 -T $K/link.ld boot.o kmain.o -o broken.elf" 1
finish "note:      the final link is expected to fail"

# Step 6: the complete kernel.
begin kernel "kernel/kmain.cpp kernel/runtime.cpp kernel/link.ld kernel/boot.S" "clang++ $CXXF -c; ld.lld -m elf_i386 -T link.ld -Map=kernel.map boot.o kmain.o runtime.o -o kernel.elf; $QEMU -kernel kernel.elf" "; $QV" "emulated PC (QEMU, TCG); untested on a physical machine"
c "clang++ $CXXF -c $K/runtime.cpp -o runtime.o"
c "ld.lld -m elf_i386 -T $K/link.ld -Map=kernel.map boot.o kmain.o runtime.o -o kernel.elf"
c "grep -E 'init_array|_GLOBAL__sub_I' kernel.map"
c "readelf -x .init_array kernel.elf"
c "nm -n kernel.elf | grep -E '_GLOBAL__sub_I|__init_array'"
c "timeout 20 $QEMU -kernel kernel.elf" 33
finish "note:      exit code 33 is expected: kmain wrote 0x10 to isa-debug-exit ((0x10 << 1) | 1)"

# Step 7 (forensic evidence): the same objects linked with an older script.
begin forensic_old "forensic/link_old.ld" "ld.lld -m elf_i386 -T link_old.ld -Map=old.map boot.o kmain.o runtime.o -o old.elf; $QEMU -kernel old.elf; map; objdump" "; $QV" "emulated PC (QEMU, TCG)"
c "ld.lld -m elf_i386 -T ${LAB}/forensic/link_old.ld -Map=old.map boot.o kmain.o runtime.o -o old.elf"
c "timeout 20 $QEMU -kernel old.elf" 35
c "grep -E -B1 -A3 '^ +[0-9a-f]+ +[0-9a-f]+ +[0-9a-f]+ +[0-9]+ \\.(ctors|init_array)' old.map"
c "objdump -d --no-show-raw-insn old.elf | sed -n '/<_Z21runGlobalConstructorsv>:/,/^\$/p'"
c "nm -n old.elf | grep -E '_GLOBAL__sub_I|__init_array'"
finish "note:      exit code 35 = (0x11 << 1) | 1: kmain reports the global was not constructed"
rm -rf "$W"
exit $status
