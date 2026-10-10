HW203 practical exam (P) - lab folder
=====================================

Run: university/labs/run_lab.sh university/labs/HW203-P   (runner exit code 0 in this build)
Paper: university/chapters/HW203/EXAMS.html, section "Practical (P)".
Marking points and reference outputs: university/_keys/HW203.keys.html.

Candidates receive ONLY these three starting files (owner ruling B7); nothing else in this
folder is handed out:
  cache_exam_start.cpp   the program to complete (four functions marked TODO 1-4); it includes
  cache.hpp              the course's cache model from F1-33 (unchanged copy; not to be edited)
  cache_exam_start.in    the exam input: five cases "sets ways lineBytes n elemBytes tile"
  As handed out the start file builds with the course flags, prints wrong miss counts and ends
  with "self-check: 7 failures" (exit code 1): that is the expected starting state.
  (transpose_time.cc, the measurement half, is given to candidates at the start of Part 3 of the
  paper, complete; it is not a starting file to edit.)

Reference solution and hidden evidence (Lab Engineer, not given to candidates):
  cache_exam.cpp / .in         the completed program on the exam input: "self-check: 0 failures",
                               exit 0; cache_exam.out holds the miss counts the key quotes
  transpose_time.cc            the four loop nests timed on this machine (-O2, by run.sh);
                               transpose_time_ratios.out holds the ratios quoted in the key
  cache_exam_case2.in / .out   the reference solution on a second input (run.sh), a marking
                               variant for re-sits
  forensic_layout.cpp          evidence for the final's forensic question "The energy sum that
                               read eight times more" (AoS versus SoA through the course model)
  cachesim_exam_*.in / .out    the midterm's hand-trace question (M5) through F1-33 cachesim.cpp
  writepolicy_mid / _final     the write-policy questions (M10, F7) through F1-35 writepolicy.cpp
  mesi_exam.in / .out          the MESI trace question (F10) through F1-36 mesi.cpp
  tlb_exam.in / .out           the TLB trace question (F17) through F1-38 tlb_sim.cpp
  vaddr_exam.in / .out         the address-split question (F15) through F1-38 vaddr_split.cpp
  exam_checks.cpp              recomputes every other number in the HW203 answer keys
  run.sh                       runs the timing, the re-sit input and the chapter models above

Every .out and .log file is written by university/labs/run_lab.sh (or run.sh in its format).
Nothing in this folder needs special hardware. transpose_time.out is a measurement on the
machine named in its .log (a shared cloud virtual machine), one run; a rerun changes its
numbers (the keys quote the recorded run, owner ruling A5). The model outputs are deterministic.
