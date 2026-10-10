# RB101-P: practical exam folder (RB101, "program the simulated robot through a course")

Run: `university/labs/run_lab.sh university/labs/RB101-P` (runner exit code 0 in this build).
The exam paper is in `university/chapters/RB101/EXAMS.html` (section "Practical"); the
marking points are in `university/_keys/RB101.keys.html`.

Candidates receive only `course_start.cpp` and `course_start.in` (owner ruling B7); every other
file here is for markers.

The course card's practical is simulator-only ("program the simulated robot through a course"),
so nothing in this folder needs hardware. The optional supervised kit session of F9-07 Part B is
not part of the exam. Everything here was run on the build machine with the course flags
(`g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined`).

| file | role | expected result |
|---|---|---|
| `course_start.cpp` + `.in` | the STARTING FILE handed to the candidate: the grid-world simulator of F9-06 on a new 7-by-11 course, with three TODO parts (it builds and runs, but the robot drives through walls and never notices the goal, `L` turns right, and the safety limit never triggers); the input holds two short test programs, not the route | exit code 0; `FFFFF` ends "at row 1, column 6" with the wall at column 6 overwritten by `*`; `FFFFRFFLFF` ends "at row 3, column 3, facing west" |
| `course_sim.cpp` + `.in` | reference solution (all three TODOs done); input: the route, a bumping program, a program with a wrong turn, a 44-command program | exit code 0; `GOAL reached at step 31`; `BUMP at step 5 … row 1, column 6 while facing east`; `BUMP at step 9 … row 3, column 6 while facing east`; `safety limit: stopped after 40 steps, goal not reached` |
| `forensic_follower.cpp` | evidence for the forensic question of the final exam (two planted bugs: the stop is not latched, `stopped = (step == stopPressedAt)`; and the robot moves by the wished `steer`, not by the `allowed` command after the speed limit and the stop) | exit code 0; step 0 shows `allowed -3.00` but the distance drops from 8.00 to 4.00; step 2 shows `stop? yes` and `allowed 0.00` but the distance drops from 2.00 to 1.00; step 3 shows `stop? no` again |
| `forensic_follower_fixed.cpp` | answer key: both bugs fixed (latched stop; move by `allowed`) | exit code 0; distances 8.00, 5.00, 2.50, then 2.50 for ever with `stop? yes` and `allowed 0.00` from step 2 |
| `line_follower_project.cpp` + `.in` | reference solution of the course project (line follower: gentle gain 0.5, speed limit 3, latched stop, self-check of the rule, scripted run with two pushes and the stop at step 13) | exit code 0; `self-check: 0 failures`; `after 15 steps: distance -0.72, crossed the line 1 times, stop pressed` |

The course of the practical (rows 0–6 top to bottom, columns 0–10 left to right; the robot starts
on S facing east):

```
###########
#S....#...#
#.###.#.#.#
#.#...#.#.#
#.#.###.#.#
#.#.....#G#
###########
```

It has exactly one route from S to G (the corridor down column 1 is a dead end). The route is
`FFFFRFFRFFLFFLFFFFLFFFFRFFRFFFF` (31 commands); a candidate may write a turn as three turns the
other way (`LLL` for `R`), which also reaches G, at a higher step number.

The specification the candidate receives (also in the exam paper):

1. TODO 1 (sense): before every `F`, look at the map square in front of the robot instead of
   pretending it is floor; a wall gives the `BUMP` result and stops the run; `G` gives the
   `GOAL reached at step N` result.
2. TODO 2 (turn): `L` must turn the robot left (three right turns), not right.
3. TODO 3 (safety limit): a program longer than 40 commands stops after 40 steps with the result
   `safety limit: stopped after 40 steps, goal not reached`.
4. The route: write a program that takes the robot from S to G as the first line of your input
   file, traced on paper first; the result line must say `GOAL reached at step N` with N equal to
   the number of letters in the program.
5. Test evidence: the input file also holds one program that bumps and one program longer than 40
   commands; the recorded run shows all three results.

All course, step and unit numbers are the course's own simulator values (owner ruling A4).
