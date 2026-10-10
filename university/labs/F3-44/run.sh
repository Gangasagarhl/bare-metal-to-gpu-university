#!/usr/bin/env bash
# F3-44 run.sh: milestone FS1 in the build container. Build the ext2 writer, check it with
# e2fsck and debugfs after 10,000 random operations, then run the power-cut harness:
# 500 cuts with careful ordering, 100 with the careless build, and the forensic evidence.
set -u -o pipefail
cd "$(dirname "$0")"
. ../F3-43/lablib.sh
status=0
PY_VER="$(python3 --version)"
DBG_VER="$(debugfs -V 2>&1 | head -n 1)"
ok() { [ "$1" = "$2" ] || { echo "step $3: exit $1, expected $2" >&2; status=1; }; }
rm -rf .w && mkdir -p .w/tmp
mkfs() { mke2fs -q -F -t ext2 -b 1024 -I 128 -N 2048 -m 0 -L fs1 \
             -U 0f5f0f5f-0401-4401-8401-000000000044 -E hash_seed=0f5f0f5f-0401-4401-8401-000000000044 "$1" 16M; }

# 1. build
hostbuild .w/fs1tool fs1tool.cc > build.out 2>&1; rc=$?
echo "build of fs1tool.cc (includes ext2w.h, blockdev.h): exit $rc (no messages means no warnings)" >> build.out
rec build "blockdev.h ext2w.h fs1tool.cc" "$GXX_VER" "g++ $HOSTFLAGS fs1tool.cc -o fs1tool" "$rc"
ok "$rc" 0 build

# 2. the empty file system, made by the host's mke2fs
{ mkfs .w/base.img 2>&1; dumpe2fs .w/base.img 2>/dev/null | grep -E '^(Filesystem features|Inode count|Block count|Block size|Inode size|Blocks per group|Inodes per group)'
  dumpe2fs .w/base.img 2>/dev/null | sed -n '/^Group 0/,/Inode table/p'; } > mkfs.out; rc=$?
rec mkfs "run.sh step 2" "$(mke2fs -V 2>&1 | head -n 1)" \
    "mke2fs -q -F -t ext2 -b 1024 -I 128 -N 2048 -m 0 -L fs1 -U <fixed> base.img 16M; dumpe2fs" "$rc"
ok "$rc" 0 mkfs

# 3. B16-style regression: 10,000 random operations, then e2fsck and every file's bytes
cp .w/base.img .w/stress.img && mkdir -p .w/es
.w/fs1tool workload .w/stress.img - 1 10000 careful .w/es > stress.out 2>&1; rc=$?
rec stress "fs1tool.cc workload" "$GXX_VER" "./fs1tool workload stress.img - 1 10000 careful es" "$rc"
ok "$rc" 0 stress
e2fsck -fn .w/stress.img > fsck_stress.out 2>&1; rc=$?
sed -i 's#\.w/##g' fsck_stress.out
rec fsck_stress "run.sh step 3" "$E2_VER" "e2fsck -fn stress.img" "$rc"
ok "$rc" 0 fsck_stress
awk -F'\t' '{printf "dump %s .w/d%d\n",$1,NR}' .w/es/final.txt > .w/cmds
debugfs -f .w/cmds .w/stress.img > /dev/null 2>&1
n=0; bad=0
while IFS=$'\t' read -r p f; do n=$((n+1)); cmp -s "$f" ".w/d$n" || { bad=$((bad+1)); echo "DIFFERENT: $p"; }; done < .w/es/final.txt > contents.out
echo "files compared byte for byte (debugfs dump vs. what the workload wrote): $n, different: $bad" >> contents.out
rec contents "run.sh step 3" "$DBG_VER" "debugfs -f cmds stress.img (dump every file); cmp with the expected bytes" "$bad"
ok "$bad" 0 contents

# 4. the recorded workload (careful ordering), checked whole
cp .w/base.img .w/careful.img && mkdir -p .w/ec
.w/fs1tool workload .w/careful.img .w/careful.stream 7 300 careful .w/ec > workload.out 2>&1; rc=$?
e2fsck -fn .w/careful.img 2>&1 | tail -n 1 | sed 's#\.w/##' >> workload.out
rec workload "fs1tool.cc workload" "$GXX_VER; $E2_VER" "./fs1tool workload careful.img careful.stream 7 300 careful ec; e2fsck -fn careful.img | tail -n 1" "$rc"
ok "$rc" 0 workload
.w/fs1tool show .w/careful.stream 0 14 > stream_careful.out 2>&1; rc=$?
rec stream_careful "fs1tool.cc show" "$GXX_VER" "./fs1tool show careful.stream 0 14" "$rc"

# 5. FS1 acceptance: 500 power cuts
python3 -I harness.py .w/fs1tool .w/base.img .w/careful.stream .w/ec 500 1 .w/tmp > harness_careful.out 2>&1; rc=$?
rec harness_careful "harness.py" "$PY_VER; $E2_VER; $GXX_VER" \
    "python3 -I harness.py fs1tool base.img careful.stream ec 500 1 tmp" "$rc"
ok "$rc" 0 harness_careful

# 6. the same workload with the careless build (flushes only at fsync; names written first)
cp .w/base.img .w/careless.img && mkdir -p .w/el
.w/fs1tool workload .w/careless.img .w/careless.stream 7 300 careless .w/el > /dev/null 2>&1
.w/fs1tool show .w/careless.stream 0 14 > stream_careless.out 2>&1
rec stream_careless "fs1tool.cc show" "$GXX_VER" "./fs1tool show careless.stream 0 14" "$?"
python3 -I harness.py .w/fs1tool .w/base.img .w/careless.stream .w/el 100 1 .w/tmp > harness_careless.out 2>&1; rc=$?
rec harness_careless "harness.py" "$PY_VER; $E2_VER; $GXX_VER" \
    "python3 -I harness.py fs1tool base.img careless.stream el 100 1 tmp" "$rc" \
    "result:    exit code 3 is the expected outcome: the careless build damages the file system"
ok "$rc" 3 harness_careless

# 7. forensic evidence: a three-operation careless workload, cut by power-cut seed 2
cp .w/base.img .w/f.img && mkdir -p .w/ef
.w/fs1tool workload .w/f.img .w/f.stream 2 3 careless .w/ef > /dev/null 2>&1
{ .w/fs1tool show .w/f.stream 0 16; .w/fs1tool crash .w/base.img .w/f.stream 2 .w/fc.img; } > forensic_stream.out 2>&1; rc=$?
rec forensic_stream "fs1tool.cc show / crash" "$GXX_VER" "./fs1tool show f.stream 0 16; ./fs1tool crash base.img f.stream 2 fc.img" "$rc"
e2fsck -fn .w/fc.img > forensic_fsck.out 2>&1; rc=$?
sed -i 's#\.w/##g' forensic_fsck.out
rec forensic_fsck "run.sh step 7" "$E2_VER" "e2fsck -fn fc.img" "$rc" "result:    exit code 4 is the expected outcome (errors left uncorrected; -n changes nothing)"
ok "$rc" 4 forensic_fsck
{ debugfs -R "ls -l /" .w/fc.img; debugfs -R "stat <12>" .w/fc.img | head -n 6; } > forensic_debugfs.out 2>&1; rc=$?
sed -i 's#\.w/##g' forensic_debugfs.out
rec forensic_debugfs "run.sh step 7" "$DBG_VER" "debugfs -R 'ls -l /' fc.img; debugfs -R 'stat <12>' fc.img | head -n 6" "$rc"

rm -rf .w
exit $status
