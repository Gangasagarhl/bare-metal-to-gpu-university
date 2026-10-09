#!/usr/bin/env bash
# F3-40 lab steps: uRTOS v2 with a sensor task that has a measured deadline (the course's
# practical-exam pattern), run twice in QEMU with a deterministic clock: with priority
# inheritance on (app) and off (forensic). The emulated sensor is set to 23.75 degC.
set -u -o pipefail
cd "$(dirname "$0")"
status=0
B=.build
rm -rf "$B"; mkdir -p "$B"
rec() {  # rec <name> <listing> <toolchain> <command> <exit code> [extra line]
    {
        echo "listing:   $2"
        echo "toolchain: $3"
        echo "command:   $4"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
        echo "exit code: $5"
        if [ $# -ge 6 ]; then echo "$6"; fi
    } > "$1.log"
}
ARMFLAGS="--target=thumbv7m-none-eabi -mcpu=cortex-m3 -std=c++20 -ffreestanding -fno-exceptions -fno-rtti -nostdlib -Os -g -Wall -Wextra -Wpedantic -Werror"
QEMU="qemu-system-arm -M mps2-an385 -nographic -serial stdio -semihosting-config enable=on,target=native -icount shift=0,sleep=off -device tmp105,address=0x48,id=sensor"
TC="$(clang++ --version | head -n 1); $(ld.lld --version | head -n 1); $(qemu-system-arm --version | head -n 1)"
HW="hardware:  untested on hardware; QEMU mps2-an385 with -icount shift=0,sleep=off and its emulated tmp105 sensor; timing is the emulator's virtual time, not a real chip's"
build() {  # build <inheritance 0|1> <elf>
    clang++ $ARMFLAGS -c startup.cc -o $B/startup.o &&
    clang++ $ARMFLAGS -c urtos.cc -o $B/urtos.o &&
    clang++ $ARMFLAGS -DOS305_INHERITANCE=$1 -c app.cc -o $B/app.o &&
    ld.lld -T os305.ld --gc-sections $B/startup.o $B/urtos.o $B/app.o -o "$B/$2"
}
run() {  # run <elf> <out>: start halted, set the sensor through the monitor, continue
    timeout 30 $QEMU -monitor unix:$B/mon.sock,server=on,wait=off -S -kernel "$B/$1" > "$2" 2>&1 < /dev/null &
    local qp=$!
    sleep 0.5
    python3 hmp.py $B/mon.sock "qom-set /machine/peripheral/sensor temperature 23750" "cont" > $B/mon.txt 2>&1
    wait $qp
}

# 1. priority inheritance ON
build 1 app.elf > $B/b1.txt 2>&1 || { cat $B/b1.txt; status=1; }
run app.elf app.out; rc=$?
rec app "startup.cc urtos.h urtos.cc app.cc i2c_bitbang.h board.h" "$TC" \
    "clang++ $ARMFLAGS -DOS305_INHERITANCE=1 ...; timeout 30 $QEMU -S -kernel app.elf; monitor: qom-set /machine/peripheral/sensor temperature 23750, cont" \
    "$rc" "$HW"
grep -q "misses 0" app.out || status=1

# 2. forensic: the same application with priority inheritance OFF
build 0 forensic.elf > $B/b0.txt 2>&1 || { cat $B/b0.txt; status=1; }
run forensic.elf forensic.out; rc=$?
rec forensic "startup.cc urtos.cc app.cc (-DOS305_INHERITANCE=0)" "$TC" \
    "same as step 1 with -DOS305_INHERITANCE=0" "$rc" "$HW"
grep -q "priority inheritance OFF" forensic.out || status=1

# 3. size: what the two kernel objects added (compare with F3-39's size.out)
{
    llvm-size -A $B/app.elf | grep -E '^(section|\.text|\.data|\.bss)'
    echo "--- kernel object code (llvm-nm -S -C, Mutex and Queue members) ---"
    llvm-nm -S -C $B/app.elf | grep -E 'os::(Mutex|Queue)::'
} > size.out 2>&1; rc=$?
rec size app.elf "$(llvm-size --version | grep -m1 'LLVM version' | sed 's/^ *//')" "llvm-size -A app.elf; llvm-nm -S -C app.elf | grep -E 'os::(Mutex|Queue)::'" "$rc"
rm -rf "$B"
exit $status
