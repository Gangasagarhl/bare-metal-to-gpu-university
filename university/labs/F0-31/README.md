# F0-31 lab folder (KID102)

Run: `university/labs/run_lab.sh university/labs/F0-31`

| file | role in the chapter | expected result |
|---|---|---|
| `types.cpp` | Listing 1 | exit code 0 |
| `division.cpp` | Listing 2 | exit code 0 |
| `sizes.cpp` | Listing 3 (measurements on the build machine only) | exit code 0 |
| `name.cpp` + `name.in` ("Amara 11") | Listing 4 | exit code 0 |
| `join_bug.cpp` | Listing 5, deliberate mistake | compile fails (invalid operands to +) |
| `int_decimal.cpp` | lab step 5: Listing 1 with `int cups = 2.9;` | builds without any message, prints `cups: 2` |
| `two_names.cpp` + `two_names.in` ("Ana Maria 11") | lab step 4: identical to Listing 4, name with a space | exit code 0; age read fails |
| `no_string_include.cpp` | troubleshooting note: Listing 1 without `#include <string>` | built in this build (another header brought `std::string` in) |
| `pizza.cpp` | forensic evidence | exit code 0, "1 slices" |
| `pizza_fixed.cpp` | forensic answer key | exit code 0, "1.5 slices" |
