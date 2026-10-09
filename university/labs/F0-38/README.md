# F0-38 lab folder (KID102)

Run: `university/labs/run_lab.sh university/labs/F0-38`

| file | role in the chapter | expected result |
|---|---|---|
| `game.cpp` + `game.in` | Listing 1, the guessing game, guesses read from the file | exit code 0, "test_hint_for: 0 failures", secret 22 found in 7 guesses |
| `halving.cpp` | Listing 2, checks "at most seven guesses" | exit code 0, "Most guesses ever needed: 7" |
| `random_secret.cpp` | Listing 3, fixed seed versus `std::random_device` | exit code 0; line 1 always "Seed 2026 twice: 22 22"; line 2 differs on every run |
| `game_test_break.cpp` + `.in` | lab step 4: one test expectation changed | exit code 1, one FAIL line, the game does not start |
| `halving_1000.cpp` | lab step 5: range 1 to 1000 | exit code 0, "Most guesses ever needed: 10" |
| `game_bug.cpp` + `.in` | forensic evidence: swapped comparisons, test result ignored | exit code 0, two FAIL lines, every hint "Too low." |

Why the seed is fixed: the game's guesses come from `game.in`, and they only
fit one secret. Seed 2026 gave secret 22 with g++ 13.3.0 and its standard
library in this build, so the recorded run can be repeated line for line.
Whether another standard library gives the same secret for the same seed was
not tested (see the "Not verified" box in Layer 3 of F0-38).
