#!/usr/bin/env bash
# F3-37 lab steps (milestone H2 in emulation): find the IRQ lines, run the interrupt-driven
# firmware with an emulated I2C temperature sensor set to a known value, capture the bus
# transactions as QEMU saw them, and produce the forensic evidence (an interrupt storm).
# The host-side driver test (i2c_sim.cpp) is built and run by run_lab.sh itself.
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
QEMU="qemu-system-arm -M mps2-an385 -nographic -serial stdio -semihosting-config enable=on,target=native"
TC="$(clang++ --version | head -n 1); $(ld.lld --version | head -n 1); $(qemu-system-arm --version | head -n 1)"
HW="hardware:  untested on hardware; QEMU's emulated mps2-an385 board and its emulated tmp105 sensor model, no real sensor, no logic analyser"
build() {  # build <app source> <elf>
    clang++ $ARMFLAGS -c startup.cc -o $B/startup.o &&
    clang++ $ARMFLAGS -c "$1" -o $B/app.o &&
    ld.lld -T os305.ld --gc-sections $B/startup.o $B/app.o -o "$B/$2"
}
SENSOR="-device tmp105,address=0x48,id=sensor"

# 1. which IRQ line does each peripheral use (measured)
{ build irqmap.cc irqmap.elf && echo x | timeout 20 $QEMU -monitor none -kernel $B/irqmap.elf; } > irqmap.out 2>&1; rc=$?
rec irqmap "startup.cc irqmap.cc board.h" "$TC" "build irqmap.cc; echo x | timeout 20 $QEMU -monitor none -kernel irqmap.elf" "$rc" "$HW"
[ "$rc" = 0 ] || status=1

# 2. the application: set the emulated sensor to 23.75 degC through the QEMU monitor, then run
#    the console commands of app.in (s = scan, r = read, c = 12-bit config, w = wait, t = ticks, q)
build app.cc app.elf > $B/build.txt 2>&1 || { cat $B/build.txt; status=1; }
run_app() {  # run_app <elf> <uart out> <monitor out> [extra qemu args...]
    local elf="$1" uart="$2" mon="$3"; shift 3
    timeout 20 $QEMU $SENSOR -monitor unix:$B/mon.sock,server=on,wait=off -S "$@" -kernel "$B/$elf" < app.in > "$uart" 2>&1 &
    local qp=$!
    sleep 0.5
    python3 hmp.py $B/mon.sock "qom-set /machine/peripheral/sensor temperature 23750" \
        "qom-get /machine/peripheral/sensor temperature" "cont" > "$mon" 2>&1
    wait $qp
}
run_app app.elf app.out monitor.out; rc=$?
rec app "startup.cc app.cc i2c_bitbang.h board.h, input app.in" "$TC" \
    "timeout 20 $QEMU $SENSOR -monitor unix:mon.sock -S -kernel app.elf < app.in; monitor: qom-set /machine/peripheral/sensor temperature 23750, cont" \
    "$rc (0 = the firmware ended the run with semihosting SYS_EXIT after the q command)" "$HW"
rec monitor "QEMU monitor transcript of the app run" "$(qemu-system-arm --version | head -n 1); Python $(python3 -c 'import sys; print(sys.version.split()[0])') (hmp.py)" \
    "python3 hmp.py mon.sock 'qom-set ...' 'qom-get ...' 'cont'" 0
grep -q "temperature = 23.75 C" app.out || status=1

# 3. the bus as the emulator saw it (QEMU's i2c trace events), same run with tracing on
run_app app.elf $B/uart2.txt $B/mon2.txt -trace 'i2c_*' -D $B/i2c.txt; rc=$?
{
    echo "--- first 12 events (the scan's probe of 0x48, then the first 'r' command) ---"
    sed -E 's/^[0-9]+@[0-9.]+://' $B/i2c.txt | head -n 12
    echo "--- totals for the whole run ---"
    echo "START events (all addresses, incl. the scan): $(grep -c 'i2c_event start' $B/i2c.txt)"
    echo "transactions answered by 0x48: $(grep -c 'i2c_event start.*addr:0x48' $B/i2c.txt)"
} > i2c_trace.out
rec i2c_trace "QEMU i2c trace of the app run" "$(qemu-system-arm --version | head -n 1)" \
    "same as step 2 plus -trace 'i2c_*' -D i2c.txt; first 12 events and totals" "$rc" "$HW"

# 4a. forensic evidence: what the clean-up commit changed
diff app.cc app_v2.cc > forensic_diff.out; rc=$?
rec forensic_diff "app.cc app_v2.cc" "$(diff --version | head -n 1)" "diff app.cc app_v2.cc" "$rc (1 = the files differ, expected)"

# 4. forensic: app_v2.cc, an interrupt storm (2 s time limit; NVIC trace on)
{ build app_v2.cc app_v2.elf && timeout 2 $QEMU $SENSOR -monitor none -trace nvic_acknowledge_irq -trace nvic_complete_irq \
    -D $B/nvic.txt -kernel $B/app_v2.elf < app.in; } > forensic_uart.out 2>&1; rc=$?
sed -i -E 's/from pid [0-9]+/from pid [pid removed]/' forensic_uart.out
rec forensic_uart "startup.cc app_v2.cc, input app.in" "$TC" "timeout 2 $QEMU $SENSOR -trace nvic_* -kernel app_v2.elf < app.in" \
    "$rc (124 = stopped by the 2 s time limit: the firmware never answered the q command)" "$HW"
[ "$rc" = 124 ] || status=1
{
    echo "--- first 8 NVIC events ---"
    sed -E 's/^[0-9]+@[0-9.]+://' $B/nvic.txt | head -n 8
    echo "--- totals in 2 s of emulation (exact counts vary from run to run) ---"
    sed -E 's/^[0-9]+@[0-9.]+://' $B/nvic.txt | grep acknowledge | sort | uniq -c | sort -rn
} > forensic_nvic.out
rec forensic_nvic "QEMU NVIC trace of the forensic run" "$(qemu-system-arm --version | head -n 1)" \
    "-trace nvic_acknowledge_irq -trace nvic_complete_irq -D nvic.txt; first lines and per-line counts" 0 "$HW"

# 5. the first version of the UART receive handler cleared its interrupt flag last: run it 40
#    times and count how often the console stops answering (a race, so the count varies)
build app_race.cc app_race.elf > $B/build_race.txt 2>&1 || { cat $B/build_race.txt; status=1; }
hung=0
for i in $(seq 1 40); do
    timeout 3 $QEMU $SENSOR -monitor none -kernel $B/app_race.elf < app.in > $B/race$i.txt 2>&1 || hung=$((hung + 1))
done
{
    echo "runs: 40, runs that never answered 'q' (stopped by the 3 s limit): $hung"
    echo "UART output of the first run that stopped (if any):"
    for i in $(seq 1 40); do
        if ! grep -q bye $B/race$i.txt; then sed -E 's/from pid [0-9]+/from pid [pid removed]/' $B/race$i.txt; break; fi
    done
} > race.out
rec race "startup.cc app_race.cc, input app.in" "$TC" "40 x: timeout 3 $QEMU $SENSOR -monitor none -kernel app_race.elf < app.in" 0 \
    "note:      a race: the number of stopped runs differs from one execution of this script to the next"

# 6. size of the application image
llvm-size -A $B/app.elf | grep -E '^(section|\.text|\.data|\.bss)' > size.out 2>&1; rc=$?
rec size app.elf "$(llvm-size --version | grep -m1 'LLVM version' | sed 's/^ *//')" "llvm-size -A app.elf (allocated sections only)" "$rc"
rm -rf "$B"
exit $status
