# F0-30 lab folder (KID102)

Run: `university/labs/run_lab.sh university/labs/F0-30`

| file | role in the chapter | expected result |
|---|---|---|
| `jars.cpp` | Listing 1 | exit code 0 |
| `swap.cpp` | Listing 2 | exit code 0 |
| `cookies.cpp` + `cookies.in` (5) | Listing 3 | exit code 0, "Now there are 17." |
| `empty_jar.cpp` | Listing 4, deliberate mistake | compile fails (uninitialized) |
| `const_change.cpp` | lab step 5: Listing 3 plus `baked = 20;` | compile fails (read-only variable) |
| `cookies_no_init.cpp` + `.in` (5) | lab step 6: Listing 3 without `= 0` | builds, exit code 0 |
| `cookies_letters.cpp` + `.in` (abc) | troubleshooting: Listing 3, identical source, given letters instead of a number | exit code 0, "Now there are 12." |
| `swap_bug.cpp` | forensic lab evidence | exit code 0, both cups 2 |
