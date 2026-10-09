#!/usr/bin/env bash
# What this machine's kernel reports about its caches (no datasheet involved).
lscpu | grep -E 'Model name|^CPU\(s\)|L1d|L1i|L2|L3'
for k in LEVEL1_DCACHE_LINESIZE LEVEL1_DCACHE_SIZE LEVEL2_CACHE_SIZE LEVEL3_CACHE_SIZE; do
    printf '%-24s %s\n' "$k" "$(getconf $k)"
done
