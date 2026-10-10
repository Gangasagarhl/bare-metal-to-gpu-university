# dlab.sh - DR402 shared lab helpers, sourced by the run.sh of F4-23 ... F4-30.
#   rec <name> <listing> <toolchain> <command> <exit code> [extra lines...]  writes <name>.log
#   kbuild <arch> <out> "<extra flags>" <sources...>   builds <out>.elf and <out>.bin
#       arch = aarch64 | riscv64. Sources are paths relative to the calling lab folder.
#       The linker script is ./linker.ld if present, else the port's (F4-24 or F4-28), or $KLDS.
#       Files reused from F3-18 (REUSED below) are copied byte for byte into a scratch folder
#       and compiled from there, so that their #include "arch.h" finds THIS port's arch.h.
DLAB="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LABS="$(cd "$DLAB/.." && pwd)"
REUSED="kprint.cc panic.cc cxxrt.cc"          # arch-neutral C++ from F3-18, never edited here
QEMU_A64_VER="$(qemu-system-aarch64 --version | head -n 1)"
QEMU_RV_VER="$(qemu-system-riscv64 --version | head -n 1)"
A64_VER="$(aarch64-linux-gnu-g++ --version | head -n 1); $(aarch64-linux-gnu-ld --version | head -n 1)"
RV_VER="$(riscv64-linux-gnu-g++ --version | head -n 1); $(riscv64-linux-gnu-ld --version | head -n 1)"
HOST_VER="$(g++ --version | head -n 1)"
HW_NOTE="hardware:  untested on hardware; QEMU 8.2.2 with TCG (no KVM in the build container)"
KFLAGS_COMMON="-std=c++20 -ffreestanding -fno-exceptions -fno-rtti -fno-threadsafe-statics \
-fno-stack-protector -fno-pie -fno-pic -fno-omit-frame-pointer -O2 -g -Wall -Wextra -Wpedantic -Werror"
KFLAGS_aarch64="-mgeneral-regs-only -mstrict-align -mno-outline-atomics"
KFLAGS_riscv64="-march=rv64imac_zicsr_zifencei -mabi=lp64 -mcmodel=medany -I$LABS/F4-28/include"

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
    local arch="$1" out="$2" extra="$3"
    shift 3
    local cxx="${arch}-linux-gnu-g++" ld="${arch}-linux-gnu-ld" objcopy="${arch}-linux-gnu-objcopy"
    local flagsvar="KFLAGS_${arch}"
    local here; here="$(pwd)"
    local port_dir="$LABS/F4-24"; [ "$arch" = riscv64 ] && port_dir="$LABS/F4-28"
    local inc="-I$here -I$port_dir -I$LABS/F4-23 -I$LABS/F3-18"
    local tmp=".obj_$out"
    rm -rf "$tmp"; mkdir -p "$tmp/reused"
    for f in $REUSED; do cp "$LABS/F3-18/$f" "$tmp/reused/$f"; done
    local objs=() src o
    for src in "$@" $(for f in $REUSED; do echo "$tmp/reused/$f"; done); do
        o="$tmp/$(basename "$src").o"
        $cxx $KFLAGS_COMMON ${!flagsvar} $extra $inc -c "$src" -o "$o" || { rm -rf "$tmp"; return 1; }
        objs+=("$o")
    done
    local lds="$port_dir/linker.ld"
    [ -f "$here/linker.ld" ] && lds="$here/linker.ld"
    [ -n "${KLDS:-}" ] && lds="$KLDS"           # a run.sh may pass another script (forensics)
    $ld -nostdlib -static --no-warn-rwx-segments -T "$lds" -o "$out.elf" "${objs[@]}" || { rm -rf "$tmp"; return 1; }
    $objcopy -O binary "$out.elf" "$out.bin"
    rm -rf "$tmp"
}

# Prints the SHA-256 of every reused F3-18 file: the build log proves they were not edited.
reuse_record() {
    (cd "$LABS/F3-18" && sha256sum kformat.h kprint.h serial.h panic.h $REUSED)
}
