# rblib.sh - RB403 lab helpers, sourced by the run.sh of F9-65 to F9-71.
# rec NAME LISTING TOOLCHAIN COMMAND EXIT [extra lines...] writes NAME.log in the course format.
RB_LIB_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
GXX_VER="$(g++ --version | head -n 1)"
XGXX_VER="$(aarch64-linux-gnu-g++ --version | head -n 1)"
XLD_VER="$(aarch64-linux-gnu-ld --version | head -n 1)"
QEMU_VER="$(qemu-system-aarch64 --version | head -n 1)"
PY_VER="$(python3 --version)"
EMU_NOTE="hardware:  untested on hardware; run in QEMU (qemu-system-aarch64 -M virt), not on a real robot computer"
HOSTFLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
MEASFLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -O2 -pthread"
KFLAGS="-g -std=c++20 -ffreestanding -fno-exceptions -fno-rtti -fno-stack-protector -fno-threadsafe-statics -mgeneral-regs-only -mstrict-align -fno-tree-loop-distribute-patterns -nostdlib -O2 -Wall -Wextra -Wpedantic -Werror"
XGXX=aarch64-linux-gnu-g++
XLD="aarch64-linux-gnu-ld --no-warn-rwx-segments"
ROBOT_VM="qemu-system-aarch64 -M virt,acpi=off -cpu cortex-a53 -m 256M"

rec() {
    local name="$1" listing="$2" tool="$3" cmd="$4" rc="$5"
    shift 5
    {
        echo "listing:   $listing"
        echo "toolchain: $tool"
        echo "command:   $cmd"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m), $(nproc) CPUs (cloud build container, a virtual machine)"
        echo "exit code: $rc"
        for extra in "$@"; do echo "$extra"; done
    } > "$name.log"
}

# kbuild OUT_IMAGE OBJDIR INCLUDE_DIR SOURCES... : the position-independent course kernel image
kbuild() {
    local out="$1" obj="$2" inc="$3"
    shift 3
    mkdir -p "$obj"
    local objs=() src
    for src in "$RB_LIB_DIR/start.S" "$RB_LIB_DIR/kbase.cc" "$@"; do
        local o="$obj/$(basename "${src%.*}").o"
        $XGXX $KFLAGS -I"$inc" -I"$RB_LIB_DIR" -fpie -c "$src" -o "$o" || return 1
        objs+=("$o")
    done
    $XLD -pie --no-dynamic-linker -T "$RB_LIB_DIR/kernel.ld" -o "$obj/image.elf" "${objs[@]}" || return 1
    aarch64-linux-gnu-objcopy -O binary "$obj/image.elf" "$out"
}

# qrun TIMEOUT -- command... : output with carriage returns removed; 124 on timeout
qrun() {
    local t="$1"
    shift 2
    timeout "$t" "$@" < /dev/null 2>&1 | tr -d '\r'
    return "${PIPESTATUS[0]}"
}
