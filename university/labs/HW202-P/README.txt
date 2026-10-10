HW202 practical exam (P) - lab folder
=====================================

Run: university/labs/run_lab.sh university/labs/HW202-P   (runner exit code 0 in this build)
Paper: university/chapters/HW202/EXAMS.html, section "Practical (P)".
Marking points and reference outputs: university/_keys/HW202.keys.html.

Candidates receive ONLY these two starting files (owner ruling B7); nothing else in this
folder is handed out:
  pipe_exam_start.cpp    the program to complete: a five-stage pipeline timing model written from
                         the rules of F1-26 and F1-27 on top of the U16 machine (../F1-23/u16.h).
                         Four functions marked TODO are missing (the hazard rules: registers read,
                         earliest EX cycle of a reader, the load-use test, CPI and the identity).
  pipe_exam_start.in     the exam program "range.s": the difference between the largest and the
                         smallest of four readings (14 3 27 9), to be hand-traced through the
                         pipeline and then run through the completed model.
  As handed out the start file builds, prints wrong numbers and ends its self-check (the sum
  program of F1-23, whose numbers are printed in F1-27) with "self-check: 7 failures" (exit
  code 1): that is the expected starting state.

Reference solution and hidden evidence (Lab Engineer, not given to candidates):
  pipe_exam.cpp / .in    the completed program on the exam input: "self-check: 0 failures", exit 0;
                         range.s: 42 instructions, 101 cycles / 37 stalls without forwarding,
                         69 cycles / 5 stalls with forwarding, 18 branch cycles, output 24
  pipe_case2.in          the re-sit variant (six readings 5 40 12 8 33 1, output 39), run by run.sh
  crosscheck_*.out       the course's own model (../F1-27/hazards.cpp) and the U16 computer
                         (../F1-24/cpu.cpp) on the exam program; crosscheck_diff.out shows that the
                         reference solution and the course model report identical totals
  forensic_orig.s        evidence for the final exam's forensic question "The fast loop that lies":
  forensic_fast.s        the clamp program as written (113 cycles, 6 stalls, output 236), Ravi's
  forensic_fixed.s       reordered version (107 cycles, 0 stalls, output 85: wrong) and the fixed
                         version (107 cycles, 0 stalls, output 236); *_pipe.out are the pipeline
                         diagrams, *_cpu.out the U16 computer's outputs
  project_ref.cpp        reference solution of the course project (guide 11.5): the U16 CPU extended
                         with sll (opcode 12) running hand-encoded machine-code programs (Fibonacci,
                         5 x 8 by shifting) and the regression test (the sum program still gives 29)
  exam_checks.cpp        recomputes every number in the HW202 answer keys (encodings, traces, stage
                         timings, pipeline cycle counts, predictor counts, Amdahl, interleavings)

Every .out and .log file is written by university/labs/run_lab.sh (or run.sh in its format).
Nothing in this folder needs hardware: the pipeline is the course's rule-based timing model of
F1-26 (a teaching stand-in, owner ruling A2), not a gate-level circuit or a real processor.
