#!/usr/bin/env bash
# F3-32 run.sh: make a FAT32 image with the host's tools, read and write it with our driver,
# check the result with mtools and fsck.fat (curriculum B15), and make the forensic evidence.
set -u -o pipefail
cd "$(dirname "$0")"
. ../F3-31/lablib.sh
export MTOOLS_SKIP_CHECK=1 LC_ALL=C
status=0
MKFS_VER="$(rm -f .v.img; mkfs.fat -C .v.img 1440 2>&1 | head -n 1)"   # the version is the first line mkfs.fat prints
MT_VER="$(mtools --version | head -n 1)"
FSCK_VER="$(fsck.fat -n .v.img 2>&1 | head -n 1; rm -f .v.img)"
ok() { [ "$1" = "$2" ] || { echo "step $3: exit $1, expected $2" >&2; status=1; }; }

# 1. build the driver and its command-line front end (same flags as run_lab.sh)
hostbuild fat32tool -x c++ fat32.cc -x c++ fat32tool.cc > build.out 2>&1; rc=$?
echo "build of fat32.cc + fat32tool.cc: exit $rc (no messages means no warnings)" >> build.out
rec build "fat32.h fat32.cc fat_names.h fat32tool.cc" "$GXX_VER" "g++ $HOSTFLAGS fat32.cc fat32tool.cc -o fat32tool" "$rc"
ok "$rc" 0 build

# 2. a FAT32 volume made by the host, with files written by the host
rm -f fat.img
{ mkfs.fat -F 32 -s 1 -S 512 -n OS304FAT -i 30430430 -C fat.img 40960 2>&1
  mmd -i fat.img ::/Docs
  printf 'Hello from the host.\n' > .r.txt; mcopy -i fat.img .r.txt "::/Read me first.txt"
  seq 1 5000 > .n.txt; mcopy -i fat.img .n.txt "::/Docs/numbers.txt"
  mdir -i fat.img ::/; mdir -i fat.img ::/Docs; } > mkfs.out 2>&1; rc=$?
rec mkfs "run.sh step 2" "$MKFS_VER; $MT_VER" \
    "mkfs.fat -F 32 -s 1 -S 512 -n OS304FAT -i 30430430 -C fat.img 40960; mmd; mcopy; mdir" "$rc"
ok "$rc" 0 mkfs

# 3. our driver reads what the host wrote
{ ./fat32tool info fat.img; ./fat32tool ls fat.img /; ./fat32tool ls fat.img /Docs
  echo '== cat "/read me FIRST.txt" (names compare without case) =='; ./fat32tool cat fat.img "/read me FIRST.txt"
  echo '== cat /Docs/numbers.txt | cmp with the host file =='
  ./fat32tool cat fat.img /Docs/numbers.txt | cmp - .n.txt && echo "identical ($(stat -c %s .n.txt) bytes)"; } > read.out 2>&1; rc=$?
rec read "fat32tool info / ls / cat" "$GXX_VER" "./fat32tool info|ls|cat fat.img ..." "$rc"
ok "$rc" 0 read

# 4. our driver writes; the host reads (B15 test 1, host side with mtools)
printf 'Written by our driver.\n' > .w.txt
{ ./fat32tool put fat.img "/Written By Kernel.txt" .w.txt
  ./fat32tool put fat.img "/Docs/Second file with a quite long name.txt" .n.txt; } > write.out 2>&1; rc=$?
rec write "fat32tool put" "$GXX_VER" "./fat32tool put fat.img <path> <host file> (twice)" "$rc"
ok "$rc" 0 write
{ mdir -i fat.img ::/; mdir -i fat.img ::/Docs
  echo '== mtype "::/Written By Kernel.txt" =='; mtype -i fat.img "::/Written By Kernel.txt"
  echo '== mtype "::/Docs/Second file with a quite long name.txt" | cmp with the source file =='
  mtype -i fat.img "::/Docs/Second file with a quite long name.txt" | cmp - .n.txt && echo identical; } > hostcheck.out 2>&1; rc=$?
rec hostcheck "run.sh step 4" "$MT_VER" "mdir; mtype" "$rc"
ok "$rc" 0 hostcheck
dd if=fat.img bs=512 skip=1292 count=1 status=none | od -A d -t x1z -v | head -n 20 > dirdump.out; rc=$?
rec dirdump "run.sh step 4" "$(od --version | head -n 1)" "dd if=fat.img bs=512 skip=1292 count=1 | od -A d -t x1z -v | head -n 20" "$rc"
fsck.fat -n -v fat.img > fsck1.out 2>&1; rc=$?
rec fsck1 "run.sh step 4" "$FSCK_VER" "fsck.fat -n -v fat.img" "$rc"
ok "$rc" 0 fsck1

# 5. B15 test 2: 10,000 random create/write/delete operations, then fsck.fat
timeout 300 ./fat32tool stress fat.img 10000 304 > stress.out 2>&1; rc=$?
rec stress "fat32tool stress" "$GXX_VER" "./fat32tool stress fat.img 10000 304" "$rc"
ok "$rc" 0 stress
{ fsck.fat -n fat.img; echo "fsck.fat exit code: $?"; mdir -i fat.img ::/ | tail -n 3; } > fsck2.out 2>&1
grep -q 'fsck.fat exit code: 0' fsck2.out; rc=$?
rec fsck2 "run.sh step 5" "$FSCK_VER" "fsck.fat -n fat.img" "$rc"
ok "$rc" 0 fsck2

# 6. forensic evidence: the driver after a colleague's "simplification" of one line in
#    fat_names.h (the change is described only in the answer key)
mkdir -p .fz && cp fat32.h fat32.cc fat32tool.cc .fz/
sed 's/sum = static_cast<std::uint8_t>(((sum \& 1) ? 0x80 : 0) + (sum >> 1) + c);   \/\/ rotate right, add/sum = static_cast<std::uint8_t>((sum << 1) + c);/' \
    fat_names.h > .fz/fat_names.h
hostbuild .fz/tool -I .fz -x c++ .fz/fat32.cc -x c++ .fz/fat32tool.cc > /dev/null 2>&1; brc=$?
rm -f fb.img
mkfs.fat -F 32 -s 1 -S 512 -n OS304FAT -i 30430430 -C fb.img 40960 > /dev/null 2>&1
mcopy -i fb.img .r.txt "::/Read me first.txt"
{ ./.fz/tool put fb.img "/Quarterly Report Draft.txt" .w.txt; ./.fz/tool put fb.img "/Meeting notes.txt" .w.txt
  ./.fz/tool ls fb.img /; } > forensic_driver.out 2>&1; rc=$?
rec forensic_driver "the changed driver (see the answer key)" "$GXX_VER" "./fat32tool put ... ; ./fat32tool ls fb.img /" "$rc"
[ "$brc" = 0 ] && ok "$rc" 0 forensic_driver || status=1
mdir -i fb.img ::/ > forensic_mdir.out 2>&1; rc=$?
rec forensic_mdir "run.sh step 6" "$MT_VER" "mdir -i fb.img ::/" "$rc"
fsck.fat -n fb.img > forensic_fsck.out 2>&1; rc=$?
rec forensic_fsck "run.sh step 6" "$FSCK_VER" "fsck.fat -n fb.img" "$rc"
dd if=fb.img bs=512 skip=1292 count=1 status=none | od -A d -t x1z -v | head -n 12 > forensic_dump.out; rc=$?
rec forensic_dump "run.sh step 6" "$(od --version | head -n 1)" "dd if=fb.img bs=512 skip=1292 count=1 | od -A d -t x1z -v | head -n 12" "$rc"

rm -rf .fz fat.img fb.img fat32tool .r.txt .n.txt .w.txt
sed -i "s#$(pwd)/##g" ./*.out
exit $status
