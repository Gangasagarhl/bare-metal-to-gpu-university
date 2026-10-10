HW101 practical exam (P) - lab folder
=====================================

Run: university/labs/run_lab.sh university/labs/HW101-P   (runner exit code 0 in this build)
Paper: university/chapters/HW101/EXAMS.html, section "Practical (P)".
Marking points and reference outputs: university/_keys/HW101.keys.html.

Candidates receive ONLY these two starting files (owner ruling B7); nothing else in this
folder is handed out:
  bench_exam_start.cpp   the program to complete (five functions marked TODO)
  bench_exam_start.in    the exam input (exercise values: 6 V supply, pretend 2 V LED, 20 mA max,
                         10 mA target; resistor box; 47 kilo-ohm and 100 uF timer with a 3 V
                         threshold; model meter 10 Mohm input, 1 ohm burden)
  As handed out the start file builds, prints wrong numbers and ends with
  "self-check: 4 failures" (exit code 1): that is the expected starting state.

Reference solution and hidden evidence (Lab Engineer, not given to candidates):
  bench_exam.cpp / .in   the completed program on the exam input; "self-check: 0 failures", exit 0
  run.sh                 runs the reference solution on a second input (bench_case2: 9 V, 15 mA,
                         22 kilo-ohm, 220 uF, 4.5 V threshold, 1 Mohm meter, 10 ohm burden),
                         a marking variant for re-sits
  forensic_board.cpp/.in evidence for the final exam's forensic question "The board with two
                         strangers" (the fitted values 1000, 100, 10000, 1000 ohms are the key)
  project_ref.cpp        reference solution of the course project (RC blinker): prediction sheet,
                         tolerance range and simulation (guide 11.5)
  exam_checks.cpp        recomputes every number in the HW101 answer keys

Every .out and .log file is written by university/labs/run_lab.sh (or run.sh in its format).
Nothing in this folder needs hardware; the breadboard version of the practical (Part B of the
paper) is untested on hardware: no kit was available in this build (owner ruling D1, C3).
