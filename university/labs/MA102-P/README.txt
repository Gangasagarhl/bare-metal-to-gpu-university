MA102 practical exam (P) - lab folder

Candidates receive ONLY the starting files the paper names (owner ruling B7):
  status_exam_start.cpp   the program to complete (four marked TODO lines)
  status_exam_start.in    the exam input: three packets, each "GSTAT TEMP" in hex
  decode_bug.out          evidence for Part C (output of a broken decoder)
Nothing else in this folder is given to candidates.

Reference solution and hidden evidence (Lab Engineer; for markers only):
  status_exam.cpp / .in   the completed program, run with the exam input
  run.sh                  runs the reference solution on one more input
                          (status_case2: the packet "AB 00" from Part A, item 4)
  decode_bug.cpp / .in    the deliberately broken decoder behind Part C
                          (ZONE mask four bits wide instead of three)
  freezer_bug.cpp         the deliberately broken program behind the final
                          exam's forensic question F27 (signed byte read as unsigned)
  freezer_fixed.cpp       the same program with the one-line fix (the key quotes its run)
  converter_ref.cpp       reference solution of the course project (converter with tests)
  exam_checks.cpp         recomputes every number in the MA102 answer keys

Every .out and .log file is written by university/labs/run_lab.sh.
