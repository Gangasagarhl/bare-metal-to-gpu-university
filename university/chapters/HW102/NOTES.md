# HW102 — Transistors to logic gates: author notes

Chapters F1-09 to F1-15. F1-09 and F1-10 are L1. F1-11 to F1-13 are L1–L2. F1-14 and F1-15 are L2.
Labs live in `university/labs/F1-09` through `university/labs/F1-15`. Each lab passes `university/labs/run_lab.sh university/labs/<ID>` (exit code 0).

## Toolchain decision (owner please confirm)

- The Nand2Tetris hardware simulator is **not installed** here. Nothing in these chapters shows Nand2Tetris HDL syntax as verified.
- All circuits are written in Verilog. They run with Icarus Verilog 12.0 (`iverilog -g2012 -Wall`, then `vvp -n`). Gate counts come from Yosys (`synth; abc -g NAND`).
- F1-09 also has a small C++ ideal-switch model.
- The practical exams and projects are set in Verilog. If the owner wants Nand2Tetris `.hdl` files instead, that needs the simulator installed, and every listing must be re-run in it.
- **All runs are simulation only.** Every log carries "untested on hardware". No breadboard or FPGA was used.

## Listings run (status as recorded in the logs)

| Chapter | Run | Status |
|---|---|---|
| F1-09 | switch_model (C++) | pass |
| F1-09 | inverter | pass |
| F1-09 | inverter_bad (nMOS in pull-up) | expected fail (z/x output) |
| F1-09 | missing_module | expected build failure ("Unknown module type") |
| F1-10 | cmos_gates (NOT/NAND/NOR) | pass |
| F1-10 | cmos_gates_bad (NOR pull-up in parallel) | expected fail |
| F1-10 | nand2_typo | demo (implicit-wire warning) |
| F1-11 | nand_only (NOT/AND/OR/NOR/XOR/XNOR from NAND) | pass |
| F1-11 | xor_good | pass, with internal nodes shown |
| F1-11 | xor_bad | expected fail |
| F1-11 | yosys_xor2 | demo: 3 NAND + 2 NOT |
| F1-11 | yosys_or2 | demo: 1 NAND + 2 NOT |
| F1-12 | mux_decoder (80 vectors) | pass |
| F1-12 | mux4_probe | pass |
| F1-12 | mux4_bad (select bits swapped) | expected fail |
| F1-13 | adders (half, full, 9-NAND full) | pass |
| F1-13 | fa_bad (cout = a&b) | expected fail |
| F1-13 | width_demo (1-bit vs integer sum) | demo |
| F1-13 | yosys_fa | demo: 7 NAND + 5 NOT |
| F1-14 | rca4 (named + exhaustive 512) | pass |
| F1-14 | rca4_bad (wrong carry wire) | expected fail: 384 of 512 wrong |
| F1-14 | alu4 (exhaustive, 2055 vectors) | pass |
| F1-14 | alu16 (200000 random vectors, seed 102) | pass |
| F1-14 | yosys_rca4 | demo: 28 NAND + 20 NOT |
| F1-15 | ripple_delay | demo: settle 9/17/33 time units at N=4/8/16 for a long carry |
| F1-15 | carry_trace | demo |
| F1-15 | glitch | demo: glitch on y during t=12–13 |
| F1-15 | sample_early (WAIT=8) | expected fail |
| F1-15 | sample_late (WAIT=20) | pass |

Gate delays in F1-15 are an abstract `#1` per NAND. They are **not** real nanoseconds.

## Unverified boxes / claims not checked against a source

All book sources (Nisan & Schocken; Harris & Harris; Petzold; Horowitz & Hill; IEEE 1364/1800 + Icarus docs; Yosys manual) are cited **by title only — not opened** (dossier gate G1 open). Unverified boxes by chapter:

- **F1-09**:
  - Transistor counts and sizes on real chips.
  - The meaning of iverilog options and the exact semantics of the `pmos`/`nmos` primitives. These are taken from behaviour observed in our runs, not from the standard.
- **F1-10**:
  - Claims that NAND is faster or smaller than NOR in CMOS (the mobility argument).
- **F1-11**:
  - Yosys/ABC internals. The counts are only what this Yosys version produced.
  - The Nand2Tetris simulator and HDL were not used.
- **F1-12**:
  - The book's chip names and interfaces (Mux, DMux, Mux4Way16, …).
- **F1-13**:
  - The book's own adder constructions.
  - "9 NAND full adder" is our construction, checked by simulation only.
- **F1-14**:
  - Real ISA flag conventions, such as the carry or borrow meaning of C after SUB on x86 vs ARM.
  - The Nand2Tetris ALU (zx/nx/zy/ny/f/no) differs from our 3-bit op-code ALU. This is said explicitly in the chapter.
- **F1-15**:
  - No real gate-delay figures.
  - The glitch/hazard explanation is checked only in our abstract-delay simulation.

## Decisions for the owner

1. Accept Verilog/iverilog as the substitute for the Nand2Tetris simulator in HW102, or install the simulator and re-verify.
2. The practical exam and project are in Verilog form (the exam builds gates from NAND; the project is a 4-bit ALU with flags). Confirm this.
3. Proposed new analogy mappings in the F1 kitchen world (not yet in the registry):
   - logic gate = sink with taps;
   - nMOS/pMOS = two kinds of tap;
   - multiplexer = the pass/hatch;
   - decoder = ticket-board lamps;
   - carry = runner's token;
   - ALU = prep station with a dial;
   - propagation delay / reading early = waiter's bell.
4. No curriculum milestone maps to HW102. Decide whether one should, for example a milestone "4-bit ALU passes exhaustive test".
5. Glossary:
   - "XOR (exclusive OR)" is not duplicated in HW102/glossary.json. HW102 chapters link to MA102's existing `gl-xor-exclusive-or` entry.
   - HW102 has 44 other terms, with no slug clashes against other courses.
