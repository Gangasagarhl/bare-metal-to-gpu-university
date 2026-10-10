#!/usr/bin/env bash
# F2-26 lab: configure, build and test the university template with CMake; show what an
# incremental rebuild does; run the template's CI script; check a freestanding object
# (curriculum P1); and produce the "works on my machine" forensic evidence with two
# real compiler versions. Each step writes <name>.out (transcript: "$ command" lines and
# what they printed) and <name>.log (run record). Build folders are deleted at the end.
set -u
cd "$(dirname "$0")" || exit 2
LAB="$(pwd)"
WORK="${LAB}/.work"
rm -rf "$WORK"; mkdir -p "$WORK"
status=0

# begin <name> <listing> <toolchain line> <command summary>
begin() {
    NAME="$1"; OUT="${LAB}/$1.out"; LOG="${LAB}/$1.log"; : > "$OUT"
    {
        echo "listing:   $2"
        echo "toolchain: $3"
        echo "command:   $4"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
    } > "$LOG"
}
# c '<command>': append "$ command" and its output to the transcript; remember the exit code
c() {
    echo "\$ $1" >> "$OUT"
    (cd "$HERE" && bash -c "$1") >> "$OUT" 2>&1
    LAST=$?
}
# end <expected exit code of the last command> [note]
end() {
    sed -i "s#${WORK}#<work>#g; s#${LAB}#<lab>#g" "$OUT"
    echo "exit code: ${LAST}" >> "$LOG"
    if [ $# -ge 2 ]; then echo "note:      $2" >> "$LOG"; fi
    if [ "$LAST" != "$1" ]; then echo "result:    UNEXPECTED (expected exit code $1)" >> "$LOG"; status=1; fi
}
GXX="$(g++ --version | head -n 1)"
CMK="$(cmake --version | head -n 1)"

# ---- 1. configure, build, rebuild, test the template (Debug) ----------------------------
cp -r template "$WORK/tpl"
HERE="$WORK/tpl"
begin configure "template/CMakeLists.txt" "$CMK; $GXX" "cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug"
c "cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug"
end 0
begin build "template/CMakeLists.txt, src/, app/, tests/" "$CMK; $GXX" "cmake --build build"
c "cmake --build build"
end 0
begin rebuild "template/src/stats.cpp" "$CMK; $GXX" "cmake --build build (no change), touch src/stats.cpp, cmake --build build"
c "cmake --build build"
sleep 1
c "touch src/stats.cpp"
c "cmake --build build"
end 0
begin compile_command "template/CMakeLists.txt" "$CMK; $GXX" "read build/compile_commands.json (entry for src/stats.cpp)"
c "python3 -c \"import json; [print(e['command']) for e in json.load(open('build/compile_commands.json')) if e['file'].endswith('stats.cpp')]\""
end 0
begin ctest "template/tests/test_stats.cpp" "$CMK; $GXX" "ctest --test-dir build --output-on-failure; build/test_stats"
c "ctest --test-dir build --output-on-failure"
c "./build/test_stats"
end 0

# ---- 1b. a project that forgot enable_testing(): what does ctest do? ----------------------
mkdir -p "$WORK/notests"
printf 'cmake_minimum_required(VERSION 3.20)\nproject(notests LANGUAGES CXX)\nadd_executable(t t.cpp)\nadd_test(NAME t COMMAND t)\n' > "$WORK/notests/CMakeLists.txt"
printf 'int main() { return 1; }\n' > "$WORK/notests/t.cpp"
HERE="$WORK/notests"
begin no_tests "(a 4-line CMakeLists.txt written by run.sh, shown in the transcript)" "$CMK; $GXX" "cmake; cmake --build; ctest; echo exit status"
c "cat CMakeLists.txt"
c "cmake -S . -B build > /dev/null && cmake --build build > /dev/null && ctest --test-dir build; echo \"ctest exit status: \$?\""
end 0 "add_test without enable_testing(): the failing test is never run"

# ---- 2. the template's own CI script, from a fresh copy ---------------------------------
rm -rf "$WORK/ci"; cp -r template "$WORK/ci"
HERE="$WORK/ci"
begin ci "template/ci.sh" "$CMK; $GXX" "./ci.sh"
c "./ci.sh"
end 0

# ---- 3. curriculum P1 check: a freestanding object for x86-64 ---------------------------
HERE="$LAB/freestanding"
begin freestanding "freestanding/kmain.cpp" "$GXX; $(readelf --version | head -n 1)" \
    "g++ -std=c++20 -ffreestanding -fno-exceptions -fno-rtti -nostdlib -O2 -c kmain.cpp; readelf -h; nm"
c "g++ -std=c++20 -Wall -Wextra -Werror -ffreestanding -fno-exceptions -fno-rtti -nostdlib -O2 -c kmain.cpp -o $WORK/kmain.o"
c "readelf -h $WORK/kmain.o | grep -E 'Class|Type|Machine'"
c "nm $WORK/kmain.o"
c "nm -u $WORK/kmain.o | wc -l"
end 0 "host g++ targeting x86_64-linux-gnu, not the x86_64-elf cross compiler of curriculum P1 (that is SP301's lab)"

# ---- 4. forensic: "works on my machine" ---------------------------------------------------
for v in dev ci ci_debug dev_release fixed; do rm -rf "$WORK/f_$v"; cp -r forensic "$WORK/f_$v"; done
HERE="$WORK/f_dev"
begin wom_dev "forensic/" "$(g++-12 --version | head -n 1); $CMK" "CXX=g++-12 cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug; cmake --build build --verbose; ./build/packer"
c "CXX=g++-12 cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug"
c "cmake --build build --verbose"
c "./build/packer"
end 0 "the developer's laptop build, reproduced in the container with g++-12"
HERE="$WORK/f_ci"
begin wom_ci "forensic/" "$GXX; $CMK" "CXX=g++-13 cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS=-Werror; cmake --build build --verbose"
c "CXX=g++-13 cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS=-Werror"
c "cmake --build build --verbose"
end 2 "the CI build; it fails, and this failure is the evidence (make returns 2)"
HERE="$WORK/f_ci_debug"
begin wom_ci_debug "forensic/" "$GXX; $CMK" "CXX=g++-13 with the developer's flags (Debug, no -Werror)"
c "CXX=g++-13 cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug > /dev/null"
c "cmake --build build 2>&1 | grep -E 'error|Error' | head -n 4"
end 0 "isolating experiment for the answer key: only the compiler changed; the grep succeeds because errors were printed"
HERE="$WORK/f_dev_release"
begin wom_dev_release "forensic/" "$(g++-12 --version | head -n 1); $CMK" "CXX=g++-12 with the CI flags (Release, -Werror)"
c "CXX=g++-12 cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS=-Werror > /dev/null"
c "cmake --build build && ./build/packer"
end 0 "isolating experiment for the answer key: only the flags changed"
HERE="$WORK/f_fixed"
begin wom_fixed "forensic/include/checksum.h + #include <cstdint>" "$GXX; $(g++-12 --version | head -n 1); $CMK" "add #include <cstdint>, then build with both compilers and the CI flags"
c "sed -i 's|#include <string>|#include <cstdint>\\n#include <string>|' include/checksum.h && head -n 3 include/checksum.h"
c "CXX=g++-13 cmake -S . -B b13 -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS=-Werror > /dev/null && cmake --build b13 > /dev/null && ./b13/packer"
c "CXX=g++-12 cmake -S . -B b12 -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS=-Werror > /dev/null && cmake --build b12 > /dev/null && ./b12/packer"
end 0

# ---- 5. CMake's own documentation for the commands used (built-in help) ----------------
HERE="$LAB"
begin cmake_help "template/CMakeLists.txt" "$CMK" "cmake --help-command <name> (first lines of each)"
for cmd in add_library add_executable target_link_libraries target_include_directories add_test enable_testing; do
    c "cmake --help-command $cmd | sed -n '1,9p'"
done
c "cmake --help-variable CMAKE_BUILD_TYPE | sed -n '1,14p'"
c "cmake --help-command add_library | sed -n '/^Interface Libraries/,/^target using/p' | sed -n '1,12p'"
c "cmake --help-command target_link_libraries | sed -n '/^Libraries and targets following/,/^used for linking/p'"
c "cmake --help-command option | sed -n '1,9p'"
c "cmake --help-variable CMAKE_EXPORT_COMPILE_COMMANDS | sed -n '1,10p'"
c "cmake --help-property PASS_REGULAR_EXPRESSION | sed -n '1,6p'"
end 0

rm -rf "$WORK"
exit $status
