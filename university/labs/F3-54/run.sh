#!/usr/bin/env bash
# F3-54 run.sh: (1) the target triple: GNU config.sub before and after teaching it "myos";
# Clang's view of the triple; (2) the driver trap: what Clang does at link time for an OS it
# does not know; (3) a myos sysroot and compiler wrapper; the first program built with them;
# (4) packages built twice and compared (reproducibility); forensic evidence and its fix.
set -u -o pipefail
cd "$(dirname "$0")"
. ../F3-50/oslib.sh
CS=/usr/share/misc/config.sub
CS_VER="$($CS --version | head -n 1)"
rm -rf .o; mkdir -p .o

# 1. triples
cp "$CS" .o/config.sub
sed -i 's/| fiwix\* )/| fiwix* | myos* )/' .o/config.sub
{ echo "== the installed config.sub =="
  for t in x86_64-linux-gnu aarch64-linux riscv64-elf x86_64-myos; do
      printf '%-18s -> ' "$t"; "$CS" "$t" 2>&1; done
  echo "== a copy with myos* added to its list of known operating systems =="
  grep -n 'myos' .o/config.sub | sed 's/^\([0-9]*\):[[:space:]]*/line \1: /'
  for t in x86_64-myos aarch64-myos riscv64-myos; do printf '%-18s -> ' "$t"; .o/config.sub "$t" 2>&1; done
} > triple_config.out 2>&1
rc=0; grep -q 'x86_64-pc-myos' triple_config.out || rc=1
rec triple_config "/usr/share/misc/config.sub and a patched copy" "$CS_VER" \
    "config.sub <triple>; sed 's/| fiwix* )/| fiwix* | myos* )/' on a copy; config.sub <triple>" "$rc"
expect "$rc" 0 triple_config
{ echo "== clang -print-target-triple =="
  for t in x86_64-linux-gnu x86_64-unknown-myos; do printf '%-22s -> ' "$t"; clang --target=$t -print-target-triple; done
  echo "== predefined macros: how many, and those that name an operating system =="
  for t in x86_64-unknown-linux-gnu x86_64-unknown-myos; do
      m=$(clang --target=$t -dM -E -x c /dev/null)
      echo "$t: $(echo "$m" | wc -l) macros; OS-related: $(echo "$m" | grep -E '__(linux|gnu_linux|unix|ELF|myos)' | awk '{print $2}' | sort | tr '\n' ' ')"
  done
} > triple_clang.out 2>&1; rc=$?
rec triple_clang "clang --target=<triple>" "$CLANG_VER" "clang --target=<t> -print-target-triple; clang --target=<t> -dM -E -x c /dev/null" "$rc"
expect "$rc" 0 triple_clang

# 2. the driver trap: linking for an unknown OS
clang --target=x86_64-unknown-myos -### hello_myos.c -o .o/trap 2>&1 | tail -n 1 | sed -E 's#"/tmp/[^"]*"#"<temporary object>"#; s#\.o/trap#hello#' > driver_trap.out; rc=$?
rec driver_trap "hello_myos.c" "$CLANG_VER" "clang --target=x86_64-unknown-myos -### hello_myos.c -o hello   (last line: the link step it would run)" "$rc" \
    "note:      -### prints the commands without running them; the temporary file name is replaced by a placeholder"
expect "$rc" 0 driver_trap

# 3. the sysroot, the wrapper, the first program
export MYOS_SYSROOT="$PWD/.o/sysroot"
C="--target=x86_64-unknown-myos -std=c17 -O2 -ffreestanding -fno-stack-protector -fno-pic -nostdinc -isystem $MYOS_SYSROOT/usr/include $WFLAGS"
{ mkdir -p "$MYOS_SYSROOT/usr/lib" && cp -r myos_libc/include "$MYOS_SYSROOT/usr/include" &&
  clang $C -c myos_libc/crt0.c -o "$MYOS_SYSROOT/usr/lib/crt0.o" &&
  clang $C -c myos_libc/libc.c -o .o/libc.o && ar rcs "$MYOS_SYSROOT/usr/lib/libc.a" .o/libc.o &&
  echo "sysroot contents:" && (cd "$MYOS_SYSROOT" && find . -type f | sort) &&
  ./x86_64-myos-cc -o .o/hello_myos hello_myos.c && echo "built hello_myos with x86_64-myos-cc" &&
  readelf -h .o/hello_myos | grep -E 'Class|OS/ABI|Type|Machine' && echo "program headers: $(readelf -l -W .o/hello_myos | grep -E '^ +[A-Z_]+ ' | awk '{print $1}' | tr '\n' ' ')"
} > sysroot.out 2>&1; rc=$?
rec sysroot "myos_libc/crt0.c myos_libc/libc.c myos_libc/include/*.h x86_64-myos-cc hello_myos.c" "$CLANG_VER; $(ld.lld --version | head -n 1); $BINUTILS_VER" \
    "clang $C -c crt0.c|libc.c; ar rcs libc.a; x86_64-myos-cc -o hello_myos hello_myos.c; readelf -h/-l" "$rc"
expect "$rc" 0 sysroot
./.o/hello_myos > hello_myos.out 2>&1; rc=$?
rec hello_myos "hello_myos.c (built for x86_64-unknown-myos)" "$CLANG_VER; $(ld.lld --version | head -n 1)" "./hello_myos   (run on the Linux build host: the port uses the Linux-compatible ABI)" "$rc"
expect "$rc" 0 hello_myos

# 4. packages, built twice; then the forensic case and its fix
pkg() {   # pkg <build folder> <tar options...>: build hello and banner into a staging tree, pack each
    local b="$1"; shift
    mkdir -p "$b/hello/usr/bin" "$b/banner/usr/bin"
    ./x86_64-myos-cc -o "$b/hello/usr/bin/hello" hello_myos.c
    ./x86_64-myos-cc -o "$b/banner/usr/bin/banner" banner.c
    (cd "$b/hello" && tar "$@" -cf ../hello-1.0.tar usr)
    (cd "$b/banner" && tar "$@" -cf ../banner-1.0.tar usr)
}
DET="--sort=name --owner=0 --group=0 --numeric-owner --mtime=@1700000000"
{ pkg .o/b1 ; sleep 2; pkg .o/b2
  echo "== plain tar, two builds two seconds apart =="
  for p in hello-1.0 banner-1.0; do a=$(sha256sum < .o/b1/$p.tar | cut -c1-16); b=$(sha256sum < .o/b2/$p.tar | cut -c1-16)
      echo "$p.tar: $a... vs $b... -> $([ "$a" = "$b" ] && echo identical || echo DIFFERENT)"; done
  rm -rf .o/b1 .o/b2; pkg .o/b1 $DET; sleep 2; pkg .o/b2 $DET
  echo "== deterministic tar options: $DET =="
  for p in hello-1.0 banner-1.0; do a=$(sha256sum < .o/b1/$p.tar | cut -c1-16); b=$(sha256sum < .o/b2/$p.tar | cut -c1-16)
      echo "$p.tar: $a... vs $b... -> $([ "$a" = "$b" ] && echo identical || echo DIFFERENT)"; done
} > repro.out 2>&1; rc=$?
rec repro "hello_myos.c banner.c x86_64-myos-cc" "$CLANG_VER; $(tar --version | head -n 1)" \
    "build both packages twice, 2 s apart: tar -cf (plain), then tar $DET -cf; sha256sum (first 16 hex digits)" "$rc"
expect "$rc" 0 repro
# forensic evidence pack: the deterministic packages from the two builds above
{ echo "== sha256sum (first 16 hex digits) of the two banner packages =="
  echo "build 1: $(sha256sum < .o/b1/banner-1.0.tar | cut -c1-16)  build 2: $(sha256sum < .o/b2/banner-1.0.tar | cut -c1-16)"
  echo "== tar -tvf of each (file list with sizes and dates) =="
  tar -tvf .o/b1/banner-1.0.tar | grep -v '/$'; tar -tvf .o/b2/banner-1.0.tar | grep -v '/$'
  echo "== cmp -l of the two banner binaries: how many bytes differ, and where =="
  cmp -l .o/b1/banner/usr/bin/banner .o/b2/banner/usr/bin/banner > .o/cmp.txt
  echo "$(wc -l < .o/cmp.txt) bytes differ; first differing offset $(head -n 1 .o/cmp.txt | awk '{print $1-1}'), last $(tail -n 1 .o/cmp.txt | awk '{print $1-1}') (0-based, decimal)"
  echo "== section headers of the banner binary (readelf -S, selected) =="
  readelf -S -W .o/b1/banner/usr/bin/banner | grep -E 'Name|\.(text|rodata)'
  echo "== printable strings of at least 8 characters that differ (strings -n 8, then diff) =="
  diff <(strings -n 8 .o/b1/banner/usr/bin/banner) <(strings -n 8 .o/b2/banner/usr/bin/banner) || true
} > forensic_repro.out 2>&1; rc=$?
rec forensic_repro "banner-1.0.tar from two builds" "$(tar --version | head -n 1); $(cmp --version | head -n 1); $BINUTILS_VER" \
    "sha256sum; tar -tvf; cmp -l (count, first and last offset); readelf -S (selected); strings -n 8 | diff" "$rc"
# the fix used in the answer key
{ rm -rf .o/b1 .o/b2; SOURCE_DATE_EPOCH=1700000000 pkg .o/b1 $DET; sleep 2; SOURCE_DATE_EPOCH=1700000000 pkg .o/b2 $DET
  for p in hello-1.0 banner-1.0; do a=$(sha256sum < .o/b1/$p.tar | cut -c1-16); b=$(sha256sum < .o/b2/$p.tar | cut -c1-16)
      echo "$p.tar: $a... vs $b... -> $([ "$a" = "$b" ] && echo identical || echo DIFFERENT)"; done
  echo "banner now prints:"; ./.o/b1/banner/usr/bin/banner
} > forensic_fixed.out 2>&1; rc=$?
rec forensic_fixed "banner.c, hello_myos.c" "$CLANG_VER; $(tar --version | head -n 1)" \
    "SOURCE_DATE_EPOCH=1700000000 for both builds, deterministic tar options; sha256sum; run banner" "$rc"
expect "$rc" 0 forensic_fixed
g++ $HOSTFLAGS ports.cpp -o .o/ports && ./.o/ports < ports_broken.in > ports_broken.out 2>&1; rc=$?
rec ports_broken "ports.cpp with ports_broken.in" "$GXX_VER" "./ports < ports_broken.in" "$rc" "note:      exit code 1 is the expected answer: the tree has errors"
expect "$rc" 1 ports_broken
rm -rf .o
exit $status
