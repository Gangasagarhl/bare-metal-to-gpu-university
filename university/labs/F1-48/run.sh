#!/usr/bin/env bash
# F1-48: what clock and timer interrupts does the build machine itself use?
# Reads the Linux clocksource files and counts the local timer interrupts
# ("LOC" in /proc/interrupts) over about one second.
set -u -o pipefail
cd "$(dirname "$0")"
rec() {  # rec <name> <command> <exit code> [extra line]
    {
        echo "listing:   (none: reads Linux kernel files)"
        echo "toolchain: Linux $(uname -r)"
        echo "command:   $2"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container), $(nproc) CPUs visible"
        echo "exit code: $3"
        if [ $# -ge 4 ]; then echo "$4"; fi
    } > "$1.log"
    [ "$3" = 0 ] || status=1
}
status=0
CS=/sys/devices/system/clocksource/clocksource0
{
    echo "current_clocksource:   $(cat $CS/current_clocksource)"
    echo "available_clocksource: $(cat $CS/available_clocksource)"
} > clocksource.out 2>&1; rc=$?
rec clocksource "cat $CS/current_clocksource $CS/available_clocksource" "$rc"
a="$(grep '^ *LOC:' /proc/interrupts)"; t0=$(date +%s.%N)
sleep 1
b="$(grep '^ *LOC:' /proc/interrupts)"; t1=$(date +%s.%N)
{
    echo "before: $a"
    echo "after:  $b"
    python3 -I -c '
import sys
a = [int(x) for x in sys.argv[1].split()[1:] if x.isdigit()]
b = [int(x) for x in sys.argv[2].split()[1:] if x.isdigit()]
dt = float(sys.argv[4]) - float(sys.argv[3])
print("interval: %.3f s" % dt)
for i, (x, y) in enumerate(zip(a, b)):
    print("CPU%d: %d local timer interrupts, %.0f per second" % (i, y - x, (y - x) / dt))
' "$a" "$b" "$t0" "$t1"
} > timer_irqs.out 2>&1; rc=$?
rec timer_irqs "grep LOC /proc/interrupts; sleep 1; grep LOC /proc/interrupts (difference computed in Python)" "$rc" \
    "note:      measured once on a shared virtual machine (AH-23); the counts change from second to second"
exit $status
