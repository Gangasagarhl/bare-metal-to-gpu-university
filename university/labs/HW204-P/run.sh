#!/bin/bash
# HW204-P extra steps, run last by university/labs/run_lab.sh (which has already built and run
# every .cpp with its .in). Each step writes <name>.out and <name>.log in the runner's format.
#  1. poll_exam_O2: compile poll_exam.cc at -O2 (no sanitizers) and disassemble it; the
#     midterm's M6 evidence (compiled only, never run).
#  2. i2c_decode_practice: the reference decoder on F1-47's recorded capture (the practice
#     capture candidates may use before the exam), to show it reproduces the chapter's decode.
#  3. capture_check: the capture handed to candidates (i2c_decode_start.in, identical to
#     i2c_decode.in) must be exactly the rows the generator (make_capture.out) printed.
set -u
CXX="${CXX:-g++}"
CXXFLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
status=0
stamp() {
    {
        echo "listing:   $1"
        echo "toolchain: $($CXX --version | head -n 1)"
        echo "command:   $2"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
    } > "$3"
}

# 1. disassembly evidence
name="poll_exam_O2"
cmd="$CXX -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror -c poll_exam.cc -o poll_exam.o; objdump -d --no-show-raw-insn -M intel -C poll_exam.o"
stamp "poll_exam.cc (compiled at -O2 and disassembled; not run)" "$cmd" "${name}.log"
if $CXX -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror -c poll_exam.cc -o ./.poll_exam.o > "${name}.out" 2>&1; then
    objdump -d --no-show-raw-insn -M intel -C ./.poll_exam.o | sed -n '/<wait_a/,$p' >> "${name}.out" 2>&1
    echo "objdump:   $(objdump --version | head -n 1)" >> "${name}.log"
    echo "exit code: 0 (compile and disassemble only)" >> "${name}.log"
else
    echo "result:    BUILD FAILED" >> "${name}.log"; status=1
fi
rm -f ./.poll_exam.o

# 2. the reference decoder on the practice capture
name="i2c_decode_practice"
cmd="$CXX $CXXFLAGS i2c_decode.cpp -o i2c_decode; ./i2c_decode < ${name}.in"
stamp "i2c_decode.cpp (run on ${name}.in, the capture of F1-47 Listing 1)" "$cmd" "${name}.log"
if $CXX $CXXFLAGS i2c_decode.cpp -o ./.bin_ref > "${name}.out" 2>&1; then
    timeout 10 ./.bin_ref < "${name}.in" > "${name}.out" 2>&1; rc=$?
    echo "exit code: $rc" >> "${name}.log"; echo "stdin:     ${name}.in" >> "${name}.log"
    [ "$rc" = 0 ] || status=1
else
    echo "result:    BUILD FAILED" >> "${name}.log"; status=1
fi
rm -f ./.bin_ref

# 3. the handed-out capture equals the generator's rows
name="capture_check"
cmd="sed -n '/^ *[0-9]* SCL /,/^ground truth/p' make_capture.out | grep -v '^ground truth' | cmp - i2c_decode_start.in; cmp i2c_decode.in i2c_decode_start.in"
stamp "make_capture.out against i2c_decode.in and i2c_decode_start.in" "$cmd" "${name}.log"
{
    if sed -n '/^ *[0-9]* SCL /,/^ground truth/p' make_capture.out | grep -v '^ground truth' | cmp - i2c_decode_start.in \
       && cmp i2c_decode.in i2c_decode_start.in; then
        echo "capture rows handed to candidates (i2c_decode_start.in) are identical to the generator's output and to i2c_decode.in: OK"
        echo "exit code: 0" >> "${name}.log"
    else
        echo "MISMATCH between make_capture.out and the .in files"
        echo "exit code: 1" >> "${name}.log"; status=1
    fi
} > "${name}.out" 2>&1
exit $status
