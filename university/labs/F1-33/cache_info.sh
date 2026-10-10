#!/usr/bin/env bash
# Print the cache geometry that the Linux kernel reports for CPU 0 (sysfs).
# On a virtual machine these are the values the hypervisor chose to show.
for d in /sys/devices/system/cpu/cpu0/cache/index*; do
    printf 'level %s %-12s size %-8s ways %-3s line %s B  sets %s\n' \
        "$(cat "$d/level")" "$(cat "$d/type")" "$(cat "$d/size")" \
        "$(cat "$d/ways_of_associativity")" "$(cat "$d/coherency_line_size")" \
        "$(cat "$d/number_of_sets")"
done
