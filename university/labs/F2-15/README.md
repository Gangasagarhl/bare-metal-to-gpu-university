# F2-15 lab folder (SP102)

Chapter: F2-15 Standard algorithms and iterators.

Run: `university/labs/run_lab.sh university/labs/F2-15`

Every `.cpp` is built with `g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined`; files with a `.expect-fail` marker must fail to compile.

| file | role in the chapter | expected result |
|---|---|---|
| `iterators.cpp` | Listing 1, one loop for two containers | exit code 0; vector contiguous 1, list contiguous 0 |
| `algorithms.cpp` | Listing 2, everyday algorithms | exit code 0; "sorted: 5 5 8 12 20" |
| `ranges.cpp` | Listing 3, ranges and projections | exit code 0; "rice is for table 7" |
| `iter_inside.cpp` | Listing 4, what an iterator is made of | exit code 0; iterator sizes 8; list neighbours not 8 bytes apart (addresses depend on the allocator) |
| `remove_fixed.cpp` | Listing 5, erase-remove and std::erase | exit code 0; size 4: 4 9 2 7 |
| `stats_lab.cpp` | Listing 6, lab reference solution | exit code 0; "statistics tests: 0 failures" |
| `stats_lab_byref.cpp` | lab step 5 sabotage: median by reference | exit code 1; 1 FAIL line |
| `remove_bug.cpp` | forensic evidence: std::remove without erase | exit code 0; "after  (size 6): 4 9 2 7 2 7" |

Recorded runs (g++ 13.3.0, Linux x86_64, 2026-10-09) are the `.out` and `.log` files next to each listing. Non-zero exit codes listed above are intended (sabotage runs, sanitizer reports, an uncaught exception).
