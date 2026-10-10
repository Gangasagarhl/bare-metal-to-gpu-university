#!/usr/bin/env bash
# F3-43 run.sh: build Listing 2 and record the system calls of the safe and the unsafe
# file replacement with strace (Listing 1, crashsim.cpp, is run by run_lab.sh itself).
set -u -o pipefail
cd "$(dirname "$0")"
. ./lablib.sh
status=0
STRACE_VER="$(strace -V | head -n 1)"
hostbuild .atomic atomic_replace.cc > build.out 2>&1; rc=$?
echo "build of atomic_replace.cc: exit $rc (no messages means no warnings)" >> build.out
rec build "atomic_replace.cc" "$GXX_VER" "g++ $HOSTFLAGS atomic_replace.cc -o atomic_replace" "$rc"
[ "$rc" = 0 ] || status=1
for mode in safe unsafe; do
    rm -rf .w && mkdir .w && printf 'volume=3\nlanguage=fr\n' > .w/settings.conf
    # LeakSanitizer cannot run under ptrace, so leak checking is off for this traced run
    # (the run_lab.sh-style build still has AddressSanitizer and UBSan). Only the lines
    # that touch the work folder .w are kept; -y prints the path behind each descriptor.
    ASAN_OPTIONS=detect_leaks=0 strace -f -y -e trace=openat,write,fsync,rename,close \
        ./.atomic "$mode" .w 2>&1 | grep -E '\.w|^(safe|unsafe) replace|exited' > "strace_$mode.out"; rc=${PIPESTATUS[0]}
    sed -i -e "s#$(pwd)#.#g" -e 's/^\[pid *[0-9]*\] //' "strace_$mode.out"
    rec "strace_$mode" "atomic_replace.cc" "$GXX_VER; $STRACE_VER" \
        "ASAN_OPTIONS=detect_leaks=0 strace -f -y -e trace=openat,write,fsync,rename,close ./atomic_replace $mode .w | grep -E '\\.w|replace|exited'" "$rc"
    [ "$rc" = 0 ] || status=1
done
rm -rf .w .atomic
exit $status
