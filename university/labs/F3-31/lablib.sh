# lablib.sh - OS304 shared helpers, sourced by the run.sh of F3-31 ... F3-35.
#   rec <name> <listing> <toolchain> <command> <exit code> [extra lines...]  writes <name>.log
#   kbuild <out> <source files...>      builds the 32-bit Multiboot mini kernel of F3-31
#   qboot <outfile> <timeout> <kernel.elf> [QEMU options...]   boots it, serial on stdout
#   hostbuild <out> <sources...>        host C++ build with the run_lab.sh flags
K31="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
QEMU=qemu-system-x86_64
QEMU_VER="$($QEMU --version | head -n 1)"
GXX_VER="$(g++ --version | head -n 1)"
LD_VER="$(ld --version | head -n 1)"
QBASE="-machine pc -m 64M -nodefaults -display none -no-reboot -monitor none -device isa-debug-exit,iobase=0xf4,iosize=0x04"
HW_NOTE="hardware:  untested on hardware; QEMU 8.2.2 with TCG (no KVM in the build container)"
HOSTFLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
KFLAGS="-m32 -std=c++20 -ffreestanding -fno-exceptions -fno-rtti -fno-threadsafe-statics \
-fno-stack-protector -fno-pic -fno-builtin -mgeneral-regs-only -O2 -Wall -Wextra -Wpedantic -Werror -I$K31"
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
kbuild() {
    local out="$1"; shift
    local odir=".obj_$(basename "$out")" objs=() s
    rm -rf "$odir"; mkdir -p "$odir"
    for s in "$@"; do
        g++ $KFLAGS -c "$s" -o "$odir/$(basename "$s").o" || return 1
        objs+=("$odir/$(basename "$s").o")
    done
    ld -m elf_i386 -nostdlib -static --no-warn-rwx-segments -T "$K31/kernel.ld" -o "$out" "${objs[@]}" || return 1
    rm -rf "$odir"
}
qboot() {
    local out="$1" t="$2" k="$3"
    shift 3
    timeout "$t" $QEMU $QBASE -serial stdio -kernel "$k" "$@" > "$out" 2>&1
}
hostbuild() {
    local out="$1"; shift
    g++ $HOSTFLAGS "$@" -o "$out"
}
