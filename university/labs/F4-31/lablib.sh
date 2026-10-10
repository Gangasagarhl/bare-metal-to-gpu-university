# lablib.sh - DR403 lab helpers, sourced by the run.sh of F4-31 to F4-37.
# rec NAME LISTING TOOLCHAIN COMMAND EXIT [extra lines...] writes NAME.log in the course format.
LAB_DIR_31="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
GXX_VER="$(g++ --version | head -n 1)"
XGXX_VER="$(aarch64-linux-gnu-g++ --version | head -n 1)"
XLD_VER="$(aarch64-linux-gnu-ld --version | head -n 1)"
QEMU_VER="$(qemu-system-aarch64 --version | head -n 1)"
PY_VER="$(python3 --version)"
EMU_NOTE="hardware:  untested on hardware; run in QEMU (qemu-system-aarch64), not on a real board"
HOSTFLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
KFLAGS="-g -std=c++20 -ffreestanding -fno-exceptions -fno-rtti -fno-stack-protector -fno-threadsafe-statics -mgeneral-regs-only -mstrict-align -fno-tree-loop-distribute-patterns -nostdlib -O2 -Wall -Wextra -Wpedantic -Werror"
XGXX=aarch64-linux-gnu-g++
XLD="aarch64-linux-gnu-ld --no-warn-rwx-segments"

rec() {
    local name="$1" listing="$2" tool="$3" cmd="$4" rc="$5"
    shift 5
    {
        echo "listing:   $listing"
        echo "toolchain: $tool"
        echo "command:   $cmd"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
        echo "exit code: $rc"
        for extra in "$@"; do echo "$extra"; done
    } > "$name.log"
}

# hostbuild OUT SOURCES... : the university's host flags (sanitizers on)
hostbuild() {
    local out="$1"
    shift
    g++ $HOSTFLAGS "$@" -o "$out"
}

# kbuild OUT.bin OBJDIR SOURCES... : position-independent DR403 kernel image (start.S first)
kbuild() {
    local out="$1" obj="$2"
    shift 2
    mkdir -p "$obj"
    local objs=()
    local src
    for src in "$LAB_DIR_31/start.S" "$LAB_DIR_31/kbase.cc" "$@"; do
        local o="$obj/$(basename "${src%.*}").o"
        $XGXX $KFLAGS -fpie -c "$src" -o "$o" || return 1
        objs+=("$o")
    done
    $XLD -pie --no-dynamic-linker -T "$LAB_DIR_31/kernel.ld" -o "${out%.bin}.elf" "${objs[@]}" || return 1
    aarch64-linux-gnu-objcopy -O binary "${out%.bin}.elf" "$out"
}

# qrun TIMEOUT -- qemu command... : serial output with carriage returns removed; 124 on timeout
qrun() {
    local t="$1"
    shift 2
    timeout "$t" "$@" < /dev/null 2>&1 | tr -d '\r'
    return "${PIPESTATUS[0]}"
}

# hmp QEMU-BINARY MACHINE-ARGS... : run QEMU stopped (-S), send the monitor commands read from
# stdin, print only the answers (the monitor's echo of the typed command is dropped)
hmp() {
    local q="$1"
    shift
    timeout 30 "$q" "$@" -display none -monitor stdio -serial null -S 2>&1 |
        sed 's/\x1b\[[0-9;]*[A-Za-z]//g' | tr -d '\r' | grep -a -v '^(qemu)' | grep -a -v '^QEMU .* monitor'
}

# memory_view : from "info mtree -f" output on stdin, keep only the flat view of the CPU's
# address space (the one QEMU labels AS "memory")
memory_view() {
    awk '/^FlatView/{keep=0} /AS "memory"/{keep=1} keep'
}
