# F0-37 lab folder (KID102): the debugger

Toolchain in this build (printed by the tools): `g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0`,
`GNU gdb (Ubuntu 15.1-1ubuntu1~24.04.1) 15.1`, Linux x86_64 cloud build container.

## Listings run with run_lab.sh

`university/labs/run_lab.sh university/labs/F0-37` builds and runs:

| file | what it is | expected result |
|---|---|---|
| `steps.cpp` | loop that goes one step too far (`<=` instead of `<`) | exit code 134: `std::out_of_range` from `.at()` |
| `steps_fixed.cpp` | the same with `<` | exit code 0 |
| `trace.cpp` | `steps.cpp` with printed `[trace]` lines (the no-debugger method) | exit code 134 |
| `walk.cpp` | tiny program used to show `next` and `step` | exit code 0 |
| `cooking_time.cpp` | forensic lab: total is overwritten instead of added to | exit code 0, wrong total |

## The gdb sessions (real, non-interactive)

Command (run from this folder): `bash run_gdb.sh`

For each of `walk`, `steps` and `cooking_time` the script:

1. builds `<name>.cpp` with exactly the course flags:
   `g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined <name>.cpp -o <name>_dbg`
2. runs `gdb -q -batch -x <name>.gdb ./<name>_dbg` (the commands are in `<name>.gdb`;
   `set trace-commands on` makes gdb echo each command with a `+` in front);
3. saves everything gdb printed in `<name>_gdb.txt` (process ids replaced by `<pid>`);
4. deletes the temporary program.

It also saves gdb's own help text for the commands used in `gdb_help.txt`.

### Why the .gdb files contain `set environment ASAN_OPTIONS=detect_leaks=0`

The first attempt of the `cooking_time` session, made before that line was added, is kept
in `cooking_time_gdb_first_try.txt`. At the end of the program, LeakSanitizer stopped with
"LeakSanitizer does not work under ptrace (strace, gdb, etc)" and the program's own output
was lost. Leak checking is therefore switched off inside the gdb sessions only. The normal
`run_lab.sh` runs keep all sanitizers on.
