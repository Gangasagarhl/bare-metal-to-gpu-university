# F2-17 lab folder (SP102)

Chapter: F2-17 Lambdas.

Run: `university/labs/run_lab.sh university/labs/F2-17`

Every `.cpp` is built with `g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined`; files with a `.expect-fail` marker must fail to compile.

| file | role in the chapter | expected result |
|---|---|---|
| `lambdas.cpp` | Listing 1, six forms of lambda | exit code 0; "tickets: 100 101 102" |
| `closure_size.cpp` | Listing 2, closures are objects | exit code 0; sizes 1, 4, 16, 8, 32 |
| `callbacks.cpp` | Listing 3, callbacks with std::function | exit code 0; "bell rang 2 times" |
| `lambda_lab.cpp` | Listing 4, lab reference solution | exit code 0; "lambda tests: 0 failures" |
| `dangling_capture.cpp` | Listing 5, lambda outlives a by-reference capture | exit code 1; AddressSanitizer stack-use-after-return (addresses and pid vary) |
| `lambda_lab_byvalue.cpp` | lab step 4 sabotage: tally captured by value | exit code 1; 1 FAIL line |
| `price_bug.cpp` | forensic evidence: price captured by value | exit code 0; evening bills at the old price |
| `price_fixed.cpp` | forensic answer key | exit code 0; bills 18 and 12 |
| `run.sh` -> `nm_lambdas.out` | hardware section: lambda call operators (nm -C) | exit code 0; 7 symbols |

Recorded runs (g++ 13.3.0, Linux x86_64, 2026-10-09) are the `.out` and `.log` files next to each listing. Non-zero exit codes listed above are intended (sabotage runs, sanitizer reports, an uncaught exception).
