#!/usr/bin/env bash
# F11-16 run.sh: (1) check that hmac.h is the copy cross-checked in F11-15;
# (2) run the answer-key analysis on the logs written by whosent (run just before).
set -u
cd "$(dirname "$0")"
status=0
stamp() { date -u +%Y-%m-%dT%H:%M:%SZ; }
mach="$(uname -s) $(uname -m) (cloud build container)"
if cmp -s hmac.h ../F11-15/hmac.h; then rc=0; r="identical"; else rc=1; r="DIFFERENT"; fi
echo "hmac.h compared with ../F11-15/hmac.h (cross-checked against Python there): $r" > samecrypto.out
printf 'listing:   (no listing: cmp, shown in run.sh)\ntoolchain: %s\ncommand:   cmp hmac.h ../F11-15/hmac.h\ndate:      %s\nmachine:   %s\nexit code: %s\n' \
    "$(cmp --version | head -n 1)" "$(stamp)" "$mach" "$rc" > samecrypto.log
[ "$rc" = 0 ] || status=1
python3 -I seqcheck.py rx_log.csv gcs_tx.csv > seqcheck.out 2>&1; rc=$?
printf 'listing:   seqcheck.py\ntoolchain: %s\ncommand:   python3 -I seqcheck.py rx_log.csv gcs_tx.csv\ndate:      %s\nmachine:   %s\nexit code: %s\n' \
    "$(python3 --version 2>&1)" "$(stamp)" "$mach" "$rc" > seqcheck.log
[ "$rc" = 0 ] || status=1
exit $status
