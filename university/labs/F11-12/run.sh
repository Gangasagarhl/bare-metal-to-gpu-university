#!/usr/bin/env bash
# F11-12 lab: untrusted input from devices (curriculum runbook 5.4 "Trust" row).
# A device-descriptor parser, buggy and fixed, on crafted malformed blobs:
#   - buggy build trusts the device's lengths -> AddressSanitizer over-read;
#   - fixed build bounds every length -> the blob is rejected, no over-read.
# run_lab.sh builds and runs desc_tests.cpp (the fixed parser's unit tests) first.
set -u
cd "$(dirname "$0")" || exit 2
LAB="$(pwd)"; status=0
GCCV="$(g++ --version | head -n 1)"
SAN="-std=c++20 -O1 -g -Wall -Wextra -fsanitize=address,undefined"
export ASAN_OPTIONS=abort_on_error=0:exitcode=99:detect_leaks=0
W="${LAB}/.work"; rm -rf "$W"; mkdir -p "$W"

rec() {
    local name="$1" listing="$2" cmd="$3" code="$4"; shift 4
    { echo "listing:   $listing"; echo "toolchain: $GCCV"; echo "command:   $cmd"
      echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
      echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
      echo "exit code: $code"; for l in "$@"; do echo "$l"; done; } > "${LAB}/${name}.log"
}

# A hostile blob: a 4-byte configuration header that CLAIMS wTotalLength = 200.
printf '\x04\x02\xc8\x00' > "$W/evil.bin"                 # bLength 4, type 2, wTotalLength 200
# A well-formed blob for comparison: header(4) + two 2-byte descriptors.
printf '\x04\x02\x08\x00\x02\x05\x02\x05' > "$W/good.bin"  # wTotalLength 8
cp "$W/evil.bin" "${LAB}/evil.bin"; cp "$W/good.bin" "${LAB}/good.bin"

g++ $SAN        "${LAB}/driver.cc" "${LAB}/descparse.cc" -o "$W/driver_bug" 2>/dev/null
g++ $SAN -DFIXED "${LAB}/driver.cc" "${LAB}/descparse.cc" -o "$W/driver_fix" 2>/dev/null

# Buggy parser on the hostile blob -> over-read.
( cd "$W" && ./driver_bug evil.bin ) > "${LAB}/buggy_evil.out" 2>&1; rc=$?
sed -i "s#${W}/##g; s#${LAB}/##g" "${LAB}/buggy_evil.out"
rec buggy_evil "driver.cc + descparse.cc (buggy, -fsanitize=address)" "./driver_bug evil.bin" "$rc" \
    "note:      exit 99 = AddressSanitizer caught a heap over-read from trusting wTotalLength"

# Fixed parser on the same blob -> rejected, no over-read.
( cd "$W" && ./driver_fix evil.bin ) > "${LAB}/fixed_evil.out" 2>&1; rc=$?
sed -i "s#${W}/##g; s#${LAB}/##g" "${LAB}/fixed_evil.out"
rec fixed_evil "driver.cc + descparse.cc (FIXED, -fsanitize=address)" "./driver_fix evil.bin" "$rc"

# Fixed parser on the good blob -> accepted.
( cd "$W" && ./driver_fix good.bin ) > "${LAB}/fixed_good.out" 2>&1; rc=$?
sed -i "s#${W}/##g; s#${LAB}/##g" "${LAB}/fixed_good.out"
rec fixed_good "driver.cc + descparse.cc (FIXED)" "./driver_fix good.bin" "$rc"

rm -rf "$W"
exit $status
