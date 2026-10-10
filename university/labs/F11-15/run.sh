#!/usr/bin/env bash
# F11-15 run.sh: cross-check the teaching SHA-256/HMAC (selftest.out, written by run_lab.sh
# just before this script) against Python's hashlib/hmac. Writes crosscheck.out/.log.
set -u
cd "$(dirname "$0")"
python3 -I crosscheck.py > crosscheck.out 2>&1
if cmp -s selftest.out crosscheck.out; then
    rc=0; echo "identical to selftest.out: yes" >> crosscheck.out
else
    rc=1; echo "identical to selftest.out: NO" >> crosscheck.out
fi
{
    echo "listing:   crosscheck.py"
    echo "toolchain: $(python3 --version 2>&1)"
    echo "command:   python3 -I crosscheck.py; cmp selftest.out crosscheck.out"
    echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
    echo "exit code: $rc"
} > crosscheck.log
exit $rc
