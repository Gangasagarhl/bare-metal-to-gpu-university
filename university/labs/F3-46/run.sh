#!/usr/bin/env bash
# F3-46 run.sh: milestone FS3 in the build container. A default ext4 image filled from a
# host tree; the reader's listing must match the host's; feature refusal; checksum
# detection; and the forensic lab's broken build.
set -u -o pipefail
cd "$(dirname "$0")"
. ../F3-43/lablib.sh
status=0
PY_VER="$(python3 --version)"
MKFS_VER="$(mkfs.ext4 -V 2>&1 | head -n 1)"
DBG_VER="$(debugfs -V 2>&1 | head -n 1)"
ok() { [ "$1" = "$2" ] || { echo "step $3: exit $1, expected $2" >&2; status=1; }; }
rm -rf .w && mkdir -p .w
U=0f5f0f5f-0401-4401-8401-000000000046

# 1. build the reader, and the forensic lab's variant
{ hostbuild .w/ext4read ext4read.cc; echo "ext4read.cc: exit $?"
  hostbuild .w/ext4read_bug -DFORENSIC_NO_HOLES ext4read.cc; echo "ext4read.cc -DFORENSIC_NO_HOLES: exit $?"; } > build.out 2>&1
grep -q 'exit [1-9]' build.out; rc=$((1 - $?))
rec build "ext4read.cc" "$GXX_VER" "g++ $HOSTFLAGS ext4read.cc -o ext4read (and again with -DFORENSIC_NO_HOLES)" "$rc"
ok "$rc" 0 build

# 2. the host tree, an ext4 image made from it with default options, then directory indexing
python3 -I mktree.py .w/tree
python3 -I hostlist.py .w/tree > .w/host.txt
{ mkfs.ext4 -q -F -U $U -E hash_seed=$U -d .w/tree .w/e4.img 300M 2>&1 | grep -v '^Creating'
  e2fsck -fyD .w/e4.img > /dev/null 2>&1; echo "e2fsck -fyD (index directories): exit $?"
  dumpe2fs -h .w/e4.img 2>/dev/null | grep -E '^(Filesystem features|Filesystem flags|Block size|Inode size|Group descriptor size|Default directory hash|Checksum type|Checksum:)'
  debugfs -R "stat /many" .w/e4.img 2>/dev/null | head -n 1; } > mkfs.out; rc=$?
rec mkfs "mktree.py; run.sh step 2" "$MKFS_VER; $PY_VER" \
    "python3 -I mktree.py tree; mkfs.ext4 -q -F -U <fixed> -E hash_seed=<fixed> -d tree e4.img 300M; e2fsck -fyD e4.img" "$rc"
ok "$rc" 0 mkfs

# 3. the reader's view of the superblock and group descriptors
.w/ext4read info .w/e4.img > info.out 2>&1; rc=$?
rec info "ext4read.cc info" "$GXX_VER" "./ext4read info e4.img" "$rc"
ok "$rc" 0 info

# 4. FS3 acceptance 1: every file of the host tree reads back with matching checksums
.w/ext4read tree .w/e4.img > .w/ours.txt 2>&1; rc=$?
{ grep -v '^checksums' .w/ours.txt | sort -k2,2 > .w/a; sort -k2,2 .w/host.txt > .w/b
  echo "entries listed by ext4read: $(wc -l < .w/a); by the host (hostlist.py): $(wc -l < .w/b)"
  if diff .w/a .w/b > .w/d; then echo "listings identical (type, path, size, CRC-32 of every file)"; else cat .w/d; fi
  head -n 8 .w/a; echo "[...]"; grep '^checksums' .w/ours.txt; } > tree.out
diff -q .w/a .w/b > /dev/null; d=$?
rec tree "ext4read.cc tree; hostlist.py" "$GXX_VER; $PY_VER" "./ext4read tree e4.img | sort; python3 -I hostlist.py tree | sort; diff" "$((rc + d))"
ok "$((rc + d))" 0 tree

# 5. the extent tree of the sparse file, and e2fsprogs' view of the same inode
{ .w/ext4read extents .w/e4.img /big/sparse.bin; echo "== debugfs stat /big/sparse.bin (EXTENTS)"
  debugfs -R "stat /big/sparse.bin" .w/e4.img 2>/dev/null | sed -n '/^EXTENTS/,$p'; } > extents.out 2>&1; rc=$?
rec extents "ext4read.cc extents" "$GXX_VER; $DBG_VER" "./ext4read extents e4.img /big/sparse.bin; debugfs -R 'stat /big/sparse.bin'" "$rc"
ok "$rc" 0 extents

# 6. hashed-tree lookup, and the hash as e2fsprogs computes it
{ .w/ext4read lookup .w/e4.img /many file1999.txt; .w/ext4read lookup .w/e4.img /many file0042.txt
  for n in file1999.txt file0042.txt; do debugfs -R "dx_hash -h half_md4 -s $U $n" .w/e4.img 2>/dev/null; done; } > lookup.out 2>&1; rc=$?
rec lookup "ext4read.cc lookup" "$GXX_VER; $DBG_VER" "./ext4read lookup e4.img /many <name>; debugfs -R 'dx_hash -h half_md4 -s <seed> <name>'" "$rc"
ok "$rc" 0 lookup

# 7. FS3 acceptance 2a: an unsupported incompatible feature is refused by name
mkfs.ext4 -q -F -O inline_data .w/inl.img 16M > /dev/null 2>&1
{ dumpe2fs -h .w/inl.img 2>/dev/null | grep '^Filesystem features'; .w/ext4read info .w/inl.img; echo "exit code $?"; } > refuse.out 2>&1
grep -q "exit code 1" refuse.out; rc=$?
rec refuse "ext4read.cc info" "$GXX_VER; $MKFS_VER" "mkfs.ext4 -q -F -O inline_data inl.img 16M; ./ext4read info inl.img" "$rc" \
    "result:    ext4read exit code 1 (refused) is the expected outcome"
ok "$rc" 0 refuse

# 8. FS3 acceptance 2b: a corrupted metadata block is detected by its checksum
cp .w/e4.img .w/bad.img
ino="$(debugfs -R 'stat /docs/notes.txt' .w/e4.img 2>/dev/null | sed -n 's/^Inode: \([0-9]*\).*/\1/p')"
itab="$(dumpe2fs .w/e4.img 2>/dev/null | sed -n 's/^  Inode table at \([0-9]*\)-.*/\1/p' | head -n 1)"
off=$(( itab * 4096 + (ino - 1) * 256 + 0x10 ))                 # one byte of i_mtime
printf '\x5a' | dd of=.w/bad.img bs=1 seek=$off conv=notrunc status=none
{ echo "flipped byte $off (inode $ino, offset 0x10 = i_mtime)"
  .w/ext4read cat .w/bad.img /docs/notes.txt > /dev/null; echo "ext4read exit code $?"
  echo "== e2fsck -fn bad.img"; e2fsck -fn .w/bad.img 2>&1 | grep -iE 'checksum|inode '"$ino"; } > corrupt.out 2>&1
sed -i 's#\.w/##g' corrupt.out
grep -q "ext4read exit code 1" corrupt.out; rc=$?
rec corrupt "ext4read.cc cat" "$GXX_VER; $E2_VER" "dd (flip one byte of the inode of /docs/notes.txt); ./ext4read cat bad.img /docs/notes.txt; e2fsck -fn bad.img" "$rc" \
    "result:    ext4read exit code 1 (checksum mismatch reported) is the expected outcome"
ok "$rc" 0 corrupt

# 9. forensic evidence: the same listing from a colleague's build of the reader
.w/ext4read_bug tree .w/e4.img > .w/bug.txt 2>&1
{ grep -v '^checksums' .w/bug.txt | sort -k2,2 > .w/c; diff .w/c .w/b; echo "diff exit code $?"; } > forensic_tree.out 2>&1
grep -q 'diff exit code 1' forensic_tree.out; rc=$?
rec forensic_tree "ext4read.cc built for the forensic lab (see the answer key)" "$GXX_VER" "./ext4read tree e4.img | sort; diff with the host listing" "$rc" \
    "result:    diff exit code 1 (listings differ) is the expected outcome"
ok "$rc" 0 forensic_tree

rm -rf .w
exit $status
