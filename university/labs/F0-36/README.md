# F0-36 lab folder (KID102)

Every listing except `fixed.cpp` has an empty `<name>.expect-fail` file: the compile is
expected to fail and `run_lab.sh` saves the compiler's real messages in `<name>.out`.

| file | mistake planted | first message g++ printed (see `.out`) |
|---|---|---|
| `fixed.cpp` | none (reference version) | builds and runs, exit code 0 |
| `missing_semicolon.cpp` | no `;` at the end of line 6 | reported at line 7 |
| `misspelled.cpp` | `apple` instead of `apples` on line 8 | not declared, with a suggestion |
| `wrong_type.cpp` | text put into an `int` on line 6 | invalid conversion |
| `unused.cpp` | variable `pears` never used | warning turned into an error by `-Werror` |
| `cascade.cpp` | missing closing `"` on line 7 (forensic lab) | several messages from one mistake |

`help_flags.sh` prints g++'s own description of the warning flags used by the course;
command: `bash help_flags.sh > help_flags.txt 2>&1` (run from this folder).

Lab-step evidence (also run by `run_lab.sh`):

| file | lab step | result in this build |
|---|---|---|
| `missing_brace.cpp` | step 5: Listing 1 without the closing `}` | compile fails: "expected '}' at end of input" |
| `no_string_include.cpp` | step 6: Listing 1 without `#include <string>` | builds and runs, exit code 0 |
