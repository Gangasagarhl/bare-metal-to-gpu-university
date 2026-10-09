#!/usr/bin/env bash
# toolchain.sh: check that every tool the build needs exists, and print its version line.
# The versions go into docs/log for every session (curriculum runbook 19.1).
set -u
missing=0
for tool in clang++ ld.lld g++ readelf make git; do
    if command -v "$tool" > /dev/null; then
        printf '%-8s %s\n' "$tool" "$("$tool" --version 2>&1 | head -n 1)"
    else
        printf '%-8s MISSING\n' "$tool"; missing=1
    fi
done
exit $missing
