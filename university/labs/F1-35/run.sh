#!/usr/bin/env bash
# F1-35: the read/write bandwidth program (-O2, no sanitizers: a timing on this machine).
set -u -o pipefail
cd "$(dirname "$0")"
TC="$(g++ --version | head -n 1)"
OPT="g++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror"
$OPT writes.cc -o .bin_writes || exit 1
timeout 120 ./.bin_writes > writes.out 2>&1; rc=$?
{
    echo "listing:   writes.cc"
    echo "toolchain: $TC"
    echo "command:   $OPT writes.cc -o writes && ./writes"
    echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "machine:   $(uname -s) $(uname -m) (cloud build container, $(nproc) CPUs); CPU model: $(grep -m1 'model name' /proc/cpuinfo | cut -d: -f2 | sed 's/^ //')"
    echo "exit code: $rc"
    echo "note:      measured on the build container (a shared cloud virtual machine), one session; not a specification (AH-23)"
} > writes.log
# which library calls does the optimised program make? (evidence for the chapter's discussion)
objdump -d --no-show-raw-insn -C .bin_writes | grep -E 'call.*<(memmove|memcpy|memset)@plt>' > writes_calls.out; rc2=$?
{
    echo "listing:   writes.cc"
    echo "toolchain: $(objdump --version | head -n 1)"
    echo "command:   objdump -d --no-show-raw-insn -C writes | grep -E 'call.*<(memmove|memcpy|memset)@plt>'"
    echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
    echo "exit code: $rc2"
} > writes_calls.log
rm -f .bin_writes
[ "$rc" = 0 ] && [ "$rc2" = 0 ]
