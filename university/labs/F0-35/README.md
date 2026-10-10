# F0-35 lab folder (KID102)

Run: `university/labs/run_lab.sh university/labs/F0-35`

| file | role in the chapter | expected result |
|---|---|---|
| `shopping.cpp` | Listing 1 | exit code 0 |
| `scores.cpp` | Listing 2 | exit code 0 |
| `week.cpp` | Listing 3 (`std::array`) | exit code 0 |
| `out_of_range.cpp` | Listing 4, `at(3)` on a vector of size 3 | exit code 134 (uncaught `std::out_of_range`); note the first output line is lost |
| `out_of_range_flush.cpp` | Listing 4 with `std::endl` | exit code 134; first line present |
| `int_counter.cpp` | Listing 1 with an `int` counter | compile fails (-Werror=sign-compare) |
| `array_push.cpp` | lab step 4: `push_back` on a `std::array` | compile fails (no member named push_back) |
| `no_vector_include.cpp` | troubleshooting: Listing 2 without `#include <vector>` | compile fails |
| `guests.cpp` | forensic evidence (loop stops one early) | exit code 0, three cards |
| `guests_fixed.cpp` | forensic answer key (range-based for) | exit code 0, four cards |
