# F2-09 lab folder (SP102)

Chapter: F2-09 Classes and invariants.

Run: `university/labs/run_lab.sh university/labs/F2-09`

Every `.cpp` is built with `g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined`; files with a `.expect-fail` marker must fail to compile.

| file | role in the chapter | expected result |
|---|---|---|
| `stock_struct.cpp` | Listing 1, a struct cannot protect its rule | exit code 0; "soup: -3 of 20, rule holds: 0" |
| `stock_class.cpp` | Listing 2, the same data protected by a class | exit code 0; every line "invariant holds: 1"; impossible stock refused by the constructor |
| `stock_private.cpp` + `.expect-fail` | Listing 3, writing a private member from outside | compile fails: "'int Stock::portions_' is private within this context" |
| `strong_types.cpp` + `.expect-fail` | Listing 4, grams passed where portions are expected | compile fails: "could not convert 'flour' from 'Grams' to 'Portions'" |
| `object_size.cpp` | Listing 5, member functions do not live in the object | exit code 0; sizeof(PlainPair) = sizeof(CheckedPair) = 8 |
| `table_lab.cpp` | lab reference solution (Table class with tests) | exit code 0; "table tests: 0 failures" |
| `table_lab_broken.cpp` | lab step 5 sabotage: the seat check ignores seated guests (line 18) | exit code 1; 4 FAIL lines |
| `stock_bug.cpp` | forensic evidence: public data changed from outside | exit code 0; "soup portions=-2" at 18:30 |

Recorded runs (g++ 13.3.0, Linux x86_64, 2026-10-09) are the `.out` and `.log` files next to each listing. Non-zero exit codes listed above are intended (sabotage runs, sanitizer reports, an uncaught exception).
