HW201 practical exam (P), forensic evidence and project reference - lab folder
=============================================================================

Run: university/labs/run_lab.sh university/labs/HW201-P   (runner exit code 0 in this build)
Paper: university/chapters/HW201/EXAMS.html, sections "Practical (P)", "Final" (forensic
question) and "Course project".
Marking points and reference outputs: university/_keys/HW201.keys.html.

Candidates receive ONLY these two starting files (owner ruling B7); nothing else in this
folder is handed out:
  start/dishwasher.v     the module to complete (ports and four TODOs; as handed out it
                         compiles with no Icarus Verilog warning, never leaves IDLE and the
                         testbench prints "RESULT: FAIL (64 rule violations)")
  dishwasher_tb.v        the self-checking testbench: 77 cycles, 4 programmes, the rules of
                         the word description checked at every cycle; it does not look at
                         the state encoding
Candidates work in a folder that holds both files (the lint tool checks that the file name
dishwasher.v matches the module name).

Reference solution and hidden evidence (Lab Engineer, not given to candidates):
  dishwasher.v           reference FSM: 6 states, 3-bit timer, 3-bit resume register;
                         RESULT: PASS, lint clean, 9 flip-flops in the synthesis report
                         (dishwasher.out, dishwasher_lint.out, dishwasher_synth.out)
  dishwasher_start.*     the starting file run against the testbench (expected FAIL) and
                         its lint report (two unused inputs)
  seq010.v, seq010_bug.v, seq010_tb.v
                         evidence for the final exam's forensic question "The pattern found
                         only once": the correct and the broken 010 detector, the testbench
                         with its shift-register check (seq010_bug.out is the evidence shown
                         in the paper; seq010.out and seq010_synth.out are the key)
  read_me.v              the module read in final question F14; read_me_latch.out and
                         read_me_synth.out hold the synthesis tool's answer
  pc_branch.v, ram256x8.v, project_tb.v
                         reference solution of the course project (program counter with
                         relative jump, 256 x 8 memory unit, combined fetch test); project.out,
                         project_lint.out, project_synth.out, project_ram_mem.out,
                         project_ram_lut.out are its run records
  exam_checks.cpp        recomputes every number in the HW201 answer keys (exam_checks.out)

Every .out and .log file is written by university/labs/run_lab.sh (exam_checks) or by run.sh
in the same format. Toolchain as printed by the tools: Icarus Verilog 12.0, Verilator 5.020,
Yosys 0.33, g++ 13.3.0. Nothing in this folder needs hardware: the practical is simulation
only, like the course's labs (the course has no hardware steps except the optional F1-22
board lab, which is untested on hardware).
