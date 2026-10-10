# labtools.sh - helpers shared by the run.sh scripts of the OS303 labs (F3-26 ... F3-30).
# Every step writes <name>.out (the real output) and <name>.log (the run record).
LABS="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
QEMU=qemu-system-x86_64
QEMUV="$($QEMU --version | head -n 1)"
GCCV="$(g++ --version | head -n 1); $(ld --version | head -n 1)"
EMU="hardware:  untested on hardware; QEMU TCG emulation of a PC (machine 'pc'), no KVM in the build container"

rec() {   # rec <name> <listing> <toolchain> <command> <exit-code text> [extra line]...
    local name="$1" listing="$2" tool="$3" cmd="$4" code="$5"; shift 5
    {
        echo "listing:   $listing"
        echo "toolchain: $tool"
        echo "command:   $cmd"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
        echo "exit code: $code"
        for line in "$@"; do echo "$line"; done
    } > "$name.log"
}

# build_step <name> <build dir> [extra compiler flags]: build kernel + user programs
build_step() {
    local name="$1" dir="$2"; shift 2
    if "$LABS/F3-26/build_kernel.sh" "$dir" "$@" > "$name.out" 2>&1; then
        (cd "$dir" && size kernel.elf u_*.elf) >> "$name.out" 2>&1
        rec "$name" "F3-26/build_kernel.sh (all kernel files of F3-26 ... F3-30)" "$GCCV" \
            "F3-26/build_kernel.sh <dir> $*; then size kernel.elf u_*.elf" 0
        return 0
    fi
    rec "$name" "F3-26/build_kernel.sh" "$GCCV" "F3-26/build_kernel.sh <dir> $*" 1 "result:    BUILD FAILED"
    return 1
}

# qemu_step <name> <listing> <build dir> <cpus> <kernel command line> <expected exit> [cpu model]
# Exit status of QEMU: 1 = the kernel wrote 0 to isa-debug-exit (test passed),
# 3 = it wrote 1 (test failed or kernel panic), 124 = stopped by the time limit.
qemu_step() {
    local name="$1" listing="$2" dir="$3" cpus="$4" append="$5" expect="$6" cpu="${7:-qemu64}"
    local mods; mods="$(cd "$dir" && ls u_*.elf | tr '\n' ',' | sed 's/,$//')"
    local args=(-M pc -cpu "$cpu" -m 256M -smp "$cpus" -nographic -no-reboot -monitor none -nic none
                -device isa-debug-exit,iobase=0xf4,iosize=0x04 -kernel kernel32.elf -initrd "$mods")
    (cd "$dir" && timeout 600 $QEMU "${args[@]}" -serial "file:$name.serial" -append "test=$append")
    local rc=$?
    # keep everything from the kernel's first line on (the firmware banner is dropped)
    sed -n '/^OS303 teaching kernel/,$p' "$dir/$name.serial" | tr -d '\r' > "$name.out"
    local meaning="test passed"
    [ "$rc" = 3 ] && meaning="the kernel reported a failure or a panic"
    [ "$rc" = 124 ] && meaning="stopped by the time limit"
    local exp="expected"
    [ "$rc" != "$expect" ] && exp="NOT the expected $expect"
    rec "$name" "$listing" "$QEMUV; kernel built with $(g++ --version | head -n 1)" \
        "$QEMU ${args[*]} -serial file:$name.serial -append 'test=$append'" \
        "$rc ($meaning; $exp)" "$EMU" \
        "note:      lines printed by the firmware (SeaBIOS) before the kernel's first line are not kept"
    [ "$rc" = "$expect" ]
}

# a2l_step <name> <build dir> <addresses...>: addr2line on the kernel ELF
a2l_step() {
    local name="$1" dir="$2"; shift 2
    (cd "$dir" && addr2line -f -C -i -e kernel.elf "$@") | sed "s#$LABS/##" > "$name.out" 2>&1
    rec "$name" "kernel.elf of the build above" "$(addr2line --version | head -n 1)" \
        "addr2line -f -C -i -e kernel.elf $*" "${PIPESTATUS[0]}"
}
