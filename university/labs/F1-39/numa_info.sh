#!/usr/bin/env bash
# What NUMA layout does the Linux kernel report on this machine? (sysfs)
for node in /sys/devices/system/node/node*; do
    echo "$(basename "$node"): CPUs $(cat "$node/cpulist")"
    grep -m1 MemTotal "$node/meminfo"
done
echo "number of NUMA nodes: $(ls -d /sys/devices/system/node/node* | wc -l)"
