# F2-14 lab folder (SP102)

Chapter: F2-14 Templates basics.

Run: `university/labs/run_lab.sh university/labs/F2-14`

Every `.cpp` is built with `g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined`; files with a `.expect-fail` marker must fail to compile.

| file | role in the chapter | expected result |
|---|---|---|
| `larger.cpp` | Listing 1, a function template | exit code 0; 8, 2.5, soup, 3 |
| `deduce_fail.cpp` + `.expect-fail` | Listing 2, conflicting deduction | compile fails: "deduced conflicting types for parameter 'T'" |
| `no_less.cpp` + `.expect-fail` | Listing 3, type without operator< | compile fails inside the template |
| `no_less_concept.cpp` + `.expect-fail` | Listing 4, the same with std::totally_ordered | compile fails at the call: "constraints not satisfied" |
| `ring_buffer.h` + `ring_test.cpp` | Listings 5 and 6, the course ring buffer and its tests (course lab) | exit code 0; "RingBuffer tests: 0 failures" |
| `ring_zero.cpp` + `.expect-fail` | Listing 7, RingBuffer<int, 0> | compile fails: static assertion |
| `ring_buffer_sabotage.h` + `ring_test_sabotage.cpp` | lab step 4 sabotage: write index ignores head_ | exit code 1; "after wrap-around the order is 3 4 5 6" fails |
| `ring_bug.h` + `rail_evening.cpp` | forensic evidence: tickets served out of order | exit code 0; "served: 105 106 103 104" |
| `run.sh` -> `nm_larger.out` | hardware section: instantiations in the object file (nm -C) | exit code 0; three W symbols |

Recorded runs (g++ 13.3.0, Linux x86_64, 2026-10-09) are the `.out` and `.log` files next to each listing. Non-zero exit codes listed above are intended (sabotage runs, sanitizer reports, an uncaught exception).
