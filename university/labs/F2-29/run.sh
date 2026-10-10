#!/usr/bin/env bash
# F2-29 lab: each bug built twice, without and with a sanitizer (and once under Valgrind),
# plus the forensic "menu" evidence. Transcript-style .out files; run records in .log.
set -u
cd "$(dirname "$0")" || exit 2
LAB="$(pwd)"
T="${LAB}/.tmp"; rm -rf "$T"; mkdir -p "$T"
cp *.cc "$T"/
status=0
begin() {
    OUT="${LAB}/$1.out"; LOG="${LAB}/$1.log"; : > "$OUT"
    {
        echo "listing:   $2"
        echo "toolchain: $3"
        echo "command:   $4"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
    } > "$LOG"
}
c() {
    echo "\$ $1" >> "$OUT"
    (cd "$T" && bash -c "$1") >> "$OUT" 2>&1
    LAST=$?
}
# end <expected exit> [note] [trim]: placeholders for process ids, build ids and folders;
# with "trim", the shadow-memory map after the SUMMARY line is cut (marked with [...])
end() {
    sed -i -E "s#${T}/#./#g; s#${LAB}/#./#g; s#==[0-9]+==#==<pid>==#g; s# \(BuildId: [0-9a-f]+\)##g; s#line [0-9]+: +[0-9]+ #line <n>: <pid> #g; s#https?://[^ ]*#<link removed: this university prints no address it has not opened (AH-30)>#g" "$OUT"
    if [ "${3:-}" = trim ]; then
        sed -i '/^Shadow bytes around the buggy address:/,/^==<pid>==ABORTING/c\[… shadow-memory map trimmed; the full map is shown in overflow_asan.out …]' "$OUT"
    fi
    echo "exit code: ${LAST}" >> "$LOG"
    if [ $# -ge 2 ] && [ -n "$2" ]; then echo "note:      $2" >> "$LOG"; fi
    if [ "$LAST" != "$1" ]; then echo "result:    UNEXPECTED (expected exit code $1)" >> "$LOG"; status=1; fi
}
GXX="$(g++ --version | head -n 1)"
VG="$(valgrind --version)"
F="g++ -std=c++20 -g -O1"
FA="g++ -std=c++20 -g -O1 -fno-omit-frame-pointer"

begin overflow_plain overflow.cc "$GXX" "$F overflow.cc -o overflow && ./overflow"
c "$F overflow.cc -o overflow && ./overflow"
end 0 "no sanitizer: the out-of-bounds read goes unnoticed"
begin overflow_asan overflow.cc "$GXX" "$FA -fsanitize=address overflow.cc -o overflow_asan && ./overflow_asan"
c "$FA -fsanitize=address overflow.cc -o overflow_asan && ./overflow_asan"
end 1 "AddressSanitizer stops the program at the first error and exits with status 1"
begin overflow_valgrind overflow.cc "$GXX; $VG" "$F overflow.cc -o overflow && valgrind ./overflow"
c "$F overflow.cc -o overflow && valgrind ./overflow"
end 0 "Valgrind reports the error but, by default, keeps the program's own exit status"

begin ub_plain ub.cc "$GXX" "$F ub.cc -o ub && ./ub"
c "$F ub.cc -o ub && ./ub"
end 0 "no sanitizer: undefined behaviour, printed without complaint"
begin ub_ubsan ub.cc "$GXX" "$F -fsanitize=undefined ub.cc -o ub_ubsan && ./ub_ubsan"
c "$F -fsanitize=undefined ub.cc -o ub_ubsan && ./ub_ubsan"
end 0 "UBSan reports and, by default, continues; the exit status stays 0"
begin ub_strict ub.cc "$GXX" "$F -fsanitize=undefined -fno-sanitize-recover=all ub.cc -o ub_strict && ./ub_strict"
c "$F -fsanitize=undefined -fno-sanitize-recover=all ub.cc -o ub_strict && ./ub_strict"
end 1 "with recovery switched off, the first report stops the program with status 1"

begin leak_asan leak.cc "$GXX" "$FA -fsanitize=address leak.cc -o leak && ./leak"
c "$FA -fsanitize=address leak.cc -o leak && ./leak"
end 1 "LeakSanitizer (part of AddressSanitizer on this platform) reports at exit"

begin reserve_asan reserve.cc "$GXX" "$FA -fsanitize=address reserve.cc -o reserve && ./reserve"
c "$FA -fsanitize=address reserve.cc -o reserve && ./reserve"
end 0 "not detected: the read is inside the allocated capacity"
begin reserve_annotated reserve.cc "$GXX" "$FA -fsanitize=address -D_GLIBCXX_SANITIZE_VECTOR reserve.cc -o reserve2 && ./reserve2"
c "$FA -fsanitize=address -D_GLIBCXX_SANITIZE_VECTOR reserve.cc -o reserve2 && ./reserve2"
end 1 "with libstdc++'s vector annotations the same read is a container-overflow" trim

begin incompatible overflow.cc "$GXX" "g++ -fsanitize=address,thread overflow.cc"
c "g++ -std=c++20 -fsanitize=address,thread overflow.cc -o both"
end 1 "the compiler refuses this combination"

begin menu_quiet menu.cc "$GXX" "$F -Wall -Wextra menu.cc -o menu && ./menu"
c "$F -Wall -Wextra menu.cc -o menu && ./menu"
end 0
begin menu_busy menu.cc "$GXX" "./menu busy"
c "./menu busy; exit \$?"
end 139 "exit status 139 = 128 + 11: stopped by signal 11 (SIGSEGV)"
begin menu_asan menu.cc "$GXX" "$FA -fsanitize=address menu.cc -o menu_asan && ./menu_asan busy"
c "$FA -fsanitize=address menu.cc -o menu_asan && ./menu_asan busy"
end 1 "" trim

rm -rf "$T"
exit $status
