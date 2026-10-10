#!/usr/bin/env bash
# F2-43 lab: object files, symbols and relocations, seen with the real tools.
# Called by run_lab.sh. Each step writes <name>.out (a transcript: every command after "$ ",
# then what it printed) and <name>.log (the run record).
set -u
cd "$(dirname "$0")" || exit 2
LAB="$(pwd)"
status=0
TOOL="$(g++ --version | head -n 1); $(ld --version | head -n 1); $(readelf --version | head -n 1)"
CXXF="-std=c++20 -O1 -U_FORTIFY_SOURCE -Wall -Wextra -Werror"

begin() {   # begin <name> <ok|fail> <folder> <listing> <command summary>
    NAME="$1"; EXPECT="$2"; LAST=0; BAD=0
    OUT="${LAB}/${NAME}.out"; LOG="${LAB}/${NAME}.log"; : > "$OUT"
    {
        echo "listing:   $4"
        echo "toolchain: $TOOL"
        echo "command:   $5"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
    } > "$LOG"
    cd "${LAB}/$3" || exit 2
}
c() {       # c '<command>': print it, run it, record failures
    echo "\$ $1" >> "$OUT"
    bash -c "$1" >> "$OUT" 2>&1
    LAST=$?
    if [ "$LAST" != 0 ]; then echo "(exit code: $LAST)" >> "$OUT"; BAD=$((BAD + 1)); fi
}
finish() {
    sed -i -e "s#${LAB}/##g" "$OUT"
    echo "exit code: $LAST" >> "$LOG"
    if [ "$EXPECT" = ok ] && [ "$BAD" != 0 ]; then
        echo "result:    STEP FAILED ($BAD command(s) failed)" >> "$LOG"; status=1
    fi
    if [ "$EXPECT" = fail ]; then
        if [ "$LAST" != 0 ] && [ "$BAD" = 1 ]; then
            echo "result:    link failed as expected on the last command (messages saved in ${NAME}.out)" >> "$LOG"
        else
            echo "result:    UNEXPECTED (only the last command was expected to fail)" >> "$LOG"; status=1
        fi
    fi
    cd "$LAB" || exit 2
}

# Step 1: two object files, before linking.
begin objects ok shop "shop/main.cpp shop/counter.cpp" "g++ $CXXF -c main.cpp counter.cpp; file, nm, readelf -s, readelf -r, objdump -dr"
c "g++ $CXXF -c main.cpp -o main.o"
c "g++ $CXXF -c counter.cpp -o counter.o"
c "file main.o counter.o"
c "nm counter.o"
c "nm -C counter.o"
c "nm main.o"
c "readelf -sW counter.o"
c "readelf -SW counter.o | sed -n '1,/Key to Flags/p' | grep -v 'Key to Flags'"
c "readelf -rW main.o"
c "objdump -dr --no-show-raw-insn main.o"
finish

# Step 2: link, run, and check the linker's arithmetic for every relocation in main's code.
begin linked ok shop "shop/main.cpp shop/counter.cpp tools/reloc_check.cpp" "g++ main.o counter.o -o shop; ./shop; nm; objdump -d; reloc_check"
c "g++ $CXXF main.o counter.o -o shop"
c "./shop"
c "file shop"
c "nm shop | grep -E ' (main|g_orders|_Z13next_order_idv|_Z12calls_so_farv|_ZL7s_calls)\$'"
c "objdump -d --disassemble=main shop | sed -n '/<main>:/,\$p'"
g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined \
    "${LAB}/tools/reloc_check.cpp" -o .reloc_check || status=1
MAIN=$(nm shop | awk '$3=="main"{print $1}')
while read -r off info type symval sym plus addend; do
    case "$type" in R_X86_64_PC32|R_X86_64_PLT32) ;; *) continue ;; esac
    S=$(nm shop | awk -v s="$sym" '$3==s{print $1}')
    [ -n "$S" ] || continue                     # printf lives in the C library: resolved at run time
    [ "$plus" = "-" ] && A="-$addend" || A="$addend"
    P=$(printf '%x' $(( 0x$MAIN + 0x$off )))
    # the field's 4 bytes, read from the file: file offset = P - .text address + .text offset
    read -r TADDR TOFF < <(readelf -SW shop | awk '$2==".text"{print $4, $5}')
    FOFF=$(( 0x$P - 0x$TADDR + 0x$TOFF ))
    BYTES=$(od -An -tx1 -j "$FOFF" -N 4 shop | xargs)
    c "./.reloc_check $type $S $A $P \"$BYTES\"   # $sym at main+0x$off"
done < <(readelf -rW main.o | awk '/^Relocation section .\.rela\.text/{f=1;next} /^Relocation section/{f=0} f && /^[0-9a-f]/')
c "readelf -rW shop | grep -E 'printf|Relocation section'"
rm -f .reloc_check shop main.o counter.o
finish

# Step 2b: a 32-bit PC-relative field cannot reach 8 GiB away: put .data there on purpose.
begin far fail shop "shop/main.cpp shop/counter.cpp" "g++ main.o counter.o -Wl,--section-start=.data=0x200000000 -o far (expected to fail)"
c "g++ $CXXF -c main.cpp -o main.o && g++ $CXXF -c counter.cpp -o counter.o"
c "g++ main.o counter.o -Wl,--section-start=.data=0x200000000 -o far 2>&1 | grep -E 'main\\.(o|cpp)|counter\\.(o|cpp)|collect2'; exit \${PIPESTATUS[0]}"
rm -f main.o counter.o far
finish

# Step 3 (forensic evidence): the oven controller as built by its Makefile.
begin forensic_build ok forensic "forensic/Makefile forensic/main.cpp forensic/sensor.cpp forensic/board.cpp" "make; ./oven; nm; grep in oven.map"
c "make clean"
c "ls"
c "make"
c "./oven"
c "nm -C oven | grep on_overheat"
c "nm -C sensor.o | grep on_overheat"
c "grep -E '^ \\.text +0x[0-9a-f]+ +0x[0-9a-f]+ .*\\.o\$' oven.map"
c "ls *.o"
c "make clean"
finish

# Step 4 (forensic answer check): add board.o to the link and compare.
begin forensic_fixed ok forensic "forensic/Makefile forensic/board.cpp" "make OBJS='main.o sensor.o board.o' with board.o built; ./oven; nm"
c "make main.o sensor.o board.o"
c "make oven OBJS='main.o sensor.o board.o'"
c "./oven"
c "nm -C oven | grep on_overheat"
c "make clean"
finish
exit $status
