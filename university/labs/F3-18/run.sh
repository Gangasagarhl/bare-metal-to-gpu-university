#!/usr/bin/env bash
# F3-18 run.sh: build the B1 kernel, check the ELF file, boot it in QEMU, boot the
# assertion-test build, and make the forensic evidence (a linker script that loses
# the constructor list).
set -u -o pipefail
cd "$(dirname "$0")"
. ./oslab.sh
status=0
SRC="boot.S cxxrt.cc serial.cc kprint.cc panic.cc b1_main.cc"

# 1. build both kernels and inspect the ELF file (acceptance test 3: no undefined symbols)
./kbuild.sh k_b1 "" $SRC && ./kbuild.sh k_b1_assert "-DB1_ASSERT_TEST" $SRC
rc=$?
{
    echo "== size k_b1.elf =="
    size k_b1.elf
    echo "== program headers (readelf -l k_b1.elf, trimmed) =="
    readelf -lW k_b1.elf | sed -n '/Program Headers/,/GNU_STACK/p'
    echo "== undefined symbols (readelf -s: entries in section UND) =="
    readelf -sW k_b1.elf | awk '$7 == "UND"' | sed 's/^ *//'
    echo "count: $(readelf -sW k_b1.elf | awk '$7 == "UND" && $8 != ""' | wc -l) named undefined symbols"
    echo "== dynamic section and needed libraries (readelf -d) =="
    readelf -d k_b1.elf
    echo "== constructor list (nm) =="
    nm -C k_b1.elf | grep -E '__init_array_(start|end)'
} > elfcheck.out 2>&1
rec elfcheck "kbuild.sh, linker.ld, boot.S and the .cc files" "$GXX_VER; $(readelf --version | head -n 1)" \
    "./kbuild.sh k_b1 \"\" $SRC; readelf -lW / -sW / -d k_b1.elf; nm -C k_b1.elf" "$rc"
[ "$rc" = 0 ] || status=1

# 2. normal boot: expect "B1 ok" from the global constructor and exit status 33 (pass)
qrun boot.out 30 k_b1.bin; rc=$?
rec boot "b1_main.cc" "$QEMU_VER" "$QEMU $QBASE -serial stdio -kernel k_b1.bin" "$rc" \
    "note:      exit code 33 = pass: the kernel wrote 0x10 to isa-debug-exit and QEMU exits with (0x10 << 1) | 1" "$HW_NOTE"
[ "$rc" = 33 ] && grep -q '^B1 ok' boot.out || status=1

# 3. assertion test build: expect the panic line and exit status 35 (fail, as designed)
qrun assert.out 30 k_b1_assert.bin; rc=$?
rec assert "b1_main.cc built with -DB1_ASSERT_TEST" "$QEMU_VER" "$QEMU $QBASE -serial stdio -kernel k_b1_assert.bin" "$rc" \
    "note:      exit code 35 is expected: panic wrote 0x11 to isa-debug-exit, (0x11 << 1) | 1 = 35" "$HW_NOTE"
[ "$rc" = 35 ] && grep -q '^PANIC at b1_main.cc' assert.out || status=1

# 4. forensic evidence: the same kernel linked with a script that collects the old-style
#    .ctors list instead of .init_array (the kind of script found in old tutorials)
sed -e 's/KEEP(\*(SORT_BY_INIT_PRIORITY(.init_array.\*)))/KEEP(*(SORT(.ctors.*)))/' \
    -e 's/KEEP(\*(.init_array))/KEEP(*(.ctors))/' linker.ld > .linker_broken.ld
sed -e 's#-T "$here/linker.ld"#-T .linker_broken.ld#' kbuild.sh > .kbuild_broken.sh
bash .kbuild_broken.sh k_broken "" $SRC > .build.txt 2>&1; brc=$?
{
    echo "== diff linker.ld linker_broken.ld =="
    diff linker.ld .linker_broken.ld
    echo "== link messages (none means the link succeeded) =="
    cat .build.txt
} > forensic_diff.out
rec forensic_diff "linker.ld with the .init_array lines replaced by .ctors lines" "$GXX_VER" \
    "sed (see run.sh) linker.ld > linker_broken.ld; kbuild.sh with linker_broken.ld" "$brc"
[ "$brc" = 0 ] || status=1
qrun forensic_boot.out 30 k_broken.bin; rc=$?
rec forensic_boot "b1_main.cc linked with linker_broken.ld" "$QEMU_VER" "$QEMU $QBASE -serial stdio -kernel k_broken.bin" "$rc" "$HW_NOTE"
[ "$rc" = 33 ] || status=1
{
    echo "== nm: the constructor-list symbols =="
    nm k_broken.elf | grep -E '__init_array_(start|end)'
    echo "== readelf -S: allocated sections =="
    readelf -SW k_broken.elf | grep -E '^ +\[ ?[0-9]+\] \.(boot|text|rodata|data|bss|init_array|ctors)'
    echo "== objdump -s -j .init_array (the list the kernel never walked) =="
    objdump -s -j .init_array k_broken.elf | tail -n +4
    echo "== addr2line of that entry =="
    addr2line -f -C -e k_broken.elf "$(objdump -s -j .init_array k_broken.elf | awk '/^ ffff/{print "0x" substr($3,7,2) substr($3,5,2) substr($3,3,2) substr($3,1,2) substr($2,7,2) substr($2,5,2) substr($2,3,2) substr($2,1,2); exit}')"
} > forensic_elf.out 2>&1
rec forensic_elf "k_broken.elf (the forensic kernel)" "$(readelf --version | head -n 1); $(nm --version | head -n 1)" \
    "nm k_broken.elf; readelf -SW k_broken.elf; objdump -s -j .init_array; addr2line -f -C -e k_broken.elf <entry>" "0"
rm -f .build.txt
rm -f k_b1.elf k_b1.bin k_b1_assert.elf k_b1_assert.bin k_broken.elf k_broken.bin .linker_broken.ld .kbuild_broken.sh
sed -i "s#$(pwd)/##g" ./*.out
exit $status
