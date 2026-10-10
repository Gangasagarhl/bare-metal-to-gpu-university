# F2-12 lab folder (SP102)

Chapter: F2-12 References and const.

Run: `university/labs/run_lab.sh university/labs/F2-12`

Every `.cpp` is built with `g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined`; files with a `.expect-fail` marker must fail to compile.

| file | role in the chapter | expected result |
|---|---|---|
| `refs.cpp` | Listing 1, a second name and three kinds of parameter | exit code 0 |
| `copies.cpp` | Listing 2, cost of passing by value | exit code 0; 10 copies by value, 0 by const reference |
| `const_error.cpp` + `.expect-fail` | Listing 3, breaking two const promises | compile fails: "discards qualifiers" |
| `return_local.cpp` + `.expect-fail` | Listing 4, returning a reference to a local | compile fails with -Werror=return-local-addr |
| `dangling.cpp` | Listing 5 / forensic evidence: reference kept across vector growth | exit code 1; AddressSanitizer heap-use-after-free (addresses and pid vary) |
| `dangling_fixed.cpp` | Listing 6, remember the index, not the element | exit code 0 |
| `ref_size.cpp` | Listing 7, what a stored reference costs | exit code 0; sizes 4, 4, 8 |
| `bind_temporary.cpp` + `.expect-fail` | small listing in Layer 2: temporary to a non-const reference | compile fails: "cannot bind non-const lvalue reference" |
| `ref_lab.cpp` | lab reference solution | exit code 0; "reference tests: 0 failures" |
| `ref_lab_byvalue.cpp` | lab step 4 sabotage: parameter by value | exit code 1; 2 FAIL lines |

Recorded runs (g++ 13.3.0, Linux x86_64, 2026-10-09) are the `.out` and `.log` files next to each listing. Non-zero exit codes listed above are intended (sabotage runs, sanitizer reports, an uncaught exception).
