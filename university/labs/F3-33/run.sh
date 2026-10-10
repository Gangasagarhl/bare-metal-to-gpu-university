#!/usr/bin/env bash
# F3-33 run.sh: make an ext2 image with mke2fs, read and write it with our driver, check it with
# e2fsck and debugfs (curriculum B16), and make the forensic evidence of the course
# ("fsck is angry"): a test run with a power cut, using a driver whose write order was changed.
set -u -o pipefail
cd "$(dirname "$0")"
. ../F3-31/lablib.sh
status=0
E2_VER="$(mke2fs -V 2>&1 | head -n 1)"
ok() { [ "$1" = "$2" ] || { echo "step $3: exit $1, expected $2" >&2; status=1; }; }
MKFS="mke2fs -q -F -t ext2 -b 1024 -I 128 -N 512 -L os304 -U c0ffee00-0304-4304-8304-000000000304 -E root_owner=0:0"

# 1. build
hostbuild ext2tool -x c++ ext2.cc -x c++ ext2tool.cc > build.out 2>&1; rc=$?
echo "build of ext2.cc + ext2tool.cc: exit $rc (no messages means no warnings)" >> build.out
rec build "ext2.h ext2.cc ext2tool.cc" "$GXX_VER" "g++ $HOSTFLAGS ext2.cc ext2tool.cc -o ext2tool" "$rc"
ok "$rc" 0 build

# 2. an image made on the host by mke2fs, populated from a folder (-d)
rm -rf .src e2.img; mkdir -p .src/docs
printf 'Hello from mkfs.ext2 -d.\n' > .src/hello.txt; seq 1 30000 > .src/docs/numbers.txt
{ $MKFS -d .src e2.img 16M 2>&1
  dumpe2fs e2.img 2>/dev/null | grep -E '^(Group [0-9]|  (Block|Inode) bitmap at|  Inode table at|Filesystem features|Inode size|Block size)'; } > mkfs.out; rc=$?
rec mkfs "run.sh step 2" "$E2_VER" "$MKFS -d src e2.img 16M; dumpe2fs e2.img | grep ..." "$rc"
ok "$rc" 0 mkfs

# 3. our driver reads it (B16 test 1, first half)
{ ./ext2tool info e2.img; ./ext2tool ls e2.img /; ./ext2tool ls e2.img /docs
  echo '== cat /hello.txt =='; ./ext2tool cat e2.img /hello.txt
  echo '== cat /docs/numbers.txt | cmp with the host file =='
  ./ext2tool cat e2.img /docs/numbers.txt | cmp - .src/docs/numbers.txt && echo "identical ($(stat -c %s .src/docs/numbers.txt) bytes)"; } > read.out 2>&1; rc=$?
rec read "ext2tool info / ls / cat" "$GXX_VER" "./ext2tool info|ls|cat e2.img ..." "$rc"
ok "$rc" 0 read

# 4. our driver writes; e2fsck and debugfs check (B16 test 1, second half)
{ ./ext2tool mkdir e2.img /notes; ./ext2tool put e2.img /notes/first.txt .src/hello.txt
  ./ext2tool put e2.img /big.txt .src/docs/numbers.txt; ./ext2tool rm e2.img /hello.txt; } > write.out 2>&1; rc=$?
rec write "ext2tool mkdir / put / rm" "$GXX_VER" "./ext2tool mkdir|put|rm e2.img ..." "$rc"
ok "$rc" 0 write
{ debugfs -R "ls -l /" e2.img 2>&1 | sed 's/ \{2,\}/ /g'
  echo '== debugfs cat /notes/first.txt =='; debugfs -R "cat /notes/first.txt" e2.img 2>/dev/null
  echo '== debugfs cat /big.txt | cmp with the source file =='
  debugfs -R "cat /big.txt" e2.img 2>/dev/null | cmp - .src/docs/numbers.txt && echo identical
  echo '== debugfs stat /big.txt (block map) =='; debugfs -R "stat /big.txt" e2.img 2>/dev/null | sed -n '/^BLOCKS:/,$p'; } > hostcheck.out; rc=$?
rec hostcheck "run.sh step 4" "$E2_VER" "debugfs -R 'ls -l /' | 'cat ...' | 'stat /big.txt' e2.img" "$rc"
ok "$rc" 0 hostcheck
e2fsck -fn e2.img > fsck1.out 2>&1; rc=$?
rec fsck1 "run.sh step 4" "$E2_VER" "e2fsck -fn e2.img" "$rc"
ok "$rc" 0 fsck1

# 5. B16 test 2: 10,000 random operations, then e2fsck
timeout 300 ./ext2tool stress e2.img 10000 304 > stress.out 2>&1; rc=$?
rec stress "ext2tool stress" "$GXX_VER" "./ext2tool stress e2.img 10000 304" "$rc"
ok "$rc" 0 stress
e2fsck -fn e2.img > fsck2.out 2>&1; rc=$?
rec fsck2 "run.sh step 5" "$E2_VER" "e2fsck -fn e2.img" "$rc"
ok "$rc" 0 fsck2

# 6. forensic evidence: the test harness cuts the power after 26 block writes, reboots without
#    fsck, writes a second file, then runs e2fsck. The driver used here differs from Listing 2
#    in the order of three lines (described only in the answer key).
mkdir -p .ez && cp ext2.h ext2tool.cc .ez/
python3 - ext2.cc .ez/ext2.cc <<'PY'
import sys
s = open(sys.argv[1]).read()
a = "    mark_blocks(blocks, true);                                       // 1) block bitmap and counts\n"
b = "    return add_entry(parent, name, ino, FT_FILE);                    // 4) the name\n"
c = "        write_inode(*existing, in);                                  // 3) the inode now uses the new blocks\n"
assert a in s and b in s and c in s
s = s.replace(a, "").replace(b, "    int rc = add_entry(parent, name, ino, FT_FILE);\n    mark_blocks(blocks, true);\n    return rc;\n")
s = s.replace(c, c + "        mark_blocks(blocks, true);\n")
open(sys.argv[2], "w").write(s)
PY
hostbuild .ez/tool -I .ez -x c++ .ez/ext2.cc -x c++ .ez/ext2tool.cc > /dev/null 2>&1; brc=$?
rm -f c.img; $MKFS -d .src c.img 16M > /dev/null 2>&1
./.ez/tool crash c.img 26 > forensic_run.out 2>&1; rc=$?
rec forensic_run "the team's driver (see the answer key), harness command crash" "$GXX_VER" "./ext2tool crash c.img 26" "$rc"
[ "$brc" = 0 ] && ok "$rc" 0 forensic_run || status=1
e2fsck -fn c.img > forensic_fsck.out 2>&1; rc=$?
rec forensic_fsck "run.sh step 6" "$E2_VER" "e2fsck -fn c.img" "$rc" \
    "note:      exit code 4 is expected here: e2fsck -n found errors it was not allowed to correct"
[ "$rc" = 4 ] || status=1
{ for f in /report-a.txt /report-b.txt; do echo "== debugfs stat $f =="
    debugfs -R "stat $f" c.img 2>/dev/null | grep -E '^(Inode:|Links:|Size of|BLOCKS:|\(|TOTAL)'; done
  echo '== debugfs testb 282 21 (is each block marked in the block bitmap?) =='
  debugfs -R "testb 282 21" c.img 2>/dev/null | head -n 3; } > forensic_stat.out; rc=$?
rec forensic_stat "run.sh step 6" "$E2_VER" "debugfs -R 'stat /report-a.txt' / 'stat /report-b.txt' / 'testb 282 21' c.img" "$rc"

# 7. for the answer key: the same harness with the driver of Listing 2
rm -f c.img; $MKFS -d .src c.img 16M > /dev/null 2>&1
./ext2tool crash c.img 26 > fixed_run.out 2>&1; rc=$?
rec fixed_run "ext2.cc (Listing 2), harness command crash" "$GXX_VER" "./ext2tool crash c.img 26" "$rc"
ok "$rc" 0 fixed_run
e2fsck -fn c.img > fixed_fsck.out 2>&1; rc=$?
rec fixed_fsck "run.sh step 7" "$E2_VER" "e2fsck -fn c.img" "$rc" \
    "note:      exit code 4: e2fsck -n reports the leaked blocks and inode left by the power cut (repairable)"

rm -rf .ez .src e2.img c.img ext2tool
sed -i "s#$(pwd)/##g" ./*.out
exit $status
