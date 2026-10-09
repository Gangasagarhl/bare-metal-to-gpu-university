#!/usr/bin/env bash
# F2-44 lab = curriculum milestone P2 ("read a binary end to end"), ELF part.
# Called by run_lab.sh (which has already built and run elf_tests.cpp, the unit tests).
#   step binaries : builds the six sample ELF files the reader is tested on
#   step compare  : elfread versus readelf on all six, line by line (acceptance test 1)
#   step elfread_sample : elfread's own output for the small object file
#   step header   : the 64 header bytes of the PIE executable, raw and as readelf decodes them
#   step fuzz_short : 60 s of the university fuzzer under ASan/UBSan (acceptance test 2 needs an
#                     hour: see fuzz_hour.sh and fuzz_hour.log, run once for the chapter)
#   step forensic_exec : the "Exec format error" evidence for the forensic lab
set -u
cd "$(dirname "$0")" || exit 2
LAB="$(pwd)"
status=0
TOOL="$(g++ --version | head -n 1); $(readelf --version | head -n 1)"
SAN="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
W="${LAB}/.work"
rm -rf "$W"; mkdir -p "$W"

begin() {   # begin <name> <listing> <command summary> [extra toolchain text]
    NAME="$1"; LAST=0; BAD=0
    OUT="${LAB}/${NAME}.out"; LOG="${LAB}/${NAME}.log"; : > "$OUT"
    {
        echo "listing:   $2"
        echo "toolchain: $TOOL${4:-}"
        echo "command:   $3"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
    } > "$LOG"
    cd "$W" || exit 2
}
c() {       # c '<command>' [expected exit code, default 0]
    echo "\$ $1" >> "$OUT"
    bash -c "$1" >> "$OUT" 2>&1
    LAST=$?
    if [ "$LAST" != "${2:-0}" ]; then BAD=$((BAD + 1)); fi
    if [ "$LAST" != 0 ]; then echo "(exit code: $LAST)" >> "$OUT"; fi
}
finish() {
    sed -i -e "s#${W}/##g" -e "s#${LAB}/##g" "$OUT"
    echo "exit code: $LAST" >> "$LOG"
    if [ "$BAD" != 0 ]; then echo "result:    STEP FAILED ($BAD command(s) did not give the expected exit code)" >> "$LOG"; status=1; fi
    cd "$LAB" || exit 2
}

# Step 1: the six sample files.
begin binaries "samples/hello.cpp samples/greet.cpp" "g++ / aarch64-linux-gnu-g++ / strip (see binaries.out)" "; $(aarch64-linux-gnu-g++ --version | head -n 1)"
c "g++ -std=c++20 -O1 -c ${LAB}/samples/hello.cpp -o hello.o"
c "g++ -std=c++20 -O1 ${LAB}/samples/hello.cpp -o hello"
c "g++ -std=c++20 -O1 -static ${LAB}/samples/hello.cpp -o hello-static"
c "g++ -std=c++20 -O1 -fPIC -shared ${LAB}/samples/greet.cpp -o libgreet.so"
c "strip hello -o hello-stripped"
c "aarch64-linux-gnu-g++ -std=c++20 -O1 ${LAB}/samples/hello.cpp -o hello-arm64"
c "file hello.o hello hello-static libgreet.so hello-stripped hello-arm64 | sed 's/BuildID\\[sha1\\]=[0-9a-f]*/BuildID[sha1]=<varies>/'"
c "./hello"
finish

# Step 2: acceptance test 1 -- elfread must print exactly what readelf prints, for the ELF
# header, every section header, every program header and every symbol, on all six files.
g++ $SAN "${LAB}/elf/elfread.cpp" -o "$W/elfread" || status=1
begin compare "elf/elf_reader.h elf/elfread.cpp canon_readelf.sh" "for f in six files: diff <(canon_readelf.sh f) <(elfread f)"
for f in hello.o hello hello-static libgreet.so hello-stripped hello-arm64; do
    c "${LAB}/canon_readelf.sh $f > $f.readelf.txt && ./elfread $f > $f.elfread.txt && diff $f.readelf.txt $f.elfread.txt && echo \"MATCH: \$(wc -l < $f.elfread.txt) lines (\$(grep -c '^S' $f.elfread.txt) sections, \$(grep -c '^P' $f.elfread.txt) segments, \$(grep -c '^Y' $f.elfread.txt) symbols)\""
done
finish

# Step 3: what elfread prints, for the smallest file.
begin elfread_sample "elf/elfread.cpp" "./elfread hello.o"
c "./elfread hello.o"
finish

# Step 4: the 64-byte ELF header of the PIE executable, raw, then decoded by readelf;
# then its program headers (the loader's view) and the section-to-segment mapping.
begin header "samples/hello.cpp" "od -A d -t x1 -N 64 hello; readelf -hW hello; readelf -lW hello"
c "od -A d -t x1 -N 64 hello"
c "readelf -hW hello"
c "readelf -lW hello"
finish

# Step 5: a short fuzzing run (60 s) with the six files as seeds.
g++ $SAN -O1 "${LAB}/elf/fuzz.cpp" -o "$W/fuzz" || status=1
begin fuzz_short "elf/fuzz.cpp elf/elf_reader.h" "./fuzz 60 hello.o hello hello-static libgreet.so hello-stripped hello-arm64 (built with $SAN -O1)"
c "./fuzz 60 hello.o hello hello-static libgreet.so hello-stripped hello-arm64"
finish

# Step 6: forensic evidence -- a file that the shell refuses to run.
begin forensic_exec "samples/hello.cpp" "./hello-arm64; readelf -h; qemu-aarch64" "; $(qemu-aarch64 --version | head -n 1)"
c "ls -l hello hello-arm64 | awk '{print \$1, \$5, \$9}'"
c "./hello-arm64" 126
c "./elfread hello-arm64 | head -n 1"
c "./elfread hello | head -n 1"
c "readelf -lW hello-arm64 | grep -A1 INTERP"
c "qemu-aarch64 -L /usr/aarch64-linux-gnu ./hello-arm64"
finish
rm -rf "$W"
exit $status
