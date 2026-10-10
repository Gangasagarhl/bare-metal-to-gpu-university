#!/usr/bin/env bash
# F3-47 run.sh: milestone FS4 in the build container, as far as it can go here: format,
# write and read exFAT volumes with our tool, check them with our checker and with
# libblkid's exFAT probe (the only independent exFAT code in the container), a 10,000-
# operation stress test, fragmented allocation, and the forensic lab's broken build.
set -u -o pipefail
cd "$(dirname "$0")"
. ../F3-43/lablib.sh
status=0
PY_VER="$(python3 --version)"
FILE_VER="$(file --version | head -n 1)"
ok() { [ "$1" = "$2" ] || { echo "step $3: exit $1, expected $2" >&2; status=1; }; }
rm -rf .w && mkdir -p .w
T=.w/exfattool; B=.w/exfattool_bug

# 1. build, and the forensic lab's variant
{ hostbuild $T exfattool.cc; echo "exfattool.cc: exit $?"
  hostbuild $B -DFORENSIC_ALWAYS_NOFATCHAIN exfattool.cc; echo "exfattool.cc -DFORENSIC_ALWAYS_NOFATCHAIN: exit $?"; } > build.out 2>&1
grep -q 'exit [1-9]' build.out; rc=$((1 - $?))
rec build "exfattool.cc" "$GXX_VER" "g++ $HOSTFLAGS exfattool.cc -o exfattool (and again with -DFORENSIC_ALWAYS_NOFATCHAIN)" "$rc"
ok "$rc" 0 build

# 2. format, then ask libblkid and file(1) what they see
{ $T mkfs .w/x.img 64 CAMERA; echo "== blkid -p -o export x.img"; blkid -p -o export .w/x.img
  echo "== file x.img"; file .w/x.img; } > mkfs.out 2>&1; rc=$?
sed -i 's#\.w/##g' mkfs.out
rec mkfs "exfattool.cc mkfs" "$GXX_VER; $BLKID_VER; $FILE_VER" "./exfattool mkfs x.img 64 CAMERA; blkid -p -o export x.img; file x.img" "$rc"
ok "$rc" 0 mkfs

# 3. write and read back, names outside ASCII included
printf 'hello exfat\n' > .w/h.txt
python3 -I -c "import sys; sys.stdout.buffer.write(bytes((i*13+5)%256 for i in range(100000)))" > .w/r.bin
{ $T mkdir .w/x.img /DCIM && $T put .w/x.img /DCIM/hello.txt .w/h.txt && $T put .w/x.img "/Grüße aus Köln.bin" .w/r.bin
  $T ls .w/x.img /; $T ls .w/x.img /DCIM
  $T cat .w/x.img "/GRüßE AUS KöLN.BIN" | cmp - .w/r.bin && echo "cat \"/GRüßE AUS KöLN.BIN\": identical to the 100000-byte source"
  echo "== the root directory's entries (32 bytes each)"; $T entries .w/x.img /; } > rw.out 2>&1; rc=$?
rec rw "exfattool.cc mkdir/put/ls/cat/entries" "$GXX_VER" "./exfattool mkdir|put|ls|cat|entries x.img ..." "$rc"
ok "$rc" 0 rw
{ $T check .w/x.img; echo "== blkid"; blkid -p -o export .w/x.img | grep -E '^(LABEL|UUID|TYPE)='; } > check.out 2>&1; rc=$?
sed -i 's#\.w/##g' check.out
rec check "exfattool.cc check" "$GXX_VER; $BLKID_VER" "./exfattool check x.img; blkid -p -o export x.img" "$rc"
ok "$rc" 0 check

# 4. libblkid validates the boot checksum: change one byte of boot code and probe again
cp .w/x.img .w/z.img
printf '\x01' | dd of=.w/z.img bs=1 seek=200 conv=notrunc status=none
{ echo "== blkid -p -o export z.img (byte 200 changed, checksum not updated)"; blkid -p -o export .w/z.img
  echo "blkid exit code $?"; $T check .w/z.img; echo "exfattool exit code $?"; } > blkid_checksum.out 2>&1
sed -i 's#\.w/##g' blkid_checksum.out
grep -q 'TYPE=exfat' blkid_checksum.out; rc=$((1 - $?))
rec blkid_checksum "run.sh step 4" "$BLKID_VER; $GXX_VER" "dd (change byte 200); blkid -p -o export z.img; ./exfattool check z.img" "$rc" \
    "result:    both tools reject the volume: the expected outcome"
ok "$rc" 0 blkid_checksum

# 5. FS4-style stress test (our checker stands in for fsck.exfat, which is not installed)
$T mkfs .w/s.img 64 STRESS > /dev/null
{ $T stress .w/s.img 10000 47; echo "== blkid"; blkid -p -o export .w/s.img | grep -E '^(LABEL|TYPE)='; } > stress.out 2>&1; rc=$?
rec stress "exfattool.cc stress" "$GXX_VER; $BLKID_VER" "./exfattool stress s.img 10000 47; blkid" "$rc"
ok "$rc" 0 stress

# 6. a full card, every other photo deleted, then a 60000-byte movie: a fragmented file
python3 -I -c "
for i in range(60): open('.w/p%02d.jpg'%i,'wb').write(bytes([(i*13+k)%256 for k in range(40000)]))
open('.w/movie.mp4','wb').write(bytes([(k*7)%251 for k in range(60000)]))"
card() {   # $1 = the tool to use
    rm -f .w/f.img; $1 mkfs .w/f.img 2 CARD > /dev/null
    local i=0; while $1 put .w/f.img /IMG$(printf %02d $i).JPG .w/p$(printf %02d $i).jpg 2>/dev/null; do i=$((i+1)); done
    echo "photos written until the card was full: $i; deleting every other one"
    for k in $(seq 0 2 $((i-1))); do $1 rm .w/f.img /IMG$(printf %02d $k).JPG; done
    $1 put .w/f.img /MOVIE.MP4 .w/movie.mp4
    $1 ls .w/f.img / | grep -E 'MOVIE|IMG0[0-3]'
    $1 cat .w/f.img /MOVIE.MP4 | cmp - .w/movie.mp4 && echo "MOVIE.MP4 reads back identical"
    echo "== the entry set of MOVIE.MP4 (File, Stream Extension, File Name)"
    $1 entries .w/f.img / | grep -B2 ': c1 00 4d 00 4f 00 56 00'      # "MOV" in UTF-16
    echo "== FAT entries of clusters 5-16"; $1 fat .w/f.img 5 12
    $1 check .w/f.img
}
card $T > frag.out 2>&1; rc=$?
sed -i 's#\.w/##g' frag.out
rec frag "exfattool.cc; run.sh step 6" "$GXX_VER" "card(): fill a 2 MiB volume, delete every other file, put MOVIE.MP4, ls, cat|cmp, entries, fat, check" "$rc"
ok "$rc" 0 frag

# 7. forensic evidence: the same story with the colleague's build
card $B > forensic.out 2>&1; rc=$?
sed -i 's#\.w/##g' forensic.out
rec forensic "exfattool.cc built for the forensic lab (see the answer key)" "$GXX_VER" "card() with the forensic build" "$rc" \
    "result:    exit code 4 (check found problems) is the expected outcome"
ok "$rc" 4 forensic

rm -rf .w
exit $status
