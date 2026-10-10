#!/usr/bin/env bash
# F3-55 run.sh: (1) the bootstrap comparison idea in miniature with minicc.cpp; (2) what a
# compiler run asks of the operating system (strace of g++ compiling the U3 test program);
# (3) build/host/target as GCC records them; (4) forensic evidence: a compile on a kernel with
# one system call missing (simulated with strace's fault injection).
set -u -o pipefail
cd "$(dirname "$0")"
. ../F3-50/oslib.sh
F="-std=c++20 -O2 $WFLAGS"
rm -rf .o; mkdir -p .o

# 1. stage comparison in miniature
{ g++ $F minicc.cpp -o .o/minicc_A && clang++ $F minicc.cpp -o .o/minicc_B && g++ $F minicc.cpp -o .o/minicc_A2; } > .o/b.txt 2>&1; rc=$?
cat .o/b.txt; expect "$rc" 0 stage_build
{ echo "== the compiler binaries =="
  echo "minicc_A (built by g++)    : $(stat -c %s .o/minicc_A) bytes, sha256 $(sha256sum < .o/minicc_A | cut -c1-16)..."
  echo "minicc_A2 (g++ again)      : $(stat -c %s .o/minicc_A2) bytes, sha256 $(sha256sum < .o/minicc_A2 | cut -c1-16)..."
  echo "minicc_B (built by clang++): $(stat -c %s .o/minicc_B) bytes, sha256 $(sha256sum < .o/minicc_B | cut -c1-16)..."
  cmp -s .o/minicc_A .o/minicc_A2 && echo "A and A2: identical (the same compiler, same inputs, same output)" || echo "A and A2: DIFFERENT"
  cmp -s .o/minicc_A .o/minicc_B && echo "A and B: identical" || echo "A and B: different binaries (two different compilers built them)"
  echo "== what they produce from the same input (minicc.in) =="
  ./.o/minicc_A < minicc.in > .o/out_A.s && ./.o/minicc_B < minicc.in > .o/out_B.s
  cmp -s .o/out_A.s .o/out_B.s && echo "out_A.s and out_B.s: identical ($(wc -l < .o/out_A.s) lines): the comparison passes" || echo "out_A.s and out_B.s: DIFFERENT: the comparison fails"
  echo "== the compiled program, assembled and linked with no C library, then run =="
  as .o/out_A.s -o .o/prog.o && ld .o/prog.o -o .o/prog && ./.o/prog
} > stage_compare.out 2>&1; rc=$?
rec stage_compare "minicc.cpp, minicc.in" "$GXX_VER; $CLANG_VER; $BINUTILS_VER" \
    "g++ $F minicc.cpp -o minicc_A (twice); clang++ $F minicc.cpp -o minicc_B; cmp; minicc_X < minicc.in > out_X.s; cmp; as; ld; ./prog" "$rc"
expect "$rc" 0 stage_compare

# 2. what one compile asks of the operating system
strace -f -qq -o .o/trace.txt g++ -std=c++20 -O2 -c ../F3-53/u3_test.cc -o .o/u3.o; rc=$?
expect "$rc" 0 compile_trace_run
{ echo "== processes: programs started by the compiler driver (successful execve calls) =="
  grep -E 'execve\(' .o/trace.txt | grep -v ' = -1' | sed -E 's/^[0-9]+ +execve\("([^"]+)".*/\1/' | sed -E 's#.*/##' | uniq -c
  echo "== system calls: total and the 20 most frequent names =="
  echo "total system calls traced: $(grep -c -E '^[0-9]+ +[a-z_0-9]+\(' .o/trace.txt)"
  sed -E 's/^[0-9]+ +([a-z_0-9]+)\(.*/\1/; t; d' .o/trace.txt | sort | uniq -c | sort -rn | head -n 20 | awk '{printf "%8d %s\n", $1, $2}'
  echo "== distinct system-call names used: $(sed -E 's/^[0-9]+ +([a-z_0-9]+)\(.*/\1/; t; d' .o/trace.txt | sort -u | wc -l) =="
  sed -E 's/^[0-9]+ +([a-z_0-9]+)\(.*/\1/; t; d' .o/trace.txt | sort -u | tr '\n' ' ' | fold -s -w 96; echo
  echo "== files: successful openat calls $(grep -E 'openat\(' .o/trace.txt | grep -c -v ' = -1'), failed $(grep -E 'openat\(' .o/trace.txt | grep -c ' = -1') =="
} > compile_trace.out 2>&1; rc=$?
rec compile_trace "../F3-53/u3_test.cc compiled by g++" "$STRACE_VER; $GXX_VER" \
    "strace -f -qq -o trace.txt g++ -std=c++20 -O2 -c u3_test.cc -o u3.o; summaries with grep/sed/sort" "$rc" \
    "note:      a resumed call that strace split over two lines is counted once (only lines starting with a call are counted)"
expect "$rc" 0 compile_trace

# 3. build, host and target, as each installed GCC records them
{ for c in gcc aarch64-linux-gnu-gcc riscv64-linux-gnu-gcc; do
      echo "$c: $($c -v 2>&1 | grep -o -E -- '--(build|host|target)=[^ ]+' | tr '\n' ' ')"; done; } > triples.out 2>&1; rc=$?
rec triples "gcc -v of three installed compilers" "$GXX_VER; $A64_VER; $RV_VER" "<compiler> -v 2>&1 | grep -o -- '--(build|host|target)=...'" "$rc"
expect "$rc" 0 triples

# 4. forensic evidence: the same compile, with lseek answering ENOSYS (as on a kernel without it)
strace -f -qq -o .o/ftrace.txt -e inject=lseek:error=ENOSYS g++ -std=c++20 -O2 -c ../F3-53/u3_test.cc -o .o/u3f.o > .o/fc.txt 2>&1; rc=$?
sed -E 's#/tmp/cc[A-Za-z0-9]+\.s#/tmp/cc<random>.s#g; s#\.o/u3f\.o#u3_test.o#g' .o/fc.txt > forensic_compile.out
rec forensic_compile "../F3-53/u3_test.cc" "$GXX_VER; $STRACE_VER" \
    "g++ -std=c++20 -O2 -c u3_test.cc -o u3_test.o   (on the simulated kernel; standard error shown)" "$rc" \
    "note:      the random part of the temporary file name is replaced by <random>" \
    "note:      the run was made under strace -f -e inject=lseek:error=ENOSYS; the injection is the cause the learner must find" \
    "note:      exit code 1 is the expected answer: the compile fails on the simulated kernel"
expect "$rc" 1 forensic_compile
{ echo "== programs run, in order, from successful execve lines =="
  grep -E 'execve\(' .o/ftrace.txt | grep -v ' = -1' | sed -E 's/^([0-9]+) +execve\("([^"]+)".*/\1 \2/' | sed -E 's# .*/# #' | awk '{print "program " NR ": " $2}'
  echo "== failed system calls per program (name and error), counted =="
  awk '/execve\(/ && !/= -1/ {match($0, /execve\("[^"]+"/); p=substr($0, RSTART+8, RLENGTH-9); sub(/.*\//, "", p); prog[$1]=p}
       / = -1 E[A-Z]+/ {if (match($0, /<\.\.\. [a-z_0-9]+ resumed>/)) a[2]=substr($0, RSTART+5, RLENGTH-14);
       else {match($0, /^[0-9]+ +[a-z_0-9]+/); split(substr($0, RSTART, RLENGTH), a, " ")}; match($0, / = -1 E[A-Z]+/);
       e=substr($0, RSTART+6, RLENGTH-6); who=($1 in prog) ? prog[$1] : "(a child, before its exec)"; key=who "  " a[2] "  " e; n[key]++}
       END {for (k in n) printf "%6d  %s\n", n[k], k}' .o/ftrace.txt | sort -t'|' -k1,1 | sort -s -k2
} > forensic_trace.out 2>&1; rc=$?
rec forensic_trace "strace output of the failing compile" "$STRACE_VER" "grep/awk over the trace: programs run; failed calls per program with their error" "$rc"
rm -rf .o
exit $status
