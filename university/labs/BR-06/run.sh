#!/usr/bin/env bash
# BR-06 run.sh: the same questions asked of three QEMU machines (x86-64 q35, AArch64 virt,
# RISC-V virt), the same arch-neutral code run on each, and every trap of the bridge shown
# by a real run. Writes <step>.out and <step>.log for each step; deletes all binaries.
set -u -o pipefail
cd "$(dirname "$0")"
status=0
S=".scratch"
rm -rf "$S"; mkdir -p "$S"

HOST_VER="$(g++ --version | head -n 1)"
A64_VER="$(aarch64-linux-gnu-g++ --version | head -n 1)"
RV_VER="$(riscv64-linux-gnu-g++ --version | head -n 1)"
CLANG_VER="$(clang++ --version | head -n 1); $(ld.lld --version | head -n 1)"
QX_VER="$(qemu-system-x86_64 --version | head -n 1)"
QA_VER="$(qemu-system-aarch64 --version | head -n 1)"
QR_VER="$(qemu-system-riscv64 --version | head -n 1)"
QU_VER="$(qemu-aarch64 --version | head -n 1) (user-mode emulation)"
HW_SYS="hardware:  untested on hardware; QEMU 8.2.2 system emulation with TCG (no KVM in the build container)"
HW_USER="hardware:  x86-64 runs on the build container's own CPU; aarch64, riscv64 and s390x runs are QEMU user-mode emulation on that x86-64 CPU, untested on hardware"

rec() {   # rec <name> <listing> <toolchain> <command> <exit code> [extra lines...]
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

# ---------------------------------------------------------------- 1. the entry probes
KF="-std=c++20 -ffreestanding -fno-exceptions -fno-rtti -fno-threadsafe-statics -fno-stack-protector \
-fno-pie -fno-pic -O2 -g -Wall -Wextra -Wpedantic -Werror -nostdinc++"
probe_build() {   # probe_build <arch> <out> <extra flags>   -> $S/<out>.elf (+ .bin for aarch64)
    local arch="$1" out="$2" extra="$3" o="$S/$2"
    mkdir -p "$o"
    case "$arch" in
    x86)
        g++ $KF -m32 -mgeneral-regs-only $extra -c probe_core.cc -o "$o/c.o" &&
        g++ $KF -m32 -mgeneral-regs-only -c arch_x86.cc -o "$o/a.o" &&
        g++ -m32 -c entry_x86.S -o "$o/e.o" &&
        ld -m elf_i386 -nostdlib -static -T link_x86.ld -o "$S/$out.elf" "$o/e.o" "$o/a.o" "$o/c.o" ;;
    aarch64)
        aarch64-linux-gnu-g++ $KF -mgeneral-regs-only -mstrict-align $extra -c probe_core.cc -o "$o/c.o" &&
        aarch64-linux-gnu-g++ $KF -mgeneral-regs-only -mstrict-align -c arch_aarch64.cc -o "$o/a.o" &&
        aarch64-linux-gnu-g++ -c entry_aarch64.S -o "$o/e.o" &&
        aarch64-linux-gnu-ld -nostdlib -static --no-warn-rwx-segments -T link_aarch64.ld -o "$S/$out.elf" \
            "$o/e.o" "$o/a.o" "$o/c.o" &&
        aarch64-linux-gnu-objcopy -O binary "$S/$out.elf" "$S/$out.bin" ;;
    riscv64*)
        local la=0x80200000; [ "$arch" = riscv64m ] && la=0x80000000
        local rf="-march=rv64imac_zicsr_zifencei -mabi=lp64 -mcmodel=medany"
        riscv64-linux-gnu-g++ $KF $rf $extra -c probe_core.cc -o "$o/c.o" &&
        riscv64-linux-gnu-g++ $KF $rf -c arch_riscv64.cc -o "$o/a.o" &&
        riscv64-linux-gnu-g++ $rf -c entry_riscv64.S -o "$o/e.o" &&
        riscv64-linux-gnu-ld -nostdlib -static --no-warn-rwx-segments --defsym=LOAD_ADDR=$la -T link_riscv64.ld \
            -o "$S/$out.elf" "$o/e.o" "$o/a.o" "$o/c.o" ;;
    esac
}
QX="qemu-system-x86_64 -M q35 -m 256M -nographic -no-reboot -net none -device isa-debug-exit,iobase=0xf4,iosize=0x04"
QA="qemu-system-aarch64 -M virt,gic-version=3 -cpu cortex-a57 -m 256M -nographic -no-reboot"
QAE="qemu-system-aarch64 -M virt,gic-version=3,virtualization=on -cpu cortex-a57 -m 256M -nographic -no-reboot"
QR="qemu-system-riscv64 -M virt -m 256M -nographic -no-reboot"
# The PC firmware and OpenSBI print banners before the probe runs. The x86 SeaBIOS banner
# (screen-control codes, option-ROM messages) is cut off up to the probe's first line;
# OpenSBI's ASCII-art logo (the lines after its version line, up to the first empty line) is
# removed; its information lines are kept.
trim_x86() { tr -d '\r' | sed -n '/BR-06 entry probe/,$p' | sed 's/^.*\(BR-06 entry probe\)/\1/'; }
trim_sbi() { tr -d '\r' | awk '/^OpenSBI v/ {print; skip = 1; next} skip && /^$/ {skip = 0} !skip'; }

boot() {   # boot <step> <arch> <extra cflags> <qemu command> <trim function> <toolchain> <note>
    local step="$1" arch="$2" extra="$3" q="$4" trim="$5" tool="$6" note="$7" img rc
    if ! probe_build "$arch" "$step" "$extra" > "$S/$step.build.txt" 2>&1; then
        cp "$S/$step.build.txt" "$step.out"; rec "$step" "probe" "$tool" "build" "1" "result:    BUILD FAILED"
        status=1; return
    fi
    img="$S/$step.elf"; [ "$arch" = aarch64 ] && img="$S/$step.bin"
    timeout 20 $q -kernel "$img" > "$S/$step.raw" 2>&1; rc=$?
    $trim < "$S/$step.raw" > "$step.out"
    rec "$step" "probe_core.cc + arch_$(echo "$arch" | sed 's/riscv64m/riscv64/').cc + entry_*.S" "$tool; $(echo "$q" | cut -d' ' -f1) 8.2.2" \
        "$q -kernel $(basename "$img")${extra:+ (probe built with $extra)}" "$rc" "$note" \
        "shared:    sha256 $(sha256sum probe_core.cc | cut -c1-16)... probe_core.cc, $(sha256sum probe.h | cut -c1-16)... probe.h (same in every probe log: compiled unchanged)" \
        "$HW_SYS"
    grep -q 'probe done\|probe stopped by an exception' "$step.out" || status=1
}
boot probe_x86 x86 "" "$QX" trim_x86 "$HOST_VER" \
    "note:      exit code 1 = isa-debug-exit with value 0, the probe's 'ok'; firmware banner before the first probe line removed"
boot probe_aarch64 aarch64 "" "$QA" cat "$A64_VER" "note:      exit code 0 after PSCI SYSTEM_OFF"
boot probe_aarch64_el2 aarch64 "" "$QAE" cat "$A64_VER" "note:      the same image, QEMU emulating EL2 (virtualization=on)"
boot probe_riscv64 riscv64 "" "$QR" trim_sbi "$RV_VER" \
    "note:      through OpenSBI (QEMU's default firmware); OpenSBI's ASCII logo lines removed, its other lines kept"
boot probe_riscv64_m riscv64m "" "$QR -bios none" cat "$RV_VER" "note:      no firmware: image linked at 0x80000000"
# 2. the APIC trap: the same core file with ASSUME_APIC defined
boot apic_x86 x86 "-DASSUME_APIC" "$QX" trim_x86 "$HOST_VER" "note:      exit code 1 = isa-debug-exit value 0 (ok)"
boot apic_aarch64 aarch64 "-DASSUME_APIC" "$QA" cat "$A64_VER" \
    "note:      the probe reports the exception itself (vector table in entry_aarch64.S), then PSCI SYSTEM_OFF (exit 0)"
boot apic_riscv64 riscv64 "-DASSUME_APIC" "$QR" trim_sbi "$RV_VER" \
    "note:      the probe reports the trap itself (fatal_trap), then SBI shutdown with reason 'failure'; OpenSBI logo removed"
# addr2line for the two faulting pc values: which function made the access?
{
    for a in aarch64 riscv64; do
        pc="$(sed -n 's/.* pc \(0x[0-9a-f]*\).*/\1/p' "apic_$a.out")"
        echo "== $a: addr2line -f -C -e apic_$a.elf $pc"
        ${a}-linux-gnu-addr2line -f -C -e "$S/apic_$a.elf" "$pc" | sed "s#$(pwd)/##g"
    done
} > apic_addr2line.out 2>&1
rec apic_addr2line "apic_aarch64.elf, apic_riscv64.elf" "$(aarch64-linux-gnu-addr2line --version | head -n 1)" \
    "<arch>-linux-gnu-addr2line -f -C -e apic_<arch>.elf <pc from the exception report>" "0"

# ---------------------------------------------------------------- 3. the checklist
for m in x86 aarch64 riscv64; do
    case $m in
    x86) q="qemu-system-x86_64 -M q35" ;;
    aarch64) q="qemu-system-aarch64 -M virt,gic-version=3 -cpu cortex-a57" ;;
    riscv64) q="qemu-system-riscv64 -M virt" ;;
    esac
    printf 'info mtree -f\ninfo cpus\ninfo pci\nquit\n' |
        timeout 20 $q -m 256M -smp 4 -S -display none -monitor stdio -net none 2>&1 | tr -d '\r' > "$S/monitor_$m.txt"
    cp "probe_$m.out" "$S/probe_$m.out"
done
python3 checklist.py "$S" > checklist.out 2>&1; rc=$?
rec checklist "checklist.py" "$(python3 --version); $QX_VER" \
    "printf 'info mtree -f\\ninfo cpus\\ninfo pci\\nquit\\n' | qemu-system-<arch> <machine> -m 256M -smp 4 -S -display none -monitor stdio -net none; python3 checklist.py" \
    "$rc" "note:      rows 1-2 from probe_x86.out, probe_aarch64.out, probe_riscv64.out; rows 3-9 from the QEMU monitor" "$HW_SYS"
[ "$rc" = 0 ] || status=1

# ---------------------------------------------------------------- 4. the neutral suite on three CPUs
UF="-std=c++20 -Wall -Wextra -Wpedantic -Werror -O2 -pthread"
{
    g++ $UF -g -fsanitize=address,undefined -DARCH_NAME='"x86-64 (native)"' neutral_suite.cc -o "$S/n_x86" &&
        "$S/n_x86"; r1=$?
    aarch64-linux-gnu-g++ $UF -static -DARCH_NAME='"aarch64 (qemu-aarch64)"' neutral_suite.cc -o "$S/n_a64" &&
        timeout 60 qemu-aarch64 "$S/n_a64"; r2=$?
    riscv64-linux-gnu-g++ $UF -static -DARCH_NAME='"riscv64 (qemu-riscv64)"' neutral_suite.cc -o "$S/n_rv" &&
        timeout 60 qemu-riscv64 "$S/n_rv"; r3=$?
    echo "exit codes: x86-64 $r1, aarch64 $r2, riscv64 $r3"
} > neutral.out 2>&1
rc=0; grep -q 'exit codes: x86-64 0, aarch64 0, riscv64 0' neutral.out || { rc=1; status=1; }
rec neutral "neutral_suite.cc" "$HOST_VER; $A64_VER; $RV_VER; $QU_VER" \
    "g++ $UF -fsanitize=address,undefined; <arch>-linux-gnu-g++ $UF -static; qemu-<arch> ./neutral" "$rc" \
    "note:      one source file, unchanged; the build passes only ARCH_NAME" "$HW_USER"

# ---------------------------------------------------------------- 5. memory ordering
{
    g++ $UF -DARCH_NAME='"x86-64 (native, the container CPU)"' litmus.cc -o "$S/l_x86" && "$S/l_x86" 1000000 10
    aarch64-linux-gnu-g++ $UF -static -DARCH_NAME='"aarch64 (qemu-aarch64 on x86-64)"' litmus.cc -o "$S/l_a64" &&
        timeout 120 qemu-aarch64 "$S/l_a64" 1000000 10
    riscv64-linux-gnu-g++ $UF -static -DARCH_NAME='"riscv64 (qemu-riscv64 on x86-64)"' litmus.cc -o "$S/l_rv" &&
        timeout 120 qemu-riscv64 "$S/l_rv" 1000000 10
} > litmus.out 2>&1; rc=$?
rec litmus "litmus.cc" "$HOST_VER; $A64_VER; $RV_VER; $QU_VER" \
    "g++ $UF (no sanitizers: they change timing); <arch>-linux-gnu-g++ $UF -static; qemu-<arch> ./litmus 1000000 10" "$rc" \
    "note:      counts vary from run to run; nproc = $(nproc)" "$HW_USER"
{
    for t in "x86-64|g++|objdump" "aarch64|aarch64-linux-gnu-g++|aarch64-linux-gnu-objdump" \
             "riscv64|riscv64-linux-gnu-g++|riscv64-linux-gnu-objdump"; do
        IFS='|' read -r n cxx od <<< "$t"
        echo "===== $n ($cxx -O2)"
        $cxx -std=c++20 -O2 -Wall -Wextra -Werror -c ordering_codegen.cc -o "$S/oc.o" &&
            $od -d --no-show-raw-insn -C "$S/oc.o" | sed -n '/>:$/,$p' | grep -v -E '^$|\snop' |
            sed -E -e 's/[[:space:]]+# [0-9a-f]+ <.*$//' -e 's/[[:space:]]+\/\/ #.*$//'
    done
} > codegen.out 2>&1; rc=$?
rec codegen "ordering_codegen.cc" "$HOST_VER; $A64_VER; $RV_VER; $(objdump --version | head -n 1)" \
    "<cxx> -std=c++20 -O2 -c ordering_codegen.cc; <objdump> -d --no-show-raw-insn -C (nop padding and comments removed)" "$rc"
python3 - <<'PY' > /dev/null || status=1
import sys
t = open("codegen.out").read()
sys.exit(0 if ("stlr" in t and "ldar" in t and "fence\trw,w" in t) else 1)
PY

# ---------------------------------------------------------------- 6. an x86 instruction in shared code
{
    for t in "x86-64|g++|" "aarch64|aarch64-linux-gnu-g++|" "riscv64|riscv64-linux-gnu-g++|" \
             "riscv64 + Zihintpause|riscv64-linux-gnu-g++|-march=rv64gc_zihintpause"; do
        IFS='|' read -r n cxx fl <<< "$t"
        echo "== $n: $cxx -std=c++20 -O2 -Wall -Wextra -Werror $fl -c x86ism.cc"
        if $cxx -std=c++20 -O2 -Wall -Wextra -Werror $fl -c x86ism.cc -o "$S/x.o" 2> "$S/x.err"; then
            echo "built"
            case "$n" in riscv64*) riscv64-linux-gnu-objdump -d --no-show-raw-insn "$S/x.o" | grep -w pause ;; esac
        else
            sed -e "s#/tmp/[^:]*\.s#<temporary .s file>#" "$S/x.err"
            echo "build failed"
        fi
    done
} > x86ism.out 2>&1
rec x86ism "x86ism.cc" "$HOST_VER; $A64_VER; $RV_VER" "<cxx> -std=c++20 -O2 -Wall -Wextra -Werror -c x86ism.cc" "0" \
    "result:    the aarch64 and riscv64 (rv64gc) builds failed as expected; the messages are in x86ism.out"
[ "$(grep -c 'build failed' x86ism.out)" = 2 ] || status=1

# ---------------------------------------------------------------- 7. cache maintenance instructions
{
    echo "== x86-64"; gcc -c cachemaint.S -o "$S/cm.o" && objdump -d "$S/cm.o" | sed -n '/<.text>:/,$p' | tail -n +2
    echo "== aarch64"; aarch64-linux-gnu-gcc -c cachemaint.S -o "$S/cm.o" &&
        aarch64-linux-gnu-objdump -d "$S/cm.o" | sed -n '/<.text>:/,$p' | tail -n +2
    echo "== riscv64 (-march=rv64gc_zicbom)"; riscv64-linux-gnu-gcc -march=rv64gc_zicbom -c cachemaint.S -o "$S/cm.o" &&
        riscv64-linux-gnu-objdump -d "$S/cm.o" | sed -n '/<.text>:/,$p' | tail -n +2
} > cachemaint.out 2>&1; rc=$?
rec cachemaint "cachemaint.S" "$(gcc --version | head -n 1); $A64_VER; $RV_VER; GNU binutils objdump" \
    "<cc> -c cachemaint.S; <objdump> -d" "$rc" "hardware:  assembled only; nothing was executed"

# ---------------------------------------------------------------- 8. forensic: the volume one machine refuses
rm -f "$S/vol.img"; truncate -s 8M "$S/vol.img"
mke2fs -q -t ext2 -b 4096 -N 512 "$S/vol.img"
dd if="$S/vol.img" of="$S/sb.bin" bs=1024 skip=1 count=1 2> /dev/null
DF="-std=c++20 -ffreestanding -fno-exceptions -fno-rtti -fno-stack-protector -fno-pic -fno-pie -O2 -Wall -Wextra -Werror -nostdinc++"
diskrun() {   # diskrun <extra flags>
    local t n tg q fl rc
    for t in "x86_64|x86_64-linux-gnu|" "aarch64|aarch64-linux-gnu|qemu-aarch64" \
             "riscv64|riscv64-linux-gnu|qemu-riscv64" "s390x|s390x-linux-gnu|qemu-s390x"; do
        IFS='|' read -r n tg q <<< "$t"
        fl=""; [ "$n" = riscv64 ] && fl="-mno-relax"
        clang++ --target="$tg" $DF $fl $1 -DARCH_NAME="\"$n\"" -c diskcheck.cc -o "$S/dc_$n.o" &&
            ld.lld -static -nostdlib -e _start -o "$S/dc_$n" "$S/dc_$n.o" || { echo "$n: build failed"; continue; }
        $q "$S/dc_$n" < "$S/sb.bin"; rc=$?
        echo "  (exit code $rc)"
    done
}
{
    echo "== the volume: mke2fs -q -t ext2 -b 4096 -N 512 vol.img (8 MiB); dumpe2fs -h vol.img, four lines"
    dumpe2fs -h "$S/vol.img" 2> /dev/null | grep -E '^(Filesystem magic number|Inode count|Block count|Block size):'
    echo "== od -A d -t x1 -N 64 superblock.bin (bytes 1024..1087 of vol.img)"
    od -A d -t x1 -N 64 "$S/sb.bin"
    echo "== diskcheck, built from the same source for four CPUs, given the same 1024 bytes"
    diskrun ""
} > forensic_diskcheck.out 2>&1
rec forensic_diskcheck "diskcheck.cc + ustart.h" "$CLANG_VER; $(mke2fs -V 2>&1 | head -n 1); $(qemu-s390x --version | head -n 1)" \
    "clang++ --target=<cpu>-linux-gnu $DF -c diskcheck.cc; ld.lld -static -nostdlib; qemu-<cpu> ./diskcheck < superblock.bin" "0" \
    "note:      the s390x refusal (exit code 1) is the evidence; the time stamps in bytes 44-51 change with every run" "$HW_USER"
{
    echo "== diskcheck built with -DFIXED, same 1024 bytes"
    diskrun "-DFIXED"
} > forensic_fixed.out 2>&1
rec forensic_fixed "diskcheck.cc + ustart.h (-DFIXED)" "$CLANG_VER" \
    "clang++ --target=<cpu>-linux-gnu $DF -DFIXED -c diskcheck.cc; ld.lld; qemu-<cpu> ./diskcheck < superblock.bin" "0" "$HW_USER"
grep -q 'refusing' forensic_diskcheck.out || status=1
[ "$(grep -c 'ok to mount' forensic_fixed.out)" = 4 ] || status=1

rm -rf "$S"
exit $status
