#!/usr/bin/env bash
# F3-35 run.sh: (1) minish against the host's /bin/sh on a scripted session; (2) one C test program
# built against glibc and against tlibc on two architectures; (3) the kernel shell of the F3-31
# mini kernel driven over its serial line and compared with a reference transcript (curriculum
# B18 acceptance tests, in the forms this build can run); (4) forensic evidence.
set -u -o pipefail
cd "$(dirname "$0")"
. ../F3-31/lablib.sh
status=0
here="$(pwd)"
ok() { [ "$1" = "$2" ] || { echo "step $3: exit $1, expected $2" >&2; status=1; }; }
SH_VER="/bin/sh is $(readlink -f /bin/sh) (Debian/Ubuntu dash package $(dpkg-query -W -f='${Version}' dash 2>/dev/null))"

# 1. minish versus /bin/sh: the same session in two empty folders; standard output must match
hostbuild minish -x c++ minish.cc > build.out 2>&1; rc=$?
echo "build of minish.cc: exit $rc (no messages means no warnings)" >> build.out
rec build "minish.cc minish_parse.h" "$GXX_VER" "g++ $HOSTFLAGS minish.cc -o minish" "$rc"
ok "$rc" 0 build
rm -rf .s1 .s2; mkdir .s1 .s2
( cd .s1 && "$here/minish" < "$here/session.txt" > ../shell_session.out 2> ../.m.err ); rc=$?
rec shell_session "minish.cc with session.txt on standard input" "$GXX_VER" "cd <empty folder>; ./minish < session.txt" "$rc"
ok "$rc" 0 shell_session
( cd .s2 && sh < "$here/session.txt" > ../.ref.txt 2> ../.s.err ); src=$?
{ if diff .ref.txt shell_session.out; then echo "standard output: MATCH ($(wc -l < .ref.txt) lines)"; else echo "standard output: DIFFERENT"; fi
  echo "exit status: minish $rc, sh $src"
  echo "== standard error, minish =="; cat .m.err
  echo "== standard error, sh =="; cat .s.err; } > shell_compare.out
grep -q 'MATCH' shell_compare.out; rc=$?
rec shell_compare "run.sh step 1" "$SH_VER" "cd <empty folder>; sh < session.txt; diff with minish's output" "$rc"
ok "$rc" 0 shell_compare

# 2. ctest.c against glibc (host), tlibc on Linux x86-64, tlibc on Linux AArch64 (qemu-aarch64)
TF="-std=c++20 -ffreestanding -fno-exceptions -fno-rtti -fno-stack-protector -fno-builtin -fno-pie -O2 -Wall -Wextra -Wpedantic -Werror"
CF="-std=c17 -ffreestanding -fno-builtin -fno-stack-protector -fno-pie -nostdinc -I tlibc_include -O2 -Wall -Wextra -Wpedantic -Werror"
mkdir -p .o
{ gcc -std=c17 -O2 -Wall -Wextra -Wpedantic -Werror ctest.c -o .o/ctest_glibc &&
  g++ $TF -c tlibc.cc -o .o/tlibc.o && g++ $TF -c sys_linux_x86_64.cc -o .o/sys.o &&
  gcc $CF -c ctest.c -o .o/ctest.o && gcc -nostdlib -static -no-pie -o .o/ctest_tlibc .o/ctest.o .o/tlibc.o .o/sys.o &&
  aarch64-linux-gnu-g++ $TF -c tlibc.cc -o .o/tlibc_a.o && aarch64-linux-gnu-g++ $TF -c sys_linux_aarch64.cc -o .o/sys_a.o &&
  aarch64-linux-gnu-gcc $CF -c ctest.c -o .o/ctest_a.o &&
  aarch64-linux-gnu-gcc -nostdlib -static -no-pie -o .o/ctest_a64 .o/ctest_a.o .o/tlibc_a.o .o/sys_a.o &&
  size .o/ctest_tlibc .o/ctest_a64 | sed 's#\.o/##'; } > tlibc_build.out 2>&1; rc=$?
rec tlibc_build "ctest.c tlibc.cc sys_linux_x86_64.cc sys_linux_aarch64.cc tlibc_include/*.h" \
    "$(gcc --version | head -n 1); $(aarch64-linux-gnu-gcc --version | head -n 1)" \
    "gcc ctest.c -o ctest_glibc; g++/gcc $TF / $CF -c ...; gcc -nostdlib -static -no-pie -o ctest_tlibc ...; same with aarch64-linux-gnu-*" "$rc"
ok "$rc" 0 tlibc_build
./.o/ctest_glibc > ctest_glibc.out 2>&1; rc=$?
rec ctest_glibc "ctest.c (host C library)" "$(ldd --version | head -n 1)" "./ctest_glibc" "$rc"
./.o/ctest_tlibc > ctest_tlibc.out 2>&1; rc=$?
rec ctest_tlibc "ctest.c + tlibc (Linux x86-64 system-call layer)" "$GXX_VER" "./ctest_tlibc" "$rc"
qemu-aarch64 ./.o/ctest_a64 > ctest_a64.out 2>&1; rc=$?
rec ctest_a64 "ctest.c + tlibc (Linux AArch64 system-call layer)" "$(qemu-aarch64 --version | head -n 1)" "qemu-aarch64 ./ctest_a64" "$rc" \
    "hardware:  untested on Arm hardware; user-mode emulation by QEMU"
{ for v in tlibc a64; do if cmp -s ctest_glibc.out ctest_$v.out; then echo "ctest_$v.out: identical to ctest_glibc.out"; else echo "ctest_$v.out: DIFFERENT"; fi; done; } > ctest_compare.out
! grep -q DIFFERENT ctest_compare.out; rc=$?
rec ctest_compare "run.sh step 2" "$(cmp --version | head -n 1)" "cmp ctest_glibc.out ctest_tlibc.out; cmp ctest_glibc.out ctest_a64.out" "$rc"
ok "$rc" 0 ctest_compare
{ for v in glibc tlibc; do echo "== system calls made by ctest_$v (strace; output to a file) =="
    strace -qq -o .o/st_$v.txt ./.o/ctest_$v > /dev/null; sed 's/(.*//' .o/st_$v.txt | sort | uniq -c | sort -rn; done; } > syscalls.out 2>&1; rc=$?
rec syscalls "run.sh step 2" "$(strace -V | head -n 1)" "strace -qq -o trace.txt ./ctest_glibc|./ctest_tlibc > /dev/null; count system-call names" "$rc"

# 3. the kernel shell: a scripted serial session against a reference transcript
./../F3-31/mkinitrd.sh .o/initrd.tar
kbuild .o/kshell.elf ../F3-31/boot.S ../F3-31/kbase.cc ../F3-31/vfs.cc ../F3-31/tarfs.cc ../F3-31/infofs.cc kshell.cc > .o/kb.txt 2>&1; rc=$?
rec kbuild "kshell.cc with the F3-31 kernel files" "$GXX_VER; $LD_VER" "g++ $KFLAGS -c <each file>; ld -m elf_i386 -T kernel.ld" "$rc"
ok "$rc" 0 kbuild
python3 kshell_ref.py .o/initrd.tar kshell_session.txt > kshell_ref.out; rc=$?
rec kshell_ref "kshell_ref.py" "$(python3 --version)" "python3 kshell_ref.py initrd.tar kshell_session.txt" "$rc"
python3 serial_session.py kshell_session.txt "os304> " 60 -- $QEMU $QBASE -serial stdio -kernel .o/kshell.elf -initrd .o/initrd.tar > kshell.out; rc=$?
rec kshell "kshell.cc, driven by serial_session.py" "$QEMU_VER; $(python3 --version)" \
    "python3 serial_session.py kshell_session.txt 'os304> ' 60 -- $QEMU $QBASE -serial stdio -kernel kshell.elf -initrd initrd.tar" "$rc" \
    "note:      exit code 33 = pass (the exit command wrote 0x10 to isa-debug-exit); carriage returns removed by the harness" "$HW_NOTE"
ok "$rc" 33 kshell
{ if diff kshell_ref.out kshell.out; then echo "transcript: MATCH ($(wc -l < kshell.out) lines)"; else echo "transcript: DIFFERENT"; fi; } > kshell_compare.out
grep -q MATCH kshell_compare.out; rc=$?
rec kshell_compare "run.sh step 3" "$(diff --version | head -n 1)" "diff kshell_ref.out kshell.out" "$rc"
ok "$rc" 0 kshell_compare
timeout 30 $QEMU $QBASE -serial stdio -kernel .o/kshell.elf -initrd .o/initrd.tar < kshell_session.txt > kshell_naive.out 2>&1; rc=$?
sed -i 's/\r$//' kshell_naive.out
rec kshell_naive "kshell.cc, whole session file on QEMU's standard input at once" "$QEMU_VER" \
    "$QEMU $QBASE -serial stdio -kernel kshell.elf -initrd initrd.tar < kshell_session.txt" "$rc" \
    "note:      kept as evidence for 'Common mistakes': compare its first command with kshell_ref.out" "$HW_NOTE"

# 4. forensic evidence: minish after a refactor; "cat f.txt | wc -l" never finishes
mkdir -p .fz
sed 's/        if (!last) { close(fds\[1\]); prev_read = fds\[0\]; } \/\/ or readers would never see end of file/        if (!last) { prev_read = fds[0]; }/' minish.cc > .fz/minish.cc
hostbuild .fz/minish -I . -x c++ .fz/minish.cc > .fz/b.txt 2>&1; brc=$?
rm -rf .s3; mkdir .s3; printf 'one\ntwo\nthree\n' > .s3/f.txt
( cd .s3 && { echo 'echo start'; echo 'cat f.txt | wc -l'; echo 'echo end'; sleep 4; } | timeout 6 "$here/.fz/minish" > ../.fz/session.txt 2>&1 ) &
bg=$!
sleep 2
shpid="$(pgrep -f "^$here/.fz/minish" | head -n 1)"
{ echo "== transcript so far (standard output of the shell) =="; cat .fz/session.txt
  echo "== processes: the shell and its children (ps) =="
  ps -o pid=,ppid=,stat=,comm= -p "$shpid" --ppid "$shpid" | awk -v s="$shpid" '{role=($1==s)?"shell":"child"; printf "%-6s %-5s %s\n", role, $3, $4}'
  echo "== open file descriptors 0-9 (/proc/<pid>/fd and fdinfo access mode; pipe numbers differ per run) =="
  for p in "$shpid" $(pgrep -P "$shpid"); do
      name="$(cat /proc/$p/comm 2>/dev/null)"; st="$(awk '{print $3}' /proc/$p/stat 2>/dev/null)"
      printf '%s (state %s):' "$name" "$st"
      for fd in 0 1 2 3 4 5 6 7 8 9; do t="$(readlink /proc/$p/fd/$fd 2>/dev/null)" || continue
          fl="$(awk '/^flags:/{print substr($2, length($2))}' /proc/$p/fdinfo/$fd 2>/dev/null)"
          case "$fl" in 0) m=read;; 1) m=write;; *) m=rw;; esac
          printf ' %s->%s(%s)' "$fd" "$(echo "$t" | sed 's#.*/\.fz/session.txt#session.txt#')" "$m"; done
      echo; done; } > forensic_procs.out 2>&1
wait "$bg"; rc=$?
{ echo "== final transcript =="; cat .fz/session.txt; } > forensic_end.out
rec forensic_procs "minish after the refactor (see the answer key)" "$(ps --version | head -n 1)" \
    "minish < (echo start; cat f.txt | wc -l; echo end) in the background; after 2 s: ps and /proc/<pid>/fd" "$brc" \
    "note:      pids replaced by roles; pipe numbers differ from run to run"
rec forensic_end "run.sh step 4" "$GXX_VER" "timeout 6 ./minish < (the three lines)" "$rc" \
    "note:      exit code 124 is expected: the shell never finished and was stopped by the 6 s limit"
[ "$brc" = 0 ] && [ "$rc" = 124 ] || status=1

rm -rf .s1 .s2 .s3 .o .fz .ref.txt .m.err .s.err minish
sed -i "s#$here/##g" ./*.out
exit $status
