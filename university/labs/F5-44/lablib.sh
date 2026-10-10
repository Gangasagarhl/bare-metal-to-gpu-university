# lablib.sh - DS403 shared helpers, sourced by the run.sh of F5-44 ... F5-47.
#   rec <name> <listing> <toolchain> <command> <exit code> [extra lines...]  writes <name>.log
#   kbuild <out.elf> <source files...>     builds the 32-bit Multiboot cluster kernel
#   hostbuild <out> <sources...>           host C++ build with the run_lab.sh flags
#   cluster <tag> <max-ms> <kernel> [<partition args>] -- <per-node command lines...>
#       starts labswitch, then one QEMU per command line (node i gets the i-th one);
#       the switch stops when every node has halted (or after <max-ms>);
#       writes <tag>_switch.txt and <tag>_n<i>.txt and the QEMU exit codes to <tag>_rc.txt
DS403="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
QEMU=qemu-system-x86_64
QEMU_VER="$($QEMU --version | head -n 1)"
GXX_VER="$(g++ --version | head -n 1)"
LD_VER="$(ld --version | head -n 1)"
HW_NOTE="hardware:  untested on hardware; QEMU 8.2.2 with TCG (no KVM), three emulated PCs on one host, network = labswitch over loopback UDP"
HOSTFLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
KFLAGS="-m32 -std=c++20 -ffreestanding -fno-exceptions -fno-rtti -fno-threadsafe-statics \
-fno-stack-protector -fno-pic -fno-builtin -mgeneral-regs-only -O2 -Wall -Wextra -Wpedantic -Werror -I$DS403"
QNODE="-machine pc -m 32M -nodefaults -display none -no-reboot -monitor none -device isa-debug-exit,iobase=0xf4,iosize=0x04"
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
    ld -m elf_i386 -nostdlib -static --no-warn-rwx-segments -T "$DS403/kernel.ld" -o "$out" "${objs[@]}" || return 1
    rm -rf "$odir"
}
hostbuild() {
    local out="$1"; shift
    g++ $HOSTFLAGS "$@" -o "$out"
}
# qemu_node <i> <base-port> <kernel> <cmdline> : the QEMU command for node i
qemu_node() {
    local i="$1" base="$2" k="$3" cl="$4" boot
    if [ -n "${NETBOOT:-}" ]; then
        printf '#!ipxe\nkernel %s %s\nboot\n' "$(basename "$k")" "$cl" > "$NETBOOT/n$i.ipxe"
        boot="-netdev user,id=b0,tftp=$NETBOOT,bootfile=n$i.ipxe -object filter-dump,id=d0,netdev=b0,file=${TAG}_boot_n$i.pcap -device virtio-net-pci,netdev=b0,mac=52:54:00:44:b0:0$i,bootindex=1"
    else
        boot="-kernel $k -append \"$cl\""
    fi
    echo "$QEMU $QNODE -serial stdio $boot \
-netdev dgram,id=c0,local.type=inet,local.host=127.0.0.1,local.port=$((base + i)),remote.type=inet,remote.host=127.0.0.1,remote.port=$((base + 10 + i)) \
-device e1000,netdev=c0,mac=52:54:00:44:03:0$i,romfile="
}
cluster() {
    local tag="$1" run_ms="$2" kernel="$3"; shift 3
    local sw=() ; while [ "$1" != "--" ]; do sw+=("$1"); shift; done; shift
    local base=$((20000 + (RANDOM % 400) * 50))
    TAG="$tag"; CLUSTER_BASE="$base"
    "$DS403/.labswitch" "${#@}" "$base" "$run_ms" "${sw[@]}" > "${tag}_switch.txt" 2>&1 &
    local swpid=$!
    sleep 0.3
    local i=0 pids=() cl
    for cl in "$@"; do
        i=$((i + 1))
        eval "timeout $(( run_ms / 1000 + 20 )) $(qemu_node "$i" "$base" "$kernel" "$cl")" > "${tag}_n$i.txt" 2>&1 &
        pids+=($!)
    done
    : > "${tag}_rc.txt"
    i=0
    for p in "${pids[@]}"; do
        i=$((i + 1)); wait "$p"; echo "n$i $?" >> "${tag}_rc.txt"
    done
    kill -TERM "$swpid" 2>/dev/null; wait "$swpid"
    sed -i 's/\r$//' "${tag}"_n*.txt
}
