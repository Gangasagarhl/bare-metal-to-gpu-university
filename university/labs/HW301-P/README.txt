HW301 practical exam (P) - lab folder
=====================================

Run: university/labs/run_lab.sh university/labs/HW301-P   (runner exit code 0 in this build)
Paper: university/chapters/HW301/EXAMS.html, section "Practical (P)".
Marking points and reference outputs: university/_keys/HW301.keys.html.

Candidates receive ONLY these two starting files (owner ruling B7); nothing else in this
folder is handed out:
  kernel_map_start.cpp   the program to complete (six functions marked TODO 1-6)
  kernel_map_start.in    the exam input: the documentation sheet of EX-1 (an INVENTED exam GPU,
                         exercise values under ruling A4: 24 SMs, warp 32, 48 warps / 16 blocks /
                         65,536 registers / 65,536 B shared per SM, 192-bit memory at 12 GT/s,
                         20 GB/s host link with 8 us per copy) and four kernels (stencil,
                         histogram, bigblock, tiny) with their compiler resource figures.
  As handed out the start file builds, prints zeros and ends with "self-check: 22 failures"
  (exit code 1): that is the expected starting state. The self-check compares against the
  TG-1 cases published in F1-56, F1-58, F1-59, F1-62 and F1-63, not against the exam sheet.

Reference solution and hidden evidence (Lab Engineer, not given to candidates):
  kernel_map.cpp / .in      the completed program on the exam sheet; "self-check: 0 failures", exit 0
  kernel_map_case2.in       a second sheet (EX-2: 8 CUs, wavefront 64) for re-sits; run.sh runs the
                            reference on it -> kernel_map_case2.out
  exam_checks.cpp           recomputes every number quoted in the final's and practical's answer keys
  forensic_*.in             inputs of the final exam's forensic question "The bigger GPU that changed
                            nothing" (run.sh feeds them to the course's own models in ../F1-58,
                            ../F1-56 and ../F1-62; outputs forensic_occupancy/waves/copy.out are the
                            evidence shown in the paper, forensic_fix*.out the key's fix estimates)
  project_passport_ref.txt  reference solution of the course project (hardware passport for the
                            gfx90a target, every DOC value read in the verification pass), checked
                            by ../F1-63/passport_check.cpp -> project_passport.out

Every .out and .log file is written by university/labs/run_lab.sh (or run.sh in its format).
Nothing in this folder needs a GPU. Part B of the practical (the same mapping on a reference-kit
GPU, with the device query and the compiler's resource report replacing the sheet) is untested on
hardware: the build container has no GPU (owner rulings C3, D1).
