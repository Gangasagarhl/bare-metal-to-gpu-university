#!/usr/bin/env bash
# F3-53 run.sh: (1) the U3 test program with libthrower.so on x86-64 (with sanitizers), AArch64
# and RISC-V 64 (QEMU user mode), outputs compared byte for byte; (2) what the unwinder uses to
# find unwind tables on this host; a cross-check of phdr_walk against readelf; (3) forensic
# evidence (an exception from a dlopen'ed plugin) and the fixed build.
set -u -o pipefail
cd "$(dirname "$0")"
. ../F3-50/oslib.sh
F="-std=c++20 -O2 $WFLAGS"
rm -rf .o; mkdir -p .o/x86 .o/a64 .o/rv
# 1. U3 on three architectures
{ g++ $HOSTFLAGS -fPIC -shared libthrower.cc -o .o/x86/libthrower.so &&
  g++ $HOSTFLAGS u3_test.cc -o .o/x86/u3_test -L.o/x86 -lthrower -Wl,-rpath,'$ORIGIN' &&
  aarch64-linux-gnu-g++ $F -fPIC -shared libthrower.cc -o .o/a64/libthrower.so &&
  aarch64-linux-gnu-g++ $F u3_test.cc -o .o/a64/u3_test -L.o/a64 -lthrower -Wl,-rpath,'$ORIGIN' &&
  riscv64-linux-gnu-g++ $F -fPIC -shared libthrower.cc -o .o/rv/libthrower.so &&
  riscv64-linux-gnu-g++ $F u3_test.cc -o .o/rv/u3_test -L.o/rv -lthrower -Wl,-rpath,'$ORIGIN'; } > u3_build.out 2>&1; rc=$?
[ -s u3_build.out ] || echo "(no messages: no warnings, no errors)" > u3_build.out
rec u3_build "u3_test.cc libthrower.cc" "$GXX_VER; $A64_VER; $RV_VER" \
    "g++ $HOSTFLAGS -fPIC -shared libthrower.cc -o libthrower.so; g++ $HOSTFLAGS u3_test.cc -o u3_test -L. -lthrower -Wl,-rpath,'\$ORIGIN'; the same with aarch64-linux-gnu-g++ / riscv64-linux-gnu-g++ and $F" "$rc"
expect "$rc" 0 u3_build
./.o/x86/u3_test .o/scratch_x86 > u3_x86.out 2>&1; rc=$?
rec u3_x86 "u3_test.cc + libthrower.so (x86-64, AddressSanitizer and UBSan on)" "$GXX_VER; $LDD_VER" "./u3_test scratch" "$rc"
expect "$rc" 0 u3_x86
qemu-aarch64 -L /usr/aarch64-linux-gnu ./.o/a64/u3_test .o/scratch_a64 > u3_a64.out 2>&1; rc=$?
rec u3_a64 "u3_test.cc + libthrower.so (AArch64)" "$A64_VER; $QA64_VER" "qemu-aarch64 -L /usr/aarch64-linux-gnu ./u3_test scratch" "$rc" "$HW_EMU"
expect "$rc" 0 u3_a64
qemu-riscv64 -L /usr/riscv64-linux-gnu ./.o/rv/u3_test .o/scratch_rv > u3_rv.out 2>&1; rc=$?
rec u3_rv "u3_test.cc + libthrower.so (RISC-V 64)" "$RV_VER; $QRV_VER" "qemu-riscv64 -L /usr/riscv64-linux-gnu ./u3_test scratch" "$rc" "$HW_EMU"
expect "$rc" 0 u3_rv
{ for a in a64 rv; do if cmp -s u3_x86.out u3_$a.out; then echo "u3_$a.out: identical to u3_x86.out ($(wc -c < u3_x86.out) bytes)"; else echo "u3_$a.out: DIFFERENT"; fi; done; } > u3_compare.out
! grep -q DIFFERENT u3_compare.out; rc=$?
rec u3_compare "run.sh step 1" "$(cmp --version | head -n 1)" "cmp u3_x86.out u3_a64.out; cmp u3_x86.out u3_rv.out" "$rc"
expect "$rc" 0 u3_compare

# 2. how this host's unwinder finds unwind tables, and a cross-check of phdr_walk.cpp
{ echo "== undefined symbols of the shared unwinder libgcc_s.so.1 that look up loaded objects =="
  for l in /lib/x86_64-linux-gnu/libgcc_s.so.1 /usr/aarch64-linux-gnu/lib/libgcc_s.so.1 /usr/riscv64-linux-gnu/lib/libgcc_s.so.1; do
      echo "$l: $(nm -D "$l" | grep -E ' U (dl_iterate_phdr|_dl_find_object)' | awk '{print $2}' | tr '\n' ' ')"; done
  echo "== the same question for the static unwinder libgcc_eh.a (x86-64) =="
  nm "$(g++ -print-file-name=libgcc_eh.a)" 2>/dev/null | grep -E ' U (dl_iterate_phdr|_dl_find_object)' | sort -u | awk '{print $2}'
  echo "== <dlfcn.h>: the comment above _dl_find_object =="
  grep -B2 '^int _dl_find_object' /usr/include/dlfcn.h
} > unwinder_lookup.out 2>&1; rc=$?
rec unwinder_lookup "libgcc_s.so.1 (three architectures), libgcc_eh.a, /usr/include/dlfcn.h" "$BINUTILS_VER; $LDD_VER" \
    "nm -D <libgcc_s.so.1> | grep <lookup functions>; nm libgcc_eh.a | grep ...; grep -B2 _dl_find_object dlfcn.h" "$rc"
g++ $F phdr_walk.cpp -o .o/phdr_walk; rc=$?
expect "$rc" 0 phdr_build
{ echo "== phdr_walk.cpp built without sanitizers =="; ./.o/phdr_walk
  echo "== readelf --debug-dump=frames on the same program: FDE records counted =="
  echo "$(readelf --debug-dump=frames .o/phdr_walk | grep -c ' FDE ') FDEs in .eh_frame"
} > phdr_check.out 2>&1; rc=$?
rec phdr_check "phdr_walk.cpp" "$GXX_VER; $BINUTILS_VER" "g++ $F phdr_walk.cpp -o phdr_walk; ./phdr_walk; readelf --debug-dump=frames phdr_walk | grep -c ' FDE '" "$rc"
expect "$rc" 0 phdr_check

# 3. forensic evidence, then the fixed loader
{ g++ $F -fPIC -shared libthrower.cc -o .o/libthrower.so && g++ $F app_exc.cc port_ldso.cc -o .o/app_exc &&
  g++ $F app_exc.cc port_ldso_fixed.cc -o .o/app_fixed; } > .o/b.txt 2>&1; rc=$?
cat .o/b.txt; expect "$rc" 0 forensic_build
( cd .o && ./app_exc ) > forensic_run.out 2>&1; rc=$?
rec forensic_run "app_exc.cc port_ldso.cc libthrower.cc" "$GXX_VER; $LDD_VER" \
    "g++ $F -fPIC -shared libthrower.cc -o libthrower.so; g++ $F app_exc.cc port_ldso.cc -o app_exc; ./app_exc" "$rc" \
    "note:      exit code 134 = 128 + 6: the process was ended by signal 6 (SIGABRT)"
expect "$rc" 134 forensic_run
( cd .o && ./app_fixed ) > forensic_fixed.out 2>&1; rc=$?
rec forensic_fixed "app_exc.cc port_ldso_fixed.cc libthrower.cc" "$GXX_VER; $LDD_VER" "g++ $F app_exc.cc port_ldso_fixed.cc -o app_fixed; ./app_fixed" "$rc"
expect "$rc" 0 forensic_fixed
rm -rf .o
exit $status
