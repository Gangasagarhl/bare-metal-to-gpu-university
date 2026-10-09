#!/usr/bin/env bash
# Extra runs for F3-06: the toy file system on the forensic lab's two crash scripts.
set -u
gxx="$(g++ --version | head -n 1)"
flags="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
g++ $flags tinyfs.cpp -o .bin_tinyfs_extra || exit 1
for name in crash crash_fixed; do
    {
        echo "listing:   tinyfs.cpp with input $name.in"
        echo "toolchain: $gxx"
        echo "command:   g++ $flags tinyfs.cpp -o tinyfs && ./tinyfs < $name.in"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
    } > "$name.log"
    ./.bin_tinyfs_extra < "$name.in" > "$name.out" 2>&1; echo "exit code: $?" >> "$name.log"
    echo "stdin:     $name.in" >> "$name.log"
done
rm -f .bin_tinyfs_extra
# the system calls behind Listing 1 (plain build: no sanitizer calls mixed in)
st="$(strace -V | head -n 1)"
{
    echo "listing:   file_calls.cpp (plain build) under strace, file-related calls only"
    echo "toolchain: $st; $gxx"
    echo "command:   g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -O2 file_calls.cpp -o file_calls_plain && strace -e trace=openat,write,read,close,link,rename,unlink,newfstatat,fstat,access -o file_strace.out ./file_calls_plain > /dev/null"
    echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
} > file_strace.log
g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -O2 file_calls.cpp -o file_calls_plain || exit 1
strace -e trace=openat,write,read,close,link,rename,unlink,newfstatat,fstat,access -o file_strace.full ./file_calls_plain > /dev/null
echo "exit code: $?" >> file_strace.log
# keep only the lines after the program's own first open (the dynamic loader's lines come before)
sed -n '/timetable.txt/,$p' file_strace.full > file_strace.out
echo "trimmed:   the dynamic loader's calls before the first line that mentions timetable.txt were removed ($(wc -l < file_strace.full) lines in full)" >> file_strace.log
rm -f file_strace.full file_calls_plain
exit 0
