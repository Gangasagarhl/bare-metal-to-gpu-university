#!/usr/bin/env bash
# F6-28 extra lab steps (each writes <name>.out and <name>.log):
#   forensic          the same model on the forensic evidence (forensic.in)
#   default_stream_help   nvcc's own description of its default-stream choices
#   default_stream_warning  nvcc's warning for a launch without a stream argument
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
NVCC="$(nvcc --version | tail -n 2 | head -n 1)"
GXX="$(g++ --version | head -n 1)"
FLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
step forensic "stream_model.cpp with forensic.in" "$GXX" "model only: no GPU involved" \
  "g++ $FLAGS stream_model.cpp -o .bin_model && ./.bin_model < forensic.in"
step default_stream_help "nvcc --help (excerpt)" "$NVCC" "-" \
  "nvcc --help | sed -n '/^--default-stream/,/Default value/p'; echo '[...]'; nvcc --help | sed -n '/^--Werror /,/^\$/p' | grep -E '^--Werror|Make warnings|default-stream-launch|explicit stream'"
step default_stream_warning "counter_reset.cu" "$NVCC" "compiled only: no GPU is needed for a compiler warning" \
  "nvcc -std=c++17 -Wdefault-stream-launch -c counter_reset.cu -o .tmp.o 2>&1 | sed 's#^.*/##'"
rm -f .bin_model .tmp.o
exit $status
