# lablib.sh - OS401 shared helpers, sourced by the run.sh of F3-43 ... F3-49.
#   rec <name> <listing> <toolchain> <command> <exit code> [extra lines...]  writes <name>.log
#   hostbuild <out> <sources...>   host C++ build with the run_lab.sh flags
GXX_VER="$(g++ --version | head -n 1)"
E2_VER="$(e2fsck -V 2>&1 | head -n 1)"
BLKID_VER="$(blkid -V 2>&1 | head -n 1)"
HOSTFLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
export LC_ALL=C TZ=UTC E2FSPROGS_FAKE_TIME=1700000000
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
hostbuild() {
    local out="$1"; shift
    g++ $HOSTFLAGS "$@" -o "$out"
}
