# F0-32 lab folder (KID102)

Run: `university/labs/run_lab.sh university/labs/F0-32`

| file | role in the chapter | expected result |
|---|---|---|
| `clothes.cpp` + `.in` (12) | Listing 1 | exit code 0, jumper |
| `clothes_cold.cpp`, `clothes_boundary.cpp`, `clothes_warm.cpp` + `.in` (2, 18, 25) | identical copies of Listing 1 with other inputs (worked example, lab) | exit code 0 |
| `snack.cpp` + `.in` (16 1) | Listing 2 | exit code 0 |
| `snack_word.cpp` + `.in` (16 true) | identical copy of Listing 2, troubleshooting | exit code 0, "Not yet." |
| `assign_bug.cpp` | Listing 3, deliberate mistake | compile fails (-Werror=parentheses) |
| `assign_fixed.cpp` | lab step 5, the fix | exit code 0, prints nothing |
| `no_brackets.cpp` | troubleshooting: condition without round brackets | compile fails |
| `stars.cpp`, `stars_60.cpp`, `stars_30.cpp` + `.in` (95, 60, 30) | forensic evidence (identical sources) | exit code 0 |
| `stars_fixed.cpp` + `.in` (95) | forensic answer key | exit code 0, three stars |
