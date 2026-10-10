# F0-34 lab folder (KID102)

Run: `university/labs/run_lab.sh university/labs/F0-34`

| file | role in the chapter | expected result |
|---|---|---|
| `pasta.cpp` | Listing 1 (functions with no return value) | exit code 0 |
| `cups.cpp` | Listing 2 (function and its test function) | exit code 0, three PASS lines |
| `cups_broken.cpp` | test function catching a wrong formula | exit code 1, three FAIL lines and "Some tests failed." |
| `no_return.cpp` | Listing 3, deliberate mistake (`.expect-fail`) | compile fails: no return statement (-Werror=return-type) |
| `order_bug.cpp` | Listing 4, call before the function (`.expect-fail`) | compile fails: "'double_it' was not declared in this scope" |
| `sugar.cpp` | forensic evidence (parameter is a copy) | exit code 0, "After:  spoons = 2" |
| `sugar_fixed.cpp` | forensic answer key (return the new value) | exit code 0, "After:  spoons = 3" |

Note: `cups_broken` returning 1 is intended; the test function's result
becomes the program's exit code.
