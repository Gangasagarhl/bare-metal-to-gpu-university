# F2-16 lab folder (SP102)

Chapter: F2-16 Error handling: exceptions, error codes, std::optional.

Run: `university/labs/run_lab.sh university/labs/F2-16`

Every `.cpp` is built with `g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined`; files with a `.expect-fail` marker must fail to compile.

| file | role in the chapter | expected result |
|---|---|---|
| `three_ways.cpp` | Listing 1, error code, exception, optional | exit code 0 |
| `unwinding.cpp` | Listing 2, stack unwinding | exit code 0; release pan, release stove, then caught in main |
| `uncaught.cpp` | Listing 3, nobody catches | exit code 134 by design; "terminate called after throwing ..." |
| `nodiscard.cpp` + `.expect-fail` | Listing 4, ignored [[nodiscard]] result | compile fails with -Werror=unused-result |
| `expected23.cc` (run.sh) | Listing 5, std::expected, built with -std=c++23 | exit code 0; "value 12" / "error not a number: \"l2\"" |
| `no_exceptions.cc` (run.sh) | Listing 6, built with -fno-exceptions | compile fails: "exception handling disabled" |
| `run.sh` -> `fry_calls.out` | hardware section: calls in fry() (objdump) | exit code 0; __cxa_allocate_exception, __cxa_throw, _Unwind_Resume |
| `error_lab.cpp` | Listing 7, lab reference solution | exit code 0; "error handling tests: 0 failures" |
| `error_lab_norange.cpp` | lab step 5 sabotage: range check removed | exit code 1; 2 FAIL lines |
| `bill_bug.cpp` | forensic evidence: value_or(0) hides a failure | exit code 0; "0 x dumplings", total 14 |
| `bill_fixed.cpp` | forensic answer key | exit code 1 by design; bill not printed |

Recorded runs (g++ 13.3.0, Linux x86_64, 2026-10-09) are the `.out` and `.log` files next to each listing. Non-zero exit codes listed above are intended (sabotage runs, sanitizer reports, an uncaught exception).

The `.cc` files are built only by `run.sh` (run_lab.sh builds `*.cpp`), because they need other flags.
