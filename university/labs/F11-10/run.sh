#!/usr/bin/env bash
# F11-10 lab: hardening. Stack protector, NX/W^X, ASLR, and a checksec-style
# read of binary hardening flags. SMEP and SMAP are kernel features demonstrated
# in OS303 (F3-29); this lab links to that evidence and marks them untested here.
set -u
cd "$(dirname "$0")" || exit 2
LAB="$(pwd)"; status=0
GCCV="$(g++ --version | head -n 1)"
export ASAN_OPTIONS=abort_on_error=0:exitcode=99:detect_leaks=0
W="${LAB}/.work"; rm -rf "$W"; mkdir -p "$W"

rec() {
    local name="$1" listing="$2" cmd="$3" code="$4"; shift 4
    { echo "listing:   $listing"; echo "toolchain: $GCCV"; echo "command:   $cmd"
      echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
      echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
      echo "exit code: $code"; for l in "$@"; do echo "$l"; done; } > "${LAB}/${name}.log"
}

# --- stack protector: overflow with and without -fstack-protector ------------
g++ -std=c++20 -O0 -fstack-protector-all -fno-pie -no-pie "${LAB}/canary.cc" -o "$W/canary_on"  2>/dev/null
g++ -std=c++20 -O0 -fno-stack-protector -fno-pie -no-pie "${LAB}/canary.cc" -o "$W/canary_off" 2>/dev/null
ATTACK="AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"   # 40 bytes into a 16-byte buffer
{
    echo "# with -fstack-protector-all:"
    echo "\$ ./canary friend"; ( cd "$W" && ./canary_on friend )
    echo "\$ ./canary <40 A's>"; ( cd "$W" && ./canary_on "$ATTACK" ); echo "(exit code: $?)"
} > "${LAB}/canary_on.out" 2>&1
rec canary_on "canary.cc (-O0 -fstack-protector-all)" "./canary friend; ./canary <40 A's>" "$(cd "$W"; ./canary_on "$ATTACK" >/dev/null 2>&1; echo $?)" \
    "note:      exit 134 = SIGABRT from __stack_chk_fail (stack smashing detected)"
{
    echo "# with -fno-stack-protector (no canary):"
    echo "\$ ./canary <40 A's>"; ( cd "$W" && ./canary_off "$ATTACK" ); echo "(exit code: $?)"
} > "${LAB}/canary_off.out" 2>&1
rec canary_off "canary.cc (-O0 -fno-stack-protector)" "./canary <40 A's>" "$(cd "$W"; ./canary_off "$ATTACK" >/dev/null 2>&1; echo $?)" \
    "note:      with no canary the corrupted return address usually ends in SIGSEGV (139) or silent corruption"

# --- NX / W^X -----------------------------------------------------------------
g++ -std=c++20 -O0 "${LAB}/nx.cc" -o "$W/nx" 2>/dev/null
( cd "$W" && ./nx ) > "${LAB}/nx.out" 2>&1; rc=$?
echo "(exit code: $rc)" >> "${LAB}/nx.out"
rec nx "nx.cc (-O0)" "./nx" "$rc" "note:      exit 139 = SIGSEGV: the processor refused to execute a data page (NX)"

# --- ASLR ---------------------------------------------------------------------
g++ -std=c++20 -O1 -fpie -pie "${LAB}/aslr.cc" -o "$W/aslr" 2>/dev/null
{
    echo "three runs of the same PIE binary; stack, heap and code addresses change:"
    ( cd "$W" && ./aslr ); ( cd "$W" && ./aslr ); ( cd "$W" && ./aslr )
} > "${LAB}/aslr.out" 2>&1
rec aslr "aslr.cc (-O1 -fpie -pie)" "./aslr (x3)" 0 \
    "note:      addresses differ only if the kernel's ASLR is enabled; they are per-run, never a fixed number"

# --- checksec: weak vs hardened binary ----------------------------------------
g++ -std=c++20 -O1 -fno-stack-protector -z execstack -fno-pie -no-pie "${LAB}/hello.cc" -o "$W/bin_weak" 2>/dev/null
g++ -std=c++20 -O1 -fstack-protector-strong -fpie -pie -Wl,-z,relro,-z,now "${LAB}/hello.cc" -o "$W/bin_hard" 2>/dev/null
g++ -std=c++20 -O1 -g -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined "${LAB}/check.cc" -o "$W/check" 2>/dev/null
( cd "$W" && ./check bin_weak bin_hard ) > "${LAB}/checksec.out" 2>&1; rc=$?
# also show readelf's own view, to confirm our reader agrees with the standard tool
{ echo; echo "# readelf -lW agrees (GNU_STACK line of each):"
  echo "bin_weak:"; ( cd "$W" && readelf -lW bin_weak | grep -E 'GNU_STACK' )
  echo "bin_hard:"; ( cd "$W" && readelf -lW bin_hard | grep -E 'GNU_STACK|GNU_RELRO' ); } >> "${LAB}/checksec.out" 2>&1
sed -i "s#${W}/##g; s#${LAB}/##g" "${LAB}/checksec.out"
rec checksec "check.cc; readelf (GNU Binutils $(readelf --version | head -n1 | grep -oE '[0-9.]+$'))" \
    "./check bin_weak bin_hard; readelf -lW" "$rc"

for f in canary_on canary_off nx aslr; do sed -i "s#${W}/##g; s#${LAB}/##g" "${LAB}/${f}.out"; done
rm -rf "$W"
exit $status
