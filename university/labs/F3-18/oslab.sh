# oslab.sh - OS302 shared helpers, sourced by the run.sh of F3-18 ... F3-25.
# rec <name> <listing> <toolchain> <command> <exit code> [extra lines...]  writes <name>.log
# qrun <outfile> <timeout s> <kernel.bin> [extra QEMU options...]          boots one kernel
QEMU=qemu-system-x86_64
QEMU_VER="$($QEMU --version | head -n 1)"
GXX_VER="$(g++ --version | head -n 1); $(ld --version | head -n 1)"
QBASE="-machine pc -m 128M -nodefaults -display none -no-reboot -monitor none -device isa-debug-exit,iobase=0xf4,iosize=0x04"
HW_NOTE="hardware:  untested on hardware; QEMU 8.2.2 with TCG (no KVM in the build container)"
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
qrun() {
    local out="$1" t="$2" bin="$3"
    shift 3
    timeout "$t" $QEMU $QBASE -serial stdio -kernel "$bin" "$@" > "$out" 2>&1
}
