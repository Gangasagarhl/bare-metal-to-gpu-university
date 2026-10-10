# F2-11 lab folder (SP102)

Chapter: F2-11 RAII: resources that clean up after themselves.

Run: `university/labs/run_lab.sh university/labs/F2-11`

Every `.cpp` is built with `g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined`; files with a `.expect-fail` marker must fail to compile.

| file | role in the chapter | expected result |
|---|---|---|
| `leak.cpp` | Listing 1, release by hand with an early return | exit code 0; open files 4 -> 7 |
| `raii_file.cpp` | Listing 2, the RAII File wrapper (course lab) | exit code 0; open files stay 4; failed open caught |
| `std_raii.cpp` | Listing 3, standard library RAII types | exit code 0; open files back to 4; mutex free again |
| `lock_fixed.cpp` | Listing 4, RAII lock owner (forensic fix) | exit code 0; "requests handled: 4 of 4" |
| `leak_until_fail.cpp` | Listing 5, a leak until the descriptor limit (set to 32) | exit code 0; "open failed after 29 leaked files: Too many open files" |
| `file_lab.cpp` | lab reference solution (File tests) | exit code 0; "File tests: 0 failures" |
| `file_lab_sabotage.cpp` | lab step 4 sabotage: destructor does not close | exit code 1; 2 FAIL lines |
| `lock_bug.cpp` | forensic evidence "The file that stays locked" | exit code 0; two requests give up; fd 3 -> menu.lock still open |

Recorded runs (g++ 13.3.0, Linux x86_64, 2026-10-09) are the `.out` and `.log` files next to each listing. Non-zero exit codes listed above are intended (sabotage runs, sanitizer reports, an uncaught exception).

The fd listing of `lock_bug` reads `/proc/self/fd`; it is Linux-only.
