#!/usr/bin/env bash
# F3-49 run.sh: milestone FS6 in the build container. No Windows, ntfs-3g or mkntfs is
# available here, so a generator of our own writes the test volume from a host tree; the
# read-only reader is checked against the host tree, against libblkid's NTFS probe, and
# against deliberate damage (torn records, a damaged $MFT record 0, a dirty volume).
set -u -o pipefail
cd "$(dirname "$0")"
. ../F3-43/lablib.sh
status=0
PY_VER="$(python3 --version)"
FILE_VER="$(file --version | head -n 1)"
ok() { [ "$1" = "$2" ] || { echo "step $3: exit $1, expected $2" >&2; status=1; }; }
rm -rf .w && mkdir -p .w
G=.w/ntfsgen; R=.w/ntfsread
poke() { printf "$3" | dd of="$1" bs=1 seek="$2" conv=notrunc status=none; }   # image, offset, bytes

# 1. build the generator, the reader, and the forensic lab's reader
{ hostbuild $G ntfsgen.cc; echo "ntfsgen.cc: exit $?"
  hostbuild $R ntfsread.cc; echo "ntfsread.cc: exit $?"
  hostbuild .w/ntfsread_bug -DFORENSIC_SKIP_RESTORE ntfsread.cc; echo "ntfsread.cc -DFORENSIC_SKIP_RESTORE: exit $?"; } > build.out 2>&1
grep -q 'exit [1-9]' build.out; rc=$((1 - $?))
rec build "ntfs.h ntfsgen.cc ntfsread.cc" "$GXX_VER" "g++ $HOSTFLAGS ntfsgen.cc|ntfsread.cc (and ntfsread.cc -DFORENSIC_SKIP_RESTORE)" "$rc"
ok "$rc" 0 build

# 2. the test volume, and what libblkid's NTFS probe and file(1) make of it
python3 -I mktree.py .w/tree
{ $G .w/n.img .w/tree OS401_NTFS; echo "== blkid -p -o export n.img"; blkid -p -o export .w/n.img
  echo "== file n.img"; file .w/n.img; } > mkimg.out 2>&1; rc=$?
sed -i 's#\.w/##g' mkimg.out
rec mkimg "mktree.py ntfsgen.cc" "$GXX_VER; $PY_VER; $BLKID_VER; $FILE_VER" "python3 -I mktree.py tree; ./ntfsgen n.img tree OS401_NTFS; blkid -p -o export n.img; file n.img" "$rc"
ok "$rc" 0 mkimg

# 3. the reader: volume information and its own consistency check
{ $R info .w/n.img; echo "== ntfsread check n.img"; $R check .w/n.img; } > info.out 2>&1; rc=$?
rec info "ntfsread.cc info / check" "$GXX_VER" "./ntfsread info n.img; ./ntfsread check n.img" "$rc"
ok "$rc" 0 info

# 4. every file and directory, compared with the host tree; two lookups through the B-trees
{ $R ls .w/n.img > .w/ls.txt; sed -n '1,12p' .w/ls.txt; echo "  ... ($(wc -l < .w/ls.txt) lines in all; the rest are IMG_0005.JPG ... IMG_0150.JPG and:)"; tail -n 1 .w/ls.txt; python3 -I hostlist.py .w/tree > .w/host.txt
  diff <(sort .w/ls.txt) .w/host.txt && echo "== all $(wc -l < .w/host.txt) entries equal the host tree (names, sizes, CRC-32)"
  echo "== ntfsread find"; $R find .w/n.img Photos/IMG_0100.JPG && $R find .w/n.img "Documents/Übersicht – Ελληνικά.txt"
  $R cat .w/n.img Documents/sparse.vhd | cmp - .w/tree/Documents/sparse.vhd && echo "cat Documents/sparse.vhd: identical (1 MiB, 2 clusters on disk)"; } > ls.out 2>&1; rc=$?
rec ls "ntfsread.cc ls / find / cat; hostlist.py" "$GXX_VER; $PY_VER" "./ntfsread ls n.img; python3 -I hostlist.py tree; diff; ./ntfsread find|cat n.img ..." "$rc"
ok "$rc" 0 ls

# 5. records: the MFT's own, a large directory, a fragmented file, a sparse file
{ for p in Photos Documents/fragmented.bin Documents/sparse.vhd; do
      r=$($R find .w/n.img "$p" | tail -n 1 | sed 's/.*record //'); echo "== $p"; $R record .w/n.img "$r"; done
  echo "== \$MFT"; $R record .w/n.img 0; } > records.out 2>&1; rc=$?
rec records "ntfsread.cc record" "$GXX_VER" "./ntfsread record n.img <n> (n from ./ntfsread find)" "$rc"
ok "$rc" 0 records

# 6. damage: a torn record (stride 2 of notes.txt's record carries a newer sequence number),
#    and a damaged record 0 of the $MFT (the mirror must take over)
cp .w/n.img .w/torn.img
r=$($R find .w/n.img notes.txt | tail -n 1 | sed 's/.*record //')
off=$(( 1536 * 4096 + (r - 24) * 1024 + 1022 ))
poke .w/torn.img $off '\x02\x00'
poke .w/torn.img $(( 4 * 4096 + 510 )) '\x07\x00'
{ echo "(record $r: bytes 1022-1023 changed from 01 00 to 02 00; record 0: bytes 510-511 to 07 00)"
  $R ls .w/torn.img > .w/tls.txt; echo "ntfsread ls exit code $?"; grep -v '^f /Photos/' .w/tls.txt; $R record .w/torn.img "$r"; $R info .w/torn.img | sed -n 3p
  $R check .w/torn.img; echo "ntfsread check exit code $?"; } > torn.out 2>&1
grep -q 'ls exit code 3' torn.out && grep -q 'check exit code 4' torn.out && grep -q 'taken from the mirror' torn.out; rc=$?
rec torn "ntfsread.cc on a damaged copy" "$GXX_VER" "dd (two 2-byte changes); ./ntfsread ls|record|info|check torn.img" "$rc" \
    "result:    expected refusals seen: ls exit 3 (one record refused), check exit 4, record 0 read from \$MFTMirr"
ok "$rc" 0 torn

# 7. a volume marked dirty is read, with a warning, and not written
$G .w/dirty.img .w/tree OS401_NTFS --dirty > /dev/null
sha256sum .w/dirty.img > .w/before
{ $R info .w/dirty.img | tail -n 5; sha256sum -c .w/before; } > dirty.out 2>&1; rc=$?
sed -i 's#\.w/##g' dirty.out
rec dirty "ntfsgen.cc --dirty; ntfsread.cc info" "$GXX_VER" "./ntfsgen dirty.img tree OS401_NTFS --dirty; ./ntfsread info dirty.img; sha256sum -c" "$rc"
ok "$rc" 0 dirty

# 8. forensic evidence: the colleague's reader build on the same, undamaged volume
{ .w/ntfsread_bug ls .w/n.img > .w/bug.txt 2> .w/bug.err; echo "exit code $?"
  echo "== stderr"; cat .w/bug.err; echo "== diff against the host listing (cat -v shows control characters)"
  diff <(sort .w/bug.txt) .w/host.txt | cat -v
  echo "== notes.txt"; .w/ntfsread_bug cat .w/n.img notes.txt | cmp - .w/tree/notes.txt
  .w/ntfsread_bug cat .w/n.img notes.txt | od -A d -t x1 -j 216 -N 16
  echo "== ntfsread check n.img (the same build)"; .w/ntfsread_bug check .w/n.img; } > forensic.out 2>&1
sed -i 's#\.w/##g; s#/dev/fd/[0-9]*#host.txt#g' forensic.out
rec forensic "ntfsread.cc built for the forensic lab (see the answer key)" "$GXX_VER" "./ntfsread_bug ls|cat|check n.img; diff; cmp; od" "0" \
    "result:    exit 0 everywhere, and still wrong: four names and notes.txt differ from the host tree (expected)"

rm -rf .w
exit $status
