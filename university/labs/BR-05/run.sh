#!/usr/bin/env bash
# BR-05 lab steps (run by run_lab.sh after core_test.cpp):
#   world 1a/1b  bare metal on the emulated microcontroller: superloop, then interrupt;
#   world 2      the RTOS (uRTOS v2 from F3-40), plus the forensic build;
#   trap 1       a blocking wait in an interrupt handler (expected to hang), and its fix;
#   trap 4 (MCU) dynamic allocation in firmware built without a library (expected link error);
#   world 3      Linux on this build machine: SCHED_OTHER and SCHED_FIFO, with background load;
#   trap 3       the running kernel's preemption model (is it PREEMPT_RT?);
#   trap 4 (PC)  allocation in the Linux control loop;
#   trap 2       priority inversion with POSIX threads, without and with priority inheritance;
#   footprint    image and process sizes; compare  one table from all SUMMARY lines.
set -u -o pipefail
cd "$(dirname "$0")"
status=0
B=.build
rm -rf "$B"; mkdir -p "$B"
rec() {  # rec <name> <listing> <toolchain> <command> <exit code> [extra lines...]
    local name="$1" listing="$2" tc="$3" cmd="$4" rc="$5"; shift 5
    {
        echo "listing:   $listing"
        echo "toolchain: $tc"
        echo "command:   $cmd"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container), kernel $(uname -r) ($(uname -v)), $(nproc) CPUs"
        echo "exit code: $rc"
        for extra in "$@"; do echo "$extra"; done
    } > "$name.log"
}

# ---------------------------------------------------------------- emulated microcontroller
ARMFLAGS="--target=thumbv7m-none-eabi -mcpu=cortex-m3 -std=c++20 -ffreestanding -fno-exceptions -fno-rtti -nostdlib -Os -g -Wall -Wextra -Wpedantic -Werror"
QEMU="qemu-system-arm -M mps2-an385 -nographic -monitor none -serial stdio -semihosting-config enable=on,target=native -icount shift=0,sleep=off"
TCA="$(clang++ --version | head -n 1); $(ld.lld --version | head -n 1); $(qemu-system-arm --version | head -n 1)"
HWQ="hardware:  untested on hardware; QEMU mps2-an385 (Cortex-M3 model) with -icount shift=0,sleep=off: virtual time advances by a fixed amount per instruction, so timing is repeatable but is not the timing of any real chip"
mcu_build() {  # mcu_build <elf> <extra flags> <sources...>
    local elf="$1" extra="$2"; shift 2
    local objs=() s
    clang++ $ARMFLAGS $extra -c startup.cc -o "$B/startup.o" || return 1
    for s in "$@"; do
        clang++ $ARMFLAGS $extra -c "$s" -o "$B/${s%.cc}.o" || return 1
        objs+=("$B/${s%.cc}.o")
    done
    ld.lld -T os305.ld --gc-sections "$B/startup.o" "${objs[@]}" -o "$B/$elf"
}
mcu_step() {  # mcu_step <name> <must-contain> <extra flags> <sources...>
    local name="$1" want="$2" extra="$3"; shift 3
    { mcu_build "$name.elf" "$extra" "$@" && timeout 60 $QEMU -kernel "$B/$name.elf"; } > "$name.out" 2>&1
    local rc=$?
    rec "$name" "startup.cc board.h loop_core.h mcu_common.h $*" "$TCA" \
        "clang++ $ARMFLAGS $extra -c startup.cc $*; ld.lld -T os305.ld --gc-sections ... -o $name.elf; timeout 60 $QEMU -kernel $name.elf" \
        "$rc (0 = the program ended the run through semihosting)" "$HWQ"
    if [ "$rc" != 0 ] || ! grep -q "$want" "$name.out"; then echo "BR-05: step $name failed" >&2; status=1; fi
}

mcu_step bm_superloop "SUMMARY world=bare-metal-superloop" "" bm_superloop.cc
mcu_step bm_irq       "SUMMARY world=bare-metal-interrupt" "" bm_irq.cc
mcu_step rtos         "SUMMARY world=rtos " "" urtos.cc rtos_app.cc
mcu_step forensic     "SUMMARY world=rtos-long-critical-section" "-DBR05_LONG_CRITICAL=1" urtos.cc rtos_app.cc

# repeatability of the emulated runs: build and run world 2 again, compare byte for byte
{ mcu_build again.elf "" urtos.cc rtos_app.cc && timeout 60 $QEMU -kernel "$B/again.elf"; } > "$B/again.txt" 2>&1
if cmp -s rtos.out "$B/again.txt"; then echo "second run of world 2: identical output ($(wc -l < rtos.out) lines)"; else echo "second run of world 2: DIFFERENT"; diff rtos.out "$B/again.txt"; fi > repeat.out
rec repeat "rtos.elf" "$TCA" "rebuild and rerun world 2, then cmp with rtos.out" 0 "$HWQ"
grep -q identical repeat.out || status=1

# trap 1: the handler that waits. EXPECTED to hang: QEMU is stopped by a 10 s time limit.
{ mcu_build trap_isr_block.elf "" trap_isr_block.cc && timeout 10 $QEMU -kernel "$B/trap_isr_block.elf"; } > trap_isr_block.out 2>&1
rc=$?
rec trap_isr_block "startup.cc board.h loop_core.h mcu_common.h bm_time.h trap_isr_block.cc" "$TCA" \
    "clang++ $ARMFLAGS -c ...; ld.lld ...; timeout 10 $QEMU -kernel trap_isr_block.elf" \
    "$rc (124 = stopped by the 10 s time limit: the firmware hung, which is the expected result of this trap)" "$HWQ"
[ "$rc" = 124 ] || { echo "BR-05: trap_isr_block was expected to hang" >&2; status=1; }
mcu_step trap_isr_fixed "finished 60 periods" "-DBR05_FIX=1" trap_isr_block.cc

# trap 4 on the microcontroller: new[] in firmware linked without any library
mcu_build trap_heap.elf "" trap_heap.cc > trap_heap.out 2>&1
rc=$?
sed -i "s#$(pwd)/##g" trap_heap.out
rec trap_heap "startup.cc board.h loop_core.h mcu_common.h bm_time.h trap_heap.cc" "$TCA" \
    "clang++ $ARMFLAGS -c startup.cc trap_heap.cc; ld.lld -T os305.ld --gc-sections startup.o trap_heap.o -o trap_heap.elf" \
    "$rc (1 = ld.lld refused to link: the failure is the expected result of this step)"
[ "$rc" != 0 ] && grep -q "undefined symbol" trap_heap.out || { echo "BR-05: trap_heap was expected to fail to link" >&2; status=1; }

# ---------------------------------------------------------------- Linux on this machine
TCH="$(g++ --version | head -n 1)"
OPT="-std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror -pthread"
SAN="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined -pthread"
MEAS="measured: on the build container (a virtual machine shared with other work; kernel without PREEMPT_RT, see kernel.out), run as root; numbers change from run to run (AH-23)"
N="$(nproc)"

# sanitizer smoke runs of the two Linux programs (correctness; their timing is not used)
g++ $SAN linux_loop.cc -o "$B/linux_loop_san" && g++ $SAN linux_inversion.cc -o "$B/linux_inversion_san" &&
{ timeout 60 "$B/linux_loop_san" --steps 200 && timeout 60 "$B/linux_inversion_san" --protocol inherit; } > smoke.out 2>&1
rc=$?
rec smoke "linux_loop.cc linux_inversion.cc loop_core.h" "$TCH" \
    "g++ $SAN linux_loop.cc -o linux_loop; g++ $SAN linux_inversion.cc -o linux_inversion; ./linux_loop --steps 200; ./linux_inversion --protocol inherit" \
    "$rc" "note:      sanitizer builds, short runs: they check memory and undefined behaviour; timing not used"
[ "$rc" = 0 ] || status=1

g++ $OPT linux_loop.cc -o "$B/linux_loop" && g++ $OPT linux_inversion.cc -o "$B/linux_inversion" || status=1
lstep() {  # lstep <name> <program> <arguments...>
    local name="$1" prog="$2"; shift 2
    timeout 120 "$B/$prog" "$@" > "$name.out" 2>&1
    local rc=$?
    rec "$name" "$prog.cc loop_core.h" "$TCH" "g++ $OPT $prog.cc -o $prog; ./$prog $*" "$rc" "$MEAS"
    [ "$rc" = 0 ] || { echo "BR-05: step $name failed" >&2; status=1; }
}
lstep linux_other_idle linux_loop --policy other
lstep linux_other_load linux_loop --policy other --load "$N"
lstep linux_fifo_load  linux_loop --policy fifo --prio 80 --mlock --load "$N"
lstep linux_alloc      linux_loop --policy fifo --prio 80 --mlock --load "$N" --alloc
lstep inversion_none    linux_inversion --protocol none
lstep inversion_inherit linux_inversion --protocol inherit

# trap 3: what kind of kernel is this? (the kernel's own build configuration)
{
    echo "uname -r: $(uname -r)"
    echo "uname -v: $(uname -v)"
    if [ -e /sys/kernel/realtime ]; then echo "/sys/kernel/realtime: $(cat /sys/kernel/realtime)"; else echo "/sys/kernel/realtime: (file does not exist)"; fi
    zcat /proc/config.gz | grep -E '^(# )?CONFIG_(PREEMPT_NONE|PREEMPT_VOLUNTARY|PREEMPT|PREEMPT_RT|PREEMPT_DYNAMIC|HZ|HIGH_RES_TIMERS)[ =]'
    echo "lines of the configuration mentioning PREEMPT_RT: $(zcat /proc/config.gz | grep -c 'PREEMPT_RT')"
} > kernel.out 2>&1
rc=$?
rec kernel "(shell commands)" "$(zcat --version | head -n 1)" \
    "uname -r; uname -v; cat /sys/kernel/realtime; zcat /proc/config.gz | grep -E '^(# )?CONFIG_(PREEMPT_NONE|PREEMPT_VOLUNTARY|PREEMPT|PREEMPT_RT|PREEMPT_DYNAMIC|HZ|HIGH_RES_TIMERS)[ =]'; grep -c PREEMPT_RT" \
    "$rc" "hardware:  PREEMPT_RT runs are untested in this build: the container's kernel cannot be replaced"
[ "$rc" = 0 ] || status=1

# ---------------------------------------------------------------- footprint and comparison
{
    echo "--- emulated microcontroller images (llvm-size, bytes) ---"
    for e in bm_superloop bm_irq rtos; do
        mcu_build "$e.fp.elf" "" $( [ $e = rtos ] && echo "urtos.cc rtos_app.cc" || echo "$e.cc" ) > /dev/null 2>&1
        llvm-size "$B/$e.fp.elf" | tail -n 1 | awk -v n="$e" '{printf "%-14s text %6d  data %5d  bss %6d  total %7d\n", n, $1, $2, $3, $4}'
    done
    echo "--- Linux program (size, bytes; dynamically linked, so the C and C++ libraries are extra) ---"
    size "$B/linux_loop" | tail -n 1 | awk '{printf "linux_loop     text %6d  data %5d  bss %6d  total %7d\n", $1, $2, $3, $4}'
    echo "shared libraries it loads: $(ldd "$B/linux_loop" | wc -l)"
    echo "--- the running Linux process (from its own /proc/self/status, see linux_other_idle.out) ---"
    grep -E '^(VmSize|VmRSS)' linux_other_idle.out
} > footprint.out 2>&1
rc=$?
rec footprint "(llvm-size, size, ldd)" "$(llvm-size --version | grep -m1 'LLVM version' | sed 's/^ *//'); $(size --version | head -n 1)" \
    "llvm-size <elf>; size linux_loop; ldd linux_loop | wc -l" "$rc"
[ "$rc" = 0 ] || status=1

python3 -I compare.py bm_superloop.out bm_irq.out rtos.out forensic.out \
    linux_other_idle.out linux_other_load.out linux_fifo_load.out linux_alloc.out > compare.out 2>&1
rc=$?
rec compare "compare.py" "$(python3 --version)" \
    "python3 -I compare.py bm_superloop.out bm_irq.out rtos.out forensic.out linux_other_idle.out linux_other_load.out linux_fifo_load.out linux_alloc.out" \
    "$rc" "note:      emulated clocks and real microseconds are different units; the table compares each world with its own period"
[ "$rc" = 0 ] || status=1

rm -rf "$B"
exit $status
