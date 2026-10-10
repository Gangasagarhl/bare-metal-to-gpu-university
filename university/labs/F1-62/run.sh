#!/usr/bin/env bash
# F1-62 extra lab steps:
#   pci_tree  - the PCI tree of the build container (there is no GPU on it)
#   forensic  - copy_model.cpp on the recorded nightly job (evidence pack)
set -u
cd "$(dirname "$0")"
status=0
step() {  # step <name> <listing> <toolchain line> <hardware note or -> <command>
    local name="$1" listing="$2" tc="$3" hw="$4" cmd="$5"
    {
        echo "listing:   $listing"
        echo "toolchain: $tc"
        echo "command:   $cmd"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
        if [ "$hw" != "-" ]; then echo "hardware:  $hw"; fi
    } > "$name.log"
    bash -c "$cmd" > "$name.out" 2>&1
    local rc=$?
    echo "exit code: $rc" >> "$name.log"
    if [ "$rc" -ne 0 ]; then status=1; fi
}
GXX="$(g++ --version | head -n 1)"
LSPCI="$(lspci --version 2>&1 | head -n 1)"
step pci_tree "(tool) lspci -tv" "$LSPCI" "the build container is a virtual machine without a GPU" "lspci -tv"
step forensic "copy_model.cpp with forensic.in" "$GXX" "-" \
  "g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined copy_model.cpp -o .bin_forensic && ./.bin_forensic < forensic.in"
rm -f .bin_forensic
exit $status
