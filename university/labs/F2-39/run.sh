#!/usr/bin/env bash
# F2-39: ThreadSanitizer runs of the message-passing listings (release/acquire must be clean,
# relaxed must be reported), the instructions each memory order becomes on x86-64 and on
# AArch64, and an AArch64 build of the forensic program run under user-mode QEMU.
set -u
cd "$(dirname "$0")"
TC="$(g++ --version | head -n 1)"
TCA="$(aarch64-linux-gnu-g++ --version | head -n 1)"
W="-std=c++20 -Wall -Wextra -Wpedantic -Werror"
rec() {  # rec <name> <listing> <toolchain> <command> <exit code> [note] [hardware]
    {
        echo "listing:   $2"
        echo "toolchain: $3"
        echo "command:   $4"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container), $(nproc) CPUs visible"
        if [ $# -ge 7 ]; then echo "hardware:  $7"; fi
        echo "exit code: $5"
        if [ $# -ge 6 ] && [ -n "$6" ]; then echo "note:      $6"; fi
    } > "$1.log"
}
clean() { sed -i "s#$(pwd)/##g; s# (BuildId: [0-9a-f]*)##g; s#\.bin_##g" "$1"; }
status=0
# 1. message passing under ThreadSanitizer, both variants
g++ $W -O1 -g -fsanitize=thread message_passing.cpp -o .bin_mp_tsan || status=1
timeout 60 ./.bin_mp_tsan > mp_release_tsan.out 2>&1; rc=$?; clean mp_release_tsan.out
rec mp_release_tsan message_passing.cpp "$TC" "g++ $W -O1 -g -fsanitize=thread message_passing.cpp -o mp_tsan && ./mp_tsan" "$rc" \
    "release/acquire: no WARNING lines and exit code 0 = no data race detected"
timeout 60 ./.bin_mp_tsan relaxed > mp_relaxed_tsan.out 2>&1; rc=$?; clean mp_relaxed_tsan.out
rec mp_relaxed_tsan message_passing.cpp "$TC" "./mp_tsan relaxed (same build)" "$rc" \
    "relaxed/relaxed: exit code 66 is ThreadSanitizer's own after it reported a race"
# 2. relay and relaxed counter under ThreadSanitizer
for p in relay relaxed_counter; do
    g++ $W -O1 -g -fsanitize=thread $p.cpp -o .bin_$p || status=1
    timeout 60 ./.bin_$p > ${p}_tsan.out 2>&1; rc=$?; clean ${p}_tsan.out
    rec ${p}_tsan $p.cpp "$TC" "g++ $W -O1 -g -fsanitize=thread $p.cpp -o ${p}_tsan && ./${p}_tsan" "$rc" \
        "no WARNING lines and exit code 0 = no data race detected in this run"
    [ "$rc" = 0 ] || status=1
done
# 3. instructions per memory order (compiled only, never run)
filter() { grep -v -E '^\s*\.(cfi|p2align|size|type|text|globl|section|align|file|ident|weak|zero|data|bss|long|LFB|LFE)' \
           | grep -v -E '^\.(LFB|LFE|LCOLDB|LHOTB)|^_GLOBAL|^\s*$|endbr64|\.note|^\s*\.string|^flag:|^\.L[A-Z]'; }
g++ $W -O2 -S orders_asm.cc -o - | sed -n '/^_Z12storeRelaxedv:/,/^_Z11fenceSeqCstv:/p;/^_Z11fenceSeqCstv:/,/ret/p' | filter | awk '!seen[$0]++ || !/^_Z/' > orders_x86.out
rec orders_x86 orders_asm.cc "$TC" "g++ $W -O2 -S orders_asm.cc -o - (function bodies only; assembler directives removed)" 0
aarch64-linux-gnu-g++ $W -O2 -S orders_asm.cc -o - | sed -n '/^_Z12storeRelaxedv:/,/^_Z11fenceSeqCstv:/p;/^_Z11fenceSeqCstv:/,/ret/p' | filter | awk '!seen[$0]++ || !/^_Z/' > orders_arm64.out
rec orders_arm64 orders_asm.cc "$TCA" "aarch64-linux-gnu-g++ $W -O2 -S orders_asm.cc -o - (function bodies only; assembler directives removed)" 0 \
    "compiled for AArch64; not run"
# 4. forensic: the relaxed publication under ThreadSanitizer, its AArch64 code, and a QEMU user-mode run
g++ $W -O1 -g -fsanitize=thread publish_config.cc -o .bin_cfg_tsan || status=1
timeout 60 ./.bin_cfg_tsan > publish_config_tsan.out 2>&1; rc=$?; clean publish_config_tsan.out
rec publish_config_tsan publish_config.cc "$TC" "g++ $W -O1 -g -fsanitize=thread publish_config.cc -o publish_config_tsan && ./publish_config_tsan" "$rc"
aarch64-linux-gnu-g++ $W -O2 -S publish_config.cc -o - | sed -n '/^_Z14settingsThreadv:/,/ret/p' | filter > publish_config_arm64.out
rec publish_config_arm64 publish_config.cc "$TCA" "aarch64-linux-gnu-g++ $W -O2 -S publish_config.cc -o - (settingsThread only)" 0 \
    "compiled for AArch64; not run"
aarch64-linux-gnu-g++ $W -O2 -static publish_config.cc -o .bin_cfg_arm64 || status=1
timeout 60 qemu-aarch64 ./.bin_cfg_arm64 > publish_config_qemu.out 2>&1; rc=$?
rec publish_config_qemu publish_config.cc "$TCA; $(qemu-aarch64 --version | head -n 1)" \
    "aarch64-linux-gnu-g++ $W -O2 -static publish_config.cc -o cfg_arm64 && qemu-aarch64 ./cfg_arm64" "$rc" \
    "user-mode emulation on an x86-64 host: a correct-looking result here proves nothing about real Arm hardware" \
    "untested on hardware: no Arm machine was available; QEMU user mode runs the guest's threads on the x86-64 host"
rm -f .bin_*
exit $status
