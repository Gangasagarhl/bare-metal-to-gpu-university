# F0-29 lab folder (KID102)

Listings (built and run with `university/labs/run_lab.sh university/labs/F0-29`):

| file | what it is | expected result |
|---|---|---|
| `hello.cpp` | Listing 1, the first program | exit code 0 |
| `recipe.cpp` | Listing 2, a recipe printed in order | exit code 0 |
| `breakfast.cpp` | forensic lab evidence (a missing `\n` and two steps in the wrong order) | exit code 0, wrong-looking output on purpose |

Extra evidence, not run by `run_lab.sh`:

- `build_by_hand.sh` builds `hello.cpp` the short way (`g++ hello.cpp -o hello`), runs it,
  and looks at the file the compiler made with `file` and `od`. It also prints g++'s own help
  lines for `-o` and `-std=`. Command used: `bash build_by_hand.sh > build_by_hand.txt 2>&1`
  (run from this folder). The transcript is `build_by_hand.txt`. One machine-specific value
  (the BuildID hash printed by `file`) is replaced by the placeholder `<hash removed>`
  by the script itself (guide AH-25).
