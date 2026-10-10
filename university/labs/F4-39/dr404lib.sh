# dr404lib.sh - helpers shared by the run.sh of the DR404 labs (F4-39, F4-40, F4-41).
#   rec <name> <listing> <toolchain> <command> <exit code text> [extra lines...] -> <name>.log
#   kbuild <out.elf> <sources...>          build a lab kernel (also writes <out>32.elf)
#   qrun <name> <listing> <expect> <kernel32.elf> <cpu> <mem> <append> [cputime]
#        boot it under QEMU (TCG), keep the serial output from the kernel's first line on,
#        record the QEMU exit status; with "cputime" also the host CPU time QEMU used.
D404="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
QEMU=qemu-system-x86_64
QEMU_VER="$($QEMU --version | head -n 1)"
GXX_VER="$(g++ --version | head -n 1); $(ld --version | head -n 1)"
HW_NOTE="hardware:  untested on hardware; QEMU TCG (software emulation, no KVM in the build container)"
QCOMMON="-M pc -nographic -no-reboot -monitor none -nic none -device isa-debug-exit,iobase=0xf4,iosize=4"

rec() {
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

kbuild() {
    "$D404/build_kernel.sh" "$@"
}

# QEMU exit status: 1 = the kernel wrote 0 to isa-debug-exit (pass), 3 = it wrote 1
# (fail), 0 = QEMU stopped because the machine reset (-no-reboot), 124 = time limit.
qrun() {
    local name="$1" listing="$2" expect="$3" kernel="$4" cpu="$5" mem="$6" append="$7" ct="${8:-}"
    local cmd=($QEMU $QCOMMON -cpu "$cpu" -m "$mem" -kernel "$kernel" -append "$append")
    local shown="$QEMU $QCOMMON -cpu $cpu -m $mem -kernel $(basename "$kernel") -append '$append'"
    local rc
    if [ "$ct" = cputime ]; then
        timeout 300 python3 -I "$D404/cputime.py" "${cmd[@]}" > ".$name.raw" 2>&1; rc=$?
        shown="python3 -I F4-39/cputime.py $shown"
    else
        timeout 300 "${cmd[@]}" > ".$name.raw" 2>&1; rc=$?
    fi
    # keep everything from the kernel's first line on (the firmware banner is dropped)
    tr -d '\r' < ".$name.raw" | sed -n '/DR404 /,$p' | sed '1s/^.*\(DR404 \)/\1/' > "$name.out"
    rm -f ".$name.raw"
    local meaning="the kernel reported PASS through isa-debug-exit"
    [ "$rc" = 3 ] && meaning="the kernel reported FAIL through isa-debug-exit"
    [ "$rc" = 0 ] && meaning="the machine reset (QEMU -no-reboot), no isa-debug-exit write"
    [ "$rc" = 124 ] && meaning="stopped by the time limit"
    local verdict="expected"; [ "$rc" != "$expect" ] && verdict="NOT the expected $expect"
    rec "$name" "$listing" "$QEMU_VER; kernel built with $(g++ --version | head -n 1)" "$shown" \
        "$rc ($meaning; $verdict)" "$HW_NOTE" \
        "note:      firmware (SeaBIOS) lines before the kernel's first line are not kept"
    [ "$rc" = "$expect" ]
}
