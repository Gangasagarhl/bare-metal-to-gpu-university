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

## Owner rulings applied

1. **Verilog/Icarus Verilog instead of the Nand2Tetris simulator** — decided by verifier, in the spirit of rulings A2 and C1: accepted. Icarus Verilog and Yosys are real tools, not stand-ins, and their options are now checked against their own documentation (D9, D6). Chapters F1-11, F1-12, F1-13 and F1-14 now say what the book D1 specifies (chip names such as Mux, DMux, Mux4Way16, HalfAdder, FullAdder; the Hack ALU's control bits zx, nx, zy, ny, f, no and flags zr, ng), checked in D1 chapters 1–2, and that the book's simulator was not installed or used.
2. **Practical exam and project in Verilog form** — decided by verifier, with ruling B4 (exams are written in this pass by the exam writer from each chapter's "Check yourself") and B3 (default project rubric, guide 11.5): confirmed. The practical builds gates from NAND; the project is a tested 16-bit ALU with flags, in Verilog, simulated only (ruling B2: a course practical may pass without a hardware run when the record says why and the simulated evidence is complete).
3. **Analogy mappings** (sink with taps, two kinds of tap, the pass/hatch, ticket-board lamps, runner's token, prep station with a dial, waiter's bell) — ruling A3: approved; registered by the integrator in "Analogy registry additions". Each chapter keeps its "Where the analogy breaks" list.
4. **Curriculum milestone for HW102** — decided by verifier: no milestone is added. Milestones belong to the curriculum, which the units do not edit; the chapters keep "no curriculum milestone". The 4-bit ALU exhaustive test remains the course practical.
5. **Glossary** — ruling A7: "XOR (exclusive OR)" keeps linking to MA102's canonical entry `gl-xor-exclusive-or`; HW102's 44 other terms have no clashes. The glossary `source` fields now name the verified tags; the "Transistor" and "Glitch (hazard)" definitions were corrected to match the chapters.
6. **Length** — ruling A1: nothing trimmed for length.
7. **Recorded runs** — ruling A5: all labs were re-run on 2026-10-10; only the date lines of the `.log` files changed, so the recorded files were restored.
8. **Untested on hardware** — ruling C3: every chapter's lab now says what would be needed to test it on hardware (named chips or an FPGA board with datasheets, a low-voltage supply, a way to read outputs, adult supervision; for F1-15 an oscilloscope or logic analyser). Nothing is claimed as observed on hardware. Ruling C4: status "internally checked · hardware steps untested".
9. **Rulings A8 (test keys), A10 (licence), D3, D4** — not applicable: HW102 labs contain no keys, no licence placeholder and no robot image. Ruling D1 (kit): `build/KIT.md` did not exist during this pass, so the hardware notes name no kit item.

## Verification pass

Done 2026-10-10 by the Fact-Checker agent (verification pass). Dossiers: `university/_dossiers/F1-09.dossier.html` … `F1-15.dossier.html`; QA records: `university/qa/F1-09.json` … `F1-15.json`.

**Opened (shared source set, D-ids the same in every chapter):**
- D1 Nisan & Schocken, "The Elements of Computing Systems": chapters 1 "Boolean Logic", 2 "Boolean Arithmetic", 3 "Sequential Logic" (free chapter PDFs on the nand2tetris website; chapter 1 labelled 1st edition there) and the Project 01–03 pages.
- D2 Harris & Harris, "Digital Design and Computer Architecture, ARM Edition" (2015): table of contents and chapter summaries only (publisher preview); no claim rests on D2 alone.
- D3 Petzold, "Code", 2nd edition: the author's companion website (chapters 6, 8, 14, 15).
- D5 IEEE 1800-2023 (SystemVerilog, includes Verilog): catalogue page only; the text needs an IEEE sign-in and was not opened.
- D6 Yosys: built-in help of the installed 0.33 and the 0.40 web page for `abc`.
- D7 "CMOS VLSI Design 4th Ed." lecture slides on David Harris's Harvey Mudd pages: lectures 1, 2, 3, 4, 5, 11, 17. (New source.)
- D8 Steve Ward, MIT "Computation Structures" course notes: chapters 3, 4, 6, 7, 11, 12, 14. (New source.)
- D9 Icarus Verilog documentation: iverilog and vvp command-line flags. (New source; split from the old D5.)
- D4 Horowitz & Hill, "The Art of Electronics", was not opened; its claims in F1-09 are now cited to D7, and D4 was removed from the Sources.

**Corrected (main items):** the transistor definition ("three terminals" → gate controls the current between two other terminals; body terminal noted); the meaning of `vvp -n` (it makes `$stop`/Control-C a synonym for `$finish`, not "do not stop for input"); the relay sentence in F1-09 (no unsourced "smaller, faster, quieter"); "only NAND and NOR are complete" → "NAND and NOR are each complete" (F1-11); the memory/datapath sentence of F1-12; "static-1 hazard" naming removed (F1-15); F1-14's operation-set and flag-register sentences. Five former unverified boxes were resolved into notes from the opened sources (Yosys commands; D1 chip names, adders and ALU; Icarus options).

**Left unverified (boxes):** Verilog standard semantics of `pmos`/`nmos`/`supply0`/`supply1`/`z`/`x`/`!==`/`!=` (F1-09; behaviour shown by runs only); the pMOS bubble symbol (F1-09); real transistor numbers (F1-09, by design); NAND-versus-NOR speed/size and the exact gate symbol shapes (F1-10); the trapezoid mux symbol (F1-12); real ISA flag registers and the borrow convention (F1-14); real gate delays (by design) and the description of static timing analysis tools (F1-15). The overflow rule "carry into the top bit XOR carry out" was not found in an opened source; it is supported by the exhaustive 4-bit run and the 200,007-vector 16-bit run (R3, R4) and by the worked argument.

**Labs:** all re-run with `run_lab.sh`, exit code 0 for F1-09 … F1-15; expected-fail and demo listings behaved as recorded; output files identical, only log dates differed (restored per A5). All runs are simulation; untested on hardware.
