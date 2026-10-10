# F0-33 lab folder (KID102)

Run: `university/labs/run_lab.sh university/labs/F0-33`

| file | role in the chapter | expected result |
|---|---|---|
| `stir.cpp` | Listing 1 (`for` loop) | exit code 0 |
| `timer.cpp` | Listing 2 (`while` loop) | exit code 0 |
| `total.cpp` | Listing 3 (running total) | exit code 0 |
| `stir_less.cpp` | trace-table row "one too few" (Listing 1 with `<`) | exit code 0 |
| `timer_zero.cpp` | trace-table row "zero times" | exit code 0 |
| `never_ends.cpp` | forensic evidence: a loop that never ends (sleeps 200 ms per pass, prints with `std::endl`) | stopped by the time limit in `never_ends.timeout` (2 s): exit code 124 |
| `never_ends_fixed.cpp` | forensic answer key | exit code 0 |

Note: the number of lines in `never_ends.out` depends on how many passes fit
into 2 seconds on the machine, so it can differ by a line or two between runs.
The chapter only relies on the pattern of the lines and on exit code 124.
