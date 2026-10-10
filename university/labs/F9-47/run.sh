#!/usr/bin/env bash
# F9-47 lab steps (rt_probe.cpp and preempt_model.cpp are built and run by run_lab.sh itself).
#  1. the preemption-related options of the running kernel, from its own configuration file;
#  2. the toy model in its forensic configuration (task priority 40), with sanitizers.
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
        echo "machine:   $(uname -s) $(uname -m) (cloud build container), kernel $(uname -r)"
        echo "exit code: $5"
        if [ $# -ge 6 ]; then echo "$6"; fi
    } > "$1.log"
}

# 1. kernel configuration (the file the running kernel exports about how it was built)
{
    echo "uname -v: $(uname -v)"
    zcat /proc/config.gz | grep -E '^(# )?CONFIG_(PREEMPT[A-Z_]*|HZ|HZ_[0-9]+|NO_HZ[A-Z_]*|HIGH_RES_TIMERS|IRQ_FORCED_THREADING|ARCH_SUPPORTS_RT|EXPERT|CPU_ISOLATION|RCU_NOCB_CPU)[ =]'
    echo "lines mentioning PREEMPT_RT: $(zcat /proc/config.gz | grep -c 'PREEMPT_RT')"
    echo "current clock source: $(cat /sys/devices/system/clocksource/clocksource0/current_clocksource)"
} > config.out 2>&1; rc=$?
rec config "(shell commands)" "$(zcat --version | head -n 1)" \
    "uname -v; zcat /proc/config.gz | grep -E '^(# )?CONFIG_(PREEMPT...|HZ...|NO_HZ...|HIGH_RES_TIMERS|IRQ_FORCED_THREADING|ARCH_SUPPORTS_RT|EXPERT|CPU_ISOLATION|RCU_NOCB_CPU)[ =]'; grep -c PREEMPT_RT; cat /sys/devices/system/clocksource/clocksource0/current_clocksource" "$rc"
[ "$rc" = 0 ] || status=1

# 2. the model, forensic configuration
SAN="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
g++ $SAN preempt_model.cpp -o $B/pm && timeout 30 $B/pm forensic > preempt_forensic.out 2>&1; rc=$?
rec preempt_forensic preempt_model.cpp "$(g++ --version | head -n 1)" \
    "g++ $SAN preempt_model.cpp -o preempt_model; ./preempt_model forensic" "$rc" \
    "note:      a model with assumed durations, not a measurement"
[ "$rc" = 0 ] || status=1
rm -rf "$B"
exit $status
