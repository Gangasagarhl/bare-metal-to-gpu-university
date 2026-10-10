#!/usr/bin/env bash
# F11-01 lab: cross-check of the SHA-256 used by chain_model.h against the system's sha256sum
# (the chain.cpp self-test prints SHA-256("abc"); this step computes it independently).
set -u -o pipefail
cd "$(dirname "$0")"
{
    printf 'sha256sum of the three bytes "abc": '
    printf 'abc' | sha256sum | cut -d' ' -f1
    printf 'chain.out self-test line:            '
    grep -o '[0-9a-f]\{64\}' chain.out
} > crosscheck.out 2>&1
a=$(sed -n 1p crosscheck.out | awk '{print $NF}'); b=$(sed -n 2p crosscheck.out | awk '{print $NF}')
if [ "$a" = "$b" ]; then rc=0; echo "equal: yes" >> crosscheck.out; else rc=1; echo "equal: NO" >> crosscheck.out; fi
{
    echo "listing:   (no listing: sha256sum and grep, shown in run.sh)"
    echo "toolchain: $(sha256sum --version | head -n 1)"
    echo "command:   printf abc | sha256sum; grep the 64-digit value in chain.out"
    echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
    echo "exit code: $rc"
} > crosscheck.log
exit $rc
