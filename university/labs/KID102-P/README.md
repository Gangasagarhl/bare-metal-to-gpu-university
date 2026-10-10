# KID102-P: practical exam folder (KID102, "times-table quiz")

Run: `university/labs/run_lab.sh university/labs/KID102-P` (runner exit code 0 in this build).
The exam paper is in `university/chapters/KID102/EXAMS.html` (section "Practical"); the
marking points are in `university/_keys/KID102.keys.html`.

Candidates receive only `times_table_start.cpp` (owner ruling B7); every other file here is for markers.

| file | role | expected result |
|---|---|---|
| `times_table_start.cpp` | the STARTING FILE handed to the candidate (five TODO parts; `times` adds on purpose) | builds; `test_times: 2 failures`; exit code 1 |
| `times_table.cpp` + `times_table.in` | reference solution; input: table 7, answers 28, 40, 63 | exit code 0; `Score: 2 out of 3` |
| `times_table_short.cpp` + `.in` | identical copy; input: table 12, then only two answers | exit code 0; `The input ended early.`; `Score: 2 out of 3` |
| `times_table_range.cpp` + `.in` | identical copy; input: 13 (outside 1 to 12) | exit code 0; `Please choose a number from 1 to 12.` |
| `forensic_table.cpp` + `.in` | evidence for the forensic question of the final exam (two planted bugs: `k < 10`, and the score declared and printed inside the quiz loop) | exit code 0; table stops at `8 x 9`; `Score: 1 out of 3` three times |
| `forensic_table_fixed.cpp` + `.in` | answer key: both bugs fixed | exit code 0; `8 x 10 = 80`; `Score: 3 out of 3` |

The specification the candidate receives (also in the exam paper):

1. Read one whole number `n`; if it is outside 1 to 12, print `Please choose a number from 1 to 12.` and end normally.
2. `int times(int a, int b)` returns a multiplied by b; a test function with at least three cases worked
   out by hand (one edge case) runs first, prints `test_times: <k> failures`, and `main` returns 1 if k > 0.
3. Print the table `n x 1 = …` to `n x 10 = …`, one line each.
4. Ask `What is n x 4? `, `What is n x 6? ` and `What is n x 9? ` (fixed, so a run can be repeated); after each answer print `Right!` or `Wrong, it is <answer>.`
5. If an answer cannot be read, print `The input ended early.` and stop asking.
6. Print `Score: <right> out of 3`.

Nothing in this folder needs hardware; everything was run on the build machine with the course flags.
