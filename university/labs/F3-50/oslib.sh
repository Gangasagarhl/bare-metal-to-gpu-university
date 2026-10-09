# oslib.sh - OS402 shared helpers, sourced by the run.sh of F3-50 ... F3-55.
#   rec <name> <listing> <toolchain> <command> <exit code> [extra lines...]  writes <name>.log
#   expect <rc> <wanted> <step>   records a failure in $status when an exit code is not the wanted one
GXX_VER="$(g++ --version | head -n 1)"
CLANG_VER="$(clang++ --version | head -n 1)"
A64_VER="$(aarch64-linux-gnu-g++ --version | head -n 1)"
RV_VER="$(riscv64-linux-gnu-g++ --version | head -n 1)"
QA64_VER="$(qemu-aarch64 --version | head -n 1)"
QRV_VER="$(qemu-riscv64 --version | head -n 1)"
STRACE_VER="$(strace -V | head -n 1)"
LDD_VER="$(ldd --version | head -n 1)"
BINUTILS_VER="$(ld --version | head -n 1)"
WFLAGS="-Wall -Wextra -Wpedantic -Werror"
HOSTFLAGS="-std=c++20 $WFLAGS -g -fsanitize=address,undefined"
HW_EMU="hardware:  untested on Arm/RISC-V hardware; QEMU user-mode emulation in the build container"
status=0
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
expect() {
    if [ "$1" != "$2" ]; then echo "step $3: exit code $1, wanted $2" >&2; status=1; fi
}
