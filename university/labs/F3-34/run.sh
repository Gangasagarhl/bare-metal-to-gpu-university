#!/usr/bin/env bash
# F3-34 run.sh: forensic evidence. A commit changed three things in the pipe model; after it, the
# test hangs. Evidence: the commit as a diff, and the run of the changed program.
set -u -o pipefail
cd "$(dirname "$0")"
. ../F3-31/lablib.sh
status=0
mkdir -p .fz
python3 commit_change.py pipe_model.cpp .fz/pipe_model.cpp; rc=$?
( cd .fz && cp ../pipe_model.cpp pipe_model.orig.cpp && diff -u --label a/pipe_model.cpp --label b/pipe_model.cpp pipe_model.orig.cpp pipe_model.cpp ) > forensic_diff.out
rec forensic_diff "commit_change.py applied to pipe_model.cpp" "$(diff --version | head -n 1)" \
    "diff -u a/pipe_model.cpp b/pipe_model.cpp" "$rc"
[ "$rc" = 0 ] || status=1
hostbuild .fz/pm -x c++ .fz/pipe_model.cpp > .fz/build.txt 2>&1 && { timeout 60 ./.fz/pm > forensic_run.out 2>&1; rc=$?; } || rc=99
rec forensic_run "pipe_model.cpp after the commit" "$GXX_VER" "g++ $HOSTFLAGS pipe_model.cpp -o pipe_model; ./pipe_model" "$rc" \
    "note:      exit code 3 is expected: the program's watchdog found no progress for 2 s and stopped it" \
    "note:      the byte count in the WATCHDOG line differs from run to run (thread timing)"
[ "$rc" = 3 ] || status=1
# For the answer key: each of the three changes alone, three runs each.
: > forensic_isolate.out; iso_ok=0
for k in 1 2 3; do
    python3 commit_change.py pipe_model.cpp .fz/iso$k.cpp $k
    hostbuild .fz/iso$k -x c++ .fz/iso$k.cpp > /dev/null 2>&1 || iso_ok=1
    for i in 1 2 3; do
        timeout 60 ./.fz/iso$k > .fz/iso.txt 2>&1; r=$?
        echo "change $k only, run $i: exit $r, $(grep -h -m1 -E 'WATCHDOG|both workers' .fz/iso.txt | sed 's/ ([0-9]* bytes moved so far)//')" >> forensic_isolate.out
        if [ "$k" = 1 ]; then [ "$r" = 3 ] || iso_ok=1; else [ "$r" = 0 ] || iso_ok=1; fi
    done
done
rec forensic_isolate "commit_change.py with one change at a time" "$GXX_VER" \
    "python3 commit_change.py pipe_model.cpp out.cpp <k>; g++ $HOSTFLAGS out.cpp; ./out (3 runs each)" "$iso_ok" \
    "note:      exit code 0 here means: change 1 hung (watchdog, exit 3) in every run, changes 2 and 3 passed in every run"
[ "$iso_ok" = 0 ] || status=1
# For Check yourself, question 8: line 57 changed to notify_one() without the was_empty test.
sed 's|            readable_.notify_all();                    // data appeared: wake every sleeping reader|            readable_.notify_one();|' \
    pipe_model.cpp > .fz/q8.cpp
grep -q 'readable_.notify_one();' .fz/q8.cpp && hostbuild .fz/q8 -x c++ .fz/q8.cpp > /dev/null 2>&1; q8rc=$?
: > q8_notify_one.out
for i in 1 2 3; do
    [ "$q8rc" = 0 ] || break
    timeout 60 ./.fz/q8 > .fz/q8.txt 2>&1; r=$?
    echo "run $i: exit $r, $(grep -h -m1 -E 'WATCHDOG|both workers' .fz/q8.txt | sed 's/ ([0-9]* bytes moved so far)//')" >> q8_notify_one.out
done
rec q8_notify_one "pipe_model.cpp with line 57 changed to readable_.notify_one();" "$GXX_VER" \
    "sed (line 57); g++ $HOSTFLAGS q8.cpp; ./q8 (3 runs)" "$q8rc" \
    "note:      the exit code is the build's; each run's own exit code is in the output"
[ "$q8rc" = 0 ] || status=1
rm -rf .fz
exit $status
