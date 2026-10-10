#!/usr/bin/env bash
# F2-30 lab: the red step of test-driven development, filters, line coverage with gcov,
# a probe that UBSan catches, and the forensic order-dependent tests.
set -u
cd "$(dirname "$0")" || exit 2
LAB="$(pwd)"
T="${LAB}/.tmp"; rm -rf "$T"; mkdir -p "$T"
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
    (cd "$HERE" && bash -c "$1") >> "$OUT" 2>&1
    LAST=$?
}
end() {
    sed -i -E "s#${T}/[a-z0-9_]+/#./#g; s#${T}/#./#g; s#${LAB}/#./#g" "$OUT"
    echo "exit code: ${LAST}" >> "$LOG"
    if [ $# -ge 2 ]; then echo "note:      $2" >> "$LOG"; fi
    if [ "$LAST" != "$1" ]; then echo "result:    UNEXPECTED (expected exit code $1)" >> "$LOG"; status=1; fi
}
GXX="$(g++ --version | head -n 1)"
F="g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"

HERE="$LAB"
begin same_harness "uni_test.h" "$(cmp --version | head -n 1)" "cmp uni_test.h ../F2-26/template/tests/uni_test.h"
c "cmp uni_test.h ../F2-26/template/tests/uni_test.h && echo identical"
end 0 "the harness in this lab is byte-for-byte the one in the university template"

mkdir -p "$T/red"; cp test_duration.cpp uni_test.h "$T/red/"; cp duration_v1.h "$T/red/duration.h"
HERE="$T/red"
begin red "test_duration.cpp with duration_v1.h" "$GXX" "$F test_duration.cpp -o test_duration && ./test_duration"
c "$F test_duration.cpp -o test_duration && ./test_duration"
end 1 "the red step: the first version fails the combined-unit tests"

HERE="$T"; cp test_duration.cpp duration.h uni_test.h probe.cc "$T/"
begin filter "test_duration.cpp" "$GXX" "./test_duration invalid"
c "$F test_duration.cpp -o test_duration && ./test_duration invalid"
end 0
begin coverage "test_duration.cpp, duration.h" "$GXX; $(gcov --version | head -n 1)" "g++ --coverage -O0; ./test_duration; gcov"
c "g++ -std=c++20 -O0 --coverage test_duration.cpp -o cov && ./cov > /dev/null && gcov -o . cov-test_duration.gcda 2>/dev/null | grep -A1 duration.h"
c "grep -E '#####' duration.h.gcov || echo 'no line of duration.h was left unexecuted'"
end 0
begin probe "probe.cc, duration.h" "$GXX" "$F probe.cc -o probe && ./probe"
c "$F probe.cc -o probe && ./probe"
end 0 "UBSan reports a signed overflow inside parse_duration and the program continues"

mkdir -p "$T/f"; cp order.cc uni_test.h "$T/f/"
HERE="$T/f"
begin order_all "order.cc" "$GXX" "$F order.cc -o order && ./order"
c "$F order.cc -o order && ./order"
end 1 "the full run fails"
begin order_alone "order.cc" "$GXX" "./order price_with_default_vat"
c "./order price_with_default_vat"
end 0 "the same test run alone passes"
rm -rf "$T"
exit $status
