#!/usr/bin/env bash
# F3-52 run.sh: shared libraries, interposition, TLS, dlopen (U2 acceptance tests in the form the
# build container can run, with the host's glibc dynamic linker as the reference), the code the
# compiler emits for TLS, and the forensic evidence (a symbol interposed by surprise).
set -u -o pipefail
cd "$(dirname "$0")"
. ../F3-50/oslib.sh
F="-std=c++20 -O2 $WFLAGS"
rm -rf .o; mkdir -p .o
# 1. the library chain and interposition
{ g++ $F -fPIC -shared liba.cc -o .o/liba.so &&
  g++ $F -fPIC -shared libb.cc -o .o/libb.so -L.o -la -Wl,-rpath,'$ORIGIN' &&
  g++ $F main_ab.cc -o .o/main_ab -L.o -lb -la -Wl,-rpath,'$ORIGIN'; } > .o/b.txt 2>&1; rc=$?
cat .o/b.txt; expect "$rc" 0 chain_build
./.o/main_ab > chain.out 2>&1; rc=$?
rec chain "liba.cc libb.cc main_ab.cc" "$GXX_VER; $LDD_VER (dynamic linker)" \
    "g++ $F -fPIC -shared liba.cc -o liba.so; g++ $F -fPIC -shared libb.cc -o libb.so -L. -la -Wl,-rpath,'\$ORIGIN'; g++ $F main_ab.cc -o main_ab -L. -lb -la -Wl,-rpath,'\$ORIGIN'; ./main_ab" "$rc"
expect "$rc" 0 chain
{ echo "== readelf -l main_ab (the interpreter request) =="
  readelf -l .o/main_ab | grep -A2 INTERP | sed 's/^ *//'
  echo "== readelf -d main_ab (dynamic section, selected tags) =="
  readelf -d .o/main_ab | grep -E 'NEEDED|RUNPATH|FLAGS'
  echo "== readelf -d libb.so =="
  readelf -d .o/libb.so | grep -E 'NEEDED|RUNPATH|SONAME'
  echo "== readelf --dyn-syms main_ab: is who() exported by the executable? =="
  readelf --dyn-syms -W .o/main_ab | grep -E ' who$| Num:'
} > chain_elf.out 2>&1; rc=$?
rec chain_elf "main_ab, libb.so" "$BINUTILS_VER" "readelf -l / -d / --dyn-syms (selected lines)" "$rc"
LD_DEBUG=bindings ./.o/main_ab 2>&1 >/dev/null | grep -E "symbol .(who|answer|base_value)'" | sed -E 's/^ *[0-9]+: *//; s#[^ ]*/\.o/#./#g' > chain_bindings.out; rc=$?
rec chain_bindings "main_ab" "$LDD_VER" "LD_DEBUG=bindings ./main_ab 2>&1 >/dev/null | grep <who, answer, base_value>" "$rc" \
    "note:      LD_DEBUG is a debugging aid of this C library's dynamic linker; process ids and folder names removed"
expect "$rc" 0 chain_bindings

# 2. TLS and dlopen (no sanitizers: their allocator would hide the C library's heap statistics)
{ g++ $F -fPIC -shared plugin.cc -o .o/plugin.so && g++ $F main_tls.cc -o .o/main_tls; } > .o/b.txt 2>&1; rc=$?
cat .o/b.txt; expect "$rc" 0 tls_build
( cd .o && ./main_tls ) > tls_dlopen.out 2>&1; rc=$?
rec tls_dlopen "plugin.cc main_tls.cc" "$GXX_VER; $LDD_VER" \
    "g++ $F -fPIC -shared plugin.cc -o plugin.so; g++ $F main_tls.cc -o main_tls; ./main_tls" "$rc"
expect "$rc" 0 tls_dlopen

# 3. what the compiler emits for a thread-local variable: in a shared object and in the program
{ echo "== plugin.so, function plugin_get (built with -fPIC -shared) =="
  objdump -d --no-show-raw-insn -C .o/plugin.so | awk '/<plugin_get>:/,/^$/' | sed '/^$/d'
  echo "== main_tls, every access to exe_slot (an offset from the thread pointer in %fs) =="
  objdump -d --no-show-raw-insn -C .o/main_tls | grep -E '%fs:0xf{8}' 
  echo "== TLS program headers =="
  echo "plugin.so: $(readelf -l -W .o/plugin.so | grep -E '^ *TLS' | awk '{print $1, "memsz", $6}')"
  echo "main_tls : $(readelf -l -W .o/main_tls | grep -E '^ *TLS' | awk '{print $1, "memsz", $6}')"
} > tls_code.out 2>&1; rc=$?
rec tls_code "plugin.so, main_tls" "$BINUTILS_VER; $GXX_VER" "objdump -d --no-show-raw-insn; readelf -l (selected lines)" "$rc"
expect "$rc" 0 tls_code

# 4. forensic evidence: the application and libstats.so, as built by the ports tree
{ g++ $F -fPIC -shared libstats.cc -o .o/libstats.so && g++ $F app_stats.cc -o .o/app_stats -L.o -lstats -Wl,-rpath,'$ORIGIN'; } > .o/b.txt 2>&1; rc=$?
cat .o/b.txt; expect "$rc" 0 forensic_build
./.o/app_stats > forensic_run.out 2>&1; rc=$?
rec forensic_run "libstats.cc app_stats.cc" "$GXX_VER; $LDD_VER" \
    "g++ $F -fPIC -shared libstats.cc -o libstats.so; g++ $F app_stats.cc -o app_stats -L. -lstats -Wl,-rpath,'\$ORIGIN'; ./app_stats" "$rc"
{ echo "== nm -D --defined-only libstats.so =="; nm -D --defined-only .o/libstats.so | awk '{print $2, $3}'
  echo "== nm -D --defined-only app_stats (selected) =="; nm -D --defined-only .o/app_stats | awk '{print $2, $3}' | grep -v -E '^[A-Z] _'
} > forensic_syms.out 2>&1; rc=$?
rec forensic_syms "libstats.so, app_stats" "$BINUTILS_VER" "nm -D --defined-only <file>" "$rc"
# the fix used in the answer key: hidden visibility by default, the API exported explicitly
{ g++ $F -fPIC -shared -fvisibility=hidden libstats.cc -o .o/libstats.so && g++ $F app_stats.cc -o .o/app_stats -L.o -lstats -Wl,-rpath,'$ORIGIN'; } > .o/b.txt 2>&1; rc=$?
cat .o/b.txt; expect "$rc" 0 fixed_build
{ ./.o/app_stats; echo "== nm -D --defined-only libstats.so =="; nm -D --defined-only .o/libstats.so | awk '{print $2, $3}'; } > forensic_fixed.out 2>&1; rc=$?
rec forensic_fixed "libstats.cc (rebuilt with -fvisibility=hidden) app_stats.cc" "$GXX_VER; $LDD_VER" \
    "g++ $F -fPIC -shared -fvisibility=hidden libstats.cc -o libstats.so; relink app_stats; ./app_stats; nm -D --defined-only libstats.so" "$rc"
expect "$rc" 0 forensic_fixed
rm -rf .o
exit $status
