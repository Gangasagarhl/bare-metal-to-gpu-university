#!/usr/bin/env bash
# F3-45 run.sh: milestone FS2 in the build container. A JBD2 journal under the F3-44 writer:
# replay of a journal written by e2fsprogs (debugfs), our own workload and its journal,
# the power-cut harness (500 random cuts, 200 cuts right after a commit block), and the
# forensic evidence: the same workload with the flush before the commit block left out.
set -u -o pipefail
cd "$(dirname "$0")"
. ../F3-43/lablib.sh
status=0
PY_VER="$(python3 --version)"
DBG_VER="$(debugfs -V 2>&1 | head -n 1)"
ok() { [ "$1" = "$2" ] || { echo "step $3: exit $1, expected $2" >&2; status=1; }; }
rm -rf .w && mkdir -p .w/tmp
mkfs() { mke2fs -q -F -t ext3 -b 1024 -I 128 -N 2048 -m 0 -J size=1 -L fs2 \
             -U 0f5f0f5f-0401-4401-8401-000000000045 -E hash_seed=0f5f0f5f-0401-4401-8401-000000000045 "$1" 16M; }

# 1. build
hostbuild .w/fs2tool fs2tool.cc > build.out 2>&1; rc=$?
echo "build of fs2tool.cc (journal.h + the F3-44 headers): exit $rc (no messages means no warnings)" >> build.out
rec build "journal.h fs2tool.cc ../F3-44/{blockdev,ext2w,stream,workload}.h" "$GXX_VER" "g++ $HOSTFLAGS fs2tool.cc -o fs2tool" "$rc"
ok "$rc" 0 build

# 2. an ext3 file system (ext2 + a 1 MiB journal) made by mke2fs
{ mkfs .w/base.img 2>&1 | grep -v '^Creating'; dumpe2fs -h .w/base.img 2>/dev/null | grep -E '^(Filesystem features|Journal)'
  debugfs -R "stat <8>" .w/base.img 2>/dev/null | sed -n '/^BLOCKS/,/^TOTAL/p'; } > mkfs.out; rc=$?
rec mkfs "run.sh step 2" "$(mke2fs -V 2>&1 | head -n 1)" \
    "mke2fs -q -F -t ext3 -b 1024 -I 128 -N 2048 -m 0 -J size=1 -L fs2 -U <fixed> base.img 16M; dumpe2fs -h; debugfs stat <8>" "$rc"
ok "$rc" 0 mkfs

# 3. a journal written by e2fsprogs (two transactions, the second revokes block 301),
#    replayed by e2fsck (image A) and by our recovery code (image B)
cp .w/base.img .w/jx.img
python3 -I -c "import sys; sys.stdout.buffer.write(bytes((i*7+3)%251 for i in range(3072)))" > .w/three
printf 'jo\njw -b 300,301,302 .w/three\njc\njo\njw -r 301\njc\n' > .w/cmds
{ debugfs -w -f .w/cmds .w/jx.img 2>&1 | grep -v '^debugfs 1'; debugfs -R logdump .w/jx.img 2>&1 | grep -v '^debugfs 1'
  cp .w/jx.img .w/A.img; cp .w/jx.img .w/B.img
  echo "== A: e2fsck -fy A.img"; e2fsck -fy .w/A.img 2>&1 | grep -E 'recovering|files'
  echo "== B: fs2tool recover B.img"; .w/fs2tool recover .w/B.img
  echo "== B: e2fsck -fn B.img"; e2fsck -fn .w/B.img 2>&1 | tail -n 1; echo "e2fsck exit code $?"
  for b in 300 301 302; do
      x="$(dd if=.w/A.img bs=1024 skip=$b count=1 status=none | sha256sum | cut -c1-16)"
      y="$(dd if=.w/B.img bs=1024 skip=$b count=1 status=none | sha256sum | cut -c1-16)"
      z="$(dd if=.w/three bs=1024 skip=$((b-300)) count=1 status=none | sha256sum | cut -c1-16)"
      echo "block $b: A $x  B $y  logged $z  $([ "$x" = "$y" ] && echo 'A = B' || echo 'A != B')"
  done; } > linuxside.out 2>&1
sed -i 's#\.w/##g' linuxside.out
grep -q 'A != B' linuxside.out; rc=$((1 - $?))
rec linuxside "run.sh step 3" "$DBG_VER; $E2_VER; $GXX_VER" \
    "debugfs -w (jo; jw -b 300,301,302 three; jc; jo; jw -r 301; jc); logdump; e2fsck -fy A.img; fs2tool recover B.img; compare blocks" "$rc"
ok "$rc" 0 linuxside

# 4. our workload on the journal (the F3-44 workload, seed 7, 300 operations)
cp .w/base.img .w/j.img && mkdir -p .w/ej
{ .w/fs2tool workload .w/j.img .w/j.stream 7 300 journal .w/ej; e2fsck -fn .w/j.img 2>&1 | tail -n 1
  debugfs -R logdump .w/j.img 2>&1 | grep -v '^debugfs 1'; } > workload.out 2>&1; rc=$?
sed -i 's#\.w/##g' workload.out
rec workload "fs2tool.cc workload" "$GXX_VER; $E2_VER" "./fs2tool workload j.img j.stream 7 300 journal ej; e2fsck -fn j.img; debugfs -R logdump j.img" "$rc"
ok "$rc" 0 workload
cj="$(.w/fs2tool commits .w/j.stream | sed -n 7p)"            # just after the 7th commit block
.w/fs2tool showj .w/base.img .w/j.stream $((cj - 30)) 31 > stream_journal.out 2>&1; rc=$?
rec stream_journal "fs2tool.cc showj" "$GXX_VER" "./fs2tool showj base.img j.stream $((cj - 30)) 31" "$rc"

# 5. FS2 acceptance: the FS1 harness, 500 random power cuts; then 200 cuts at commit points
python3 -I harness.py .w/fs2tool .w/base.img .w/j.stream .w/ej 500 1 .w/tmp > harness_random.out 2>&1; rc=$?
rec harness_random "harness.py" "$PY_VER; $E2_VER; $GXX_VER" "python3 -I harness.py fs2tool base.img j.stream ej 500 1 tmp" "$rc"
ok "$rc" 0 harness_random
python3 -I harness.py .w/fs2tool .w/base.img .w/j.stream .w/ej 200 1 .w/tmp commits > harness_commits.out 2>&1; rc=$?
rec harness_commits "harness.py" "$PY_VER; $E2_VER; $GXX_VER" "python3 -I harness.py fs2tool base.img j.stream ej 200 1 tmp commits" "$rc"
ok "$rc" 0 harness_commits

# 6. forensic evidence: the same workload built WITHOUT the flush before the commit block
cp .w/base.img .w/n.img && mkdir -p .w/en
.w/fs2tool workload .w/n.img .w/n.stream 7 300 nobarrier .w/en > /dev/null 2>&1
python3 -I harness.py .w/fs2tool .w/base.img .w/n.stream .w/en 100 1 .w/tmp commits > forensic_harness.out 2>&1; rc=$?
rec forensic_harness "harness.py" "$PY_VER; $E2_VER; $GXX_VER" "python3 -I harness.py fs2tool base.img n.stream en 100 1 tmp commits" "$rc" \
    "result:    exit code 3 is the expected outcome: this build loses fsynced data"
ok "$rc" 3 forensic_harness
cn="$(.w/fs2tool commits .w/n.stream | sed -n 7p)"            # the cut point the harness used for seed 6
.w/fs2tool showj .w/base.img .w/n.stream $((cn - 30)) 30 > forensic_stream.out 2>&1; rc=$?
rec forensic_stream "fs2tool.cc showj" "$GXX_VER" "./fs2tool showj base.img n.stream $((cn - 30)) 30" "$rc"
{ .w/fs2tool crash .w/base.img .w/n.stream 6 .w/c6.img "$cn"; .w/fs2tool recover .w/c6.img
  e2fsck -fn .w/c6.img; echo "e2fsck exit code $?"; } > forensic_fsck.out 2>&1
sed -i 's#\.w/##g' forensic_fsck.out
grep -q 'e2fsck exit code 4' forensic_fsck.out; rc=$?
rec forensic_fsck "run.sh step 6" "$E2_VER; $GXX_VER" "./fs2tool crash base.img n.stream 6 c6.img $cn; ./fs2tool recover c6.img; e2fsck -fn c6.img" "$rc" \
    "result:    e2fsck exit code 4 (errors left uncorrected) is the expected outcome"
ok "$rc" 0 forensic_fsck

rm -rf .w
exit $status
