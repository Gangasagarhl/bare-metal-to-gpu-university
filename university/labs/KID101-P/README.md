# KID101-P: practical exam folder (KID101, "the literal bean helper")

Run: `university/labs/run_lab.sh university/labs/KID101-P` (runner exit code 0 in this build).
The exam paper is in `university/chapters/KID101/EXAMS.html` (section "Practical"); the
marking points are in `university/_keys/KID101.keys.html`.

**Candidates receive only the starting files named in the paper: `beans_start.in` (their
steps file, which they edit) and `bean_helper.hpp` with `beans_start.cpp` (the helper, so a
grown-up can run their steps). Everything else in this folder (the reference solutions, the
counter-examples, the forensic evidence and its fixed version, and `key_checks.cpp`) is the
examiner's and stays hidden until marking (owner ruling B7).**

| file | role | expected result |
|---|---|---|
| `bean_helper.hpp` | the literal bean helper: the course's own tiny simulator of a helper that follows written steps word for word (like the literal cook of F0-01); gives up after 30 steps | compiled with every `.cpp` below |
| `beans_start.cpp` + `beans_start.in` | STARTING FILE handed to the candidate: situation 1 (left cup 10, right cup 0, target 5) with two example steps and no STOP | exit code 1; `ran out of steps before STOP: yes`; right cup 1 |
| `beans.cpp` + `beans.in` | reference solution, situation 1 (four steps: pick, put, `IF THE RIGHT CUP HAS 5 BEANS, STOP`, `GO BACK TO STEP 1`) | exit code 0; `task passed: yes`; 19 steps; left cup 5, right cup 5 |
| `beans_two.cpp` + `beans_two.in` | identical steps, situation 2 (left cup 10, right cup already holds 2, target 5) | exit code 0; `task passed: yes`; 11 steps; left cup 7, right cup 5 |
| `beans_straight.cpp` + `beans_straight.in` | counter-example for marking: ten straight-line steps and STOP (no loop), situation 1 | exit code 0; `task passed: yes` (it works here) |
| `beans_straight_two.cpp` + `beans_straight_two.in` | the same straight-line steps in situation 2 | exit code 1; right cup 7; `task passed: no` (this is why the paper has two situations) |
| `forensic_beans.cpp` + `forensic_beans.in` | evidence for the forensic question of the final paper: one planted bug, `GO BACK TO STEP 2` instead of `GO BACK TO STEP 1` | exit code 1; `gave up after 30 steps: yes`; right cup 1; 9 steps where nothing happened |
| `forensic_beans_fixed.cpp` + `forensic_beans_fixed.in` | answer key: the bug fixed (identical to `beans.in`) | exit code 0; `task passed: yes` |
| `key_checks.cpp` | the Lab Engineer's run behind every number in the final-exam key (bytes, letter numbers, the network trace, the toy kitchen, the fan limit, the paper adder, the card count) | exit code 0 |

The steps the helper understands (also printed in the paper): `PICK UP ONE BEAN FROM THE LEFT CUP`,
`PUT THE BEAN IN THE RIGHT CUP`, `IF THE RIGHT CUP HAS n BEANS, STOP` (or `LEFT CUP`),
`GO BACK TO STEP n`, `STOP`. Any other line is skipped with "I do not understand this step".
Exit code 0 means the task passed (STOP reached, exactly the target number of beans in the right
cup, nothing in the hand); exit code 1 means it did not. The 30-step limit and the bean counts are
exercise values (ruling A4).

Nothing in this folder needs hardware; everything was run on the build machine with the course flags.
