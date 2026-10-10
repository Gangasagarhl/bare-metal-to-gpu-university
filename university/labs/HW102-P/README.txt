HW102 practical exam (P) - lab folder
=====================================

Run: university/labs/run_lab.sh university/labs/HW102-P   (runner exit code 0 in this build)
Paper: university/chapters/HW102/EXAMS.html, section "Practical (P)".
Marking points and reference outputs: university/_keys/HW102.keys.html.

Candidates receive ONLY these three starting files (owner ruling B7); nothing else in this
folder is handed out:
  alu4_start.v      the module alu4 to complete (seven TODO steps; the mux2 of F1-12 is given)
  full_adder.v      the gate-level full adder of F1-13/F1-14 (two half adders and an OR), given
  tb_alu4_exam.v    the exam testbench: reference model, 8 named vectors, all 2048 combinations
  As handed out the start file compiles and the testbench prints
  "FAIL: 1859 of 2056 vectors wrong": that is the expected starting state (alu4_start.out).

Reference solution and hidden evidence (Lab Engineer, not given to candidates):
  alu4_ref.v              the completed structural ALU; "ALL PASS (2056 vectors)" (alu4_exam.out)
  alu4_fault_cin.v        Part B of the paper: the reference with the adder's carry in tied to 0;
                          SUB results one too small, "FAIL: 259 of 2056" (alu4_fault_cin.out)
  alu4_fault_carry.v      re-sit variant: the carry flag not gated by arith (alu4_fault_carry.out)
  yosys_alu4.out          Yosys NAND/NOT cell count of the reference ALU (Part C of the paper)
  alu4_forensic.v         evidence for the final exam's forensic question "Dana's ALU":
  tb_alu4_forensic.v      the result multiplexer's select bits op[0] and op[2] are exchanged
                          between the tree levels; the table and per-opcode counts are the key
  inverter_bad2.v         evidence for final question F2 (pull-up gate wired to ground)
  xor_bad2.v              evidence for midterm question M8 (g2 takes b instead of t)
  mux4_bad2.v             evidence for midterm question M11 (data swapped in both first-level muxes)
  tb_inverter.v, tb_xor.v, tb_mux4_probe.v, mux2_only.v   unchanged copies of the course
                          testbenches of F1-09, F1-11 and F1-12, used on the evidence above
  cmp4.v, tb_cmp4.v       reference for final question F19 (design): compare block on the ALU's
                          SUB flags, exhaustive 256 pairs, ALL PASS
  alu_n_ref.v             course project reference (guide 11.5): a structural N-bit ALU;
  tb_alu16_project.v      16-bit run: 8 edge cases + 100000 seeded random vectors, ALL PASS
  tb_alu16_timing.v       16-bit timing report with 1 model unit per gate (-DTIMING)
  yosys_alu16.out         Yosys NAND/NOT cell count of the 16-bit reference ALU
  exam_checks.cpp         recomputes every hand-worked number in the HW102 answer keys

Every .out and .log file is written by university/labs/run_lab.sh (exam_checks.cpp) or by
run.sh in the same format. Everything here is simulation only (Icarus Verilog, Yosys):
untested on hardware; no logic chips or FPGA board were available in this build (rulings C3, D1).
