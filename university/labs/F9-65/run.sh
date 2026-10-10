#!/usr/bin/env bash
# F9-65 run.sh: the forensic evidence - the same checker on the release-4 stack description.
# (Listing 1 with stack.in runs through run_lab.sh itself.)
set -u -o pipefail
cd "$(dirname "$0")"
. ../F9-67/rblib.sh
B=.build
rm -rf "$B"; mkdir -p "$B"
g++ $HOSTFLAGS stack.cpp -o $B/stack || exit 1
timeout 10 $B/stack < stack_forensic.in > forensic_stack.out 2>&1; rc=$?
rec forensic_stack "stack.cpp" "$GXX_VER" "g++ $HOSTFLAGS stack.cpp -o stack; ./stack < stack_forensic.in" "$rc" \
    "stdin:     stack_forensic.in" "note:      exit code 1 = the checker found a problem (expected for this evidence)"
diff --label stack.in --label stack_forensic.in -u stack.in stack_forensic.in > forensic_diff.out
rec forensic_diff "stack.in stack_forensic.in" "$(diff --version | head -n 1)" "diff -u stack.in stack_forensic.in" "$?" \
    "note:      exit code 1 = the files differ (expected)"
rm -rf "$B"
[ "$rc" = 1 ]
