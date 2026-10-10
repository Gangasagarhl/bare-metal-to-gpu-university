# HW201 — Sequential logic and digital design: author notes

Chapters F1-16 to F1-22, all L2. F1-22 (FPGAs) is optional.
Labs are in `university/labs/F1-16` to `university/labs/F1-22`.
Each lab passes `university/labs/run_lab.sh university/labs/<ID>` with exit code 0. All seven were re-run at the end of this build, on 2026-10-09.

## Toolchain (as recorded in the logs)

- **Icarus Verilog version 12.0 (stable).** Designs are compiled with `iverilog -g2005 -Wall`. Each lab's `run.sh` treats any compiler warning as a failure, except in forensic evidence, where the warning is the evidence.
- **Yosys 0.33 (git sha1 2584903a060).** Used for synthesis reports and generic 4-input LUT mapping (`synth -lut 4`).
- **Verilator 5.020 2024-01-01 rev (Debian 5.020-1).** Lint only, in F1-21 and F1-22. Verilator prints lines that point to its web documentation; those lines are filtered out, and each output says so.
- **g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0.** Uses `-std=c++20 -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined`.
- **Python 3.** Runs `wave.py`, which turns a VCD file into a text timing diagram. There is one copy in each lab that draws waveforms.
- The SVG waveforms in F1-16 (Fig. 2), F1-17 (Fig. 3) and F1-18 (Fig. 3) were generated from the real VCD files of these runs. The bar chart in F1-20 (Fig. 3) was drawn from the printed values of `dram_refresh`.

## Listings run

**All runs are simulation or tool runs. Nothing was run on real hardware.**

| Chapter | Run | Status |
|---|---|---|
| F1-16 | sr_latch | demo, exit 0 (NOR SR latch, including the forbidden input) |
| F1-16 | latch_vs_ff (+ _wave) | pass |
| F1-16 | mux_bug | expected fail (inferred latch; forensic "The selector that remembers") |
| F1-16 | mux_bug_yosys | demo: "Latch inferred" in the Yosys log |
| F1-16 | mux_fixed | pass |
| F1-16 | mux_fixed_yosys | demo: 0 latches |
| F1-17 | regs (register, counter, shift register) | pass |
| F1-17 | ripple | demo: in-between values of a ripple counter |
| F1-17 | display_good (+ _wave) | pass |
| F1-17 | display_skip (+ _wave) | expected fail (`always @(clk)` counts on both edges; forensic "The counter skips") |
| F1-18 | clock_math (C++) | exit 0 |
| F1-18 | divider (+ _wave) | pass |
| F1-18 | clock_ok (PERIOD=10, + _wave) | pass |
| F1-18 | clock_fast (PERIOD=6, + _wave) | expected fail (adder slower than the period; forensic "Fast clock, slow count") |
| F1-19 | seq101 | pass |
| F1-19 | traffic | pass (3 safety rules × 40 cycles) |
| F1-19 | traffic_bug | expected fail (2-bit state register for 6 states; the compiler width warning is part of the evidence; forensic "The yellow that never came") |
| F1-20 | sram_cell (switch-level, weak/strong drives) | pass |
| F1-20 | ram (16 × 8) | pass |
| F1-20 | dram_refresh (C++ model) | exit 0: A loses 8 rows, B loses 0 |
| F1-20 | dram_bug (C++) | expected bad result: 2-bit refresh counter, 4 rows lost (forensic "The forgetful upper half") |
| F1-21 | pc | pass |
| F1-21 | pc_lint | no warnings |
| F1-21 | pc_synth | 33 cells, 8 `$_SDFFE_PP0P_` |
| F1-21 | lint_me | demo: WIDTHEXPAND + UNUSEDSIGNAL; Verilator exit 1 is recorded on purpose (the step itself exits 0) |
| F1-21 | delay_good | pass |
| F1-21 | delay_bug | expected fail (blocking `=` in a clocked block; forensic "The delay line that does not delay") |
| F1-21 | delay_synth | 12 vs 4 `$_DFF_P_` |
| F1-21 | delay_bug_lint | demo: 3 × BLKSEQ; Verilator exit 1 recorded on purpose |
| F1-22 | lut4 (C++) | pass |
| F1-22 | counter_lut_stat / counter_lut_netlist | 4 FF + 6 LUT; netlist shows `16'h7f80 >> count` |
| F1-22 | decade_sim, decade_big_sim | both pass (forensic "The counter that grew") |
| F1-22 | decade_lut | 4 FF + 6 LUT vs 32 FF + 64 LUT |
| F1-22 | board_sim / board_lint / board_lut | pass / no warnings / 36 FF + 69 LUT |
| F1-22 | **board lab Part B** | **UNTESTED ON HARDWARE** (no FPGA board in this build) |

The answers to some "Predict, then run" questions were checked with extra runs in a scratch directory. These runs are not part of the labs.

| Chapter | Question | Result of the check run |
|---|---|---|
| F1-17 | Q7 | Verified by simulation |
| F1-18 | Period 8 | PASS |
| F1-19 | Q7 (no cars) | PASS |
| F1-20 | Q4 (strong inverters) | FAIL, x everywhere |
| F1-20 | Q7 (refresh only on even ticks) | B = 8 rows lost |
| F1-21 | Q6 (reversed blocking order) | PASS |
| F1-21 | Q7 (WIDTH = 4) | 4 FF, 15 cells |
| F1-22 | Q7 (8-bit counter) | 8 FF, 12 LUT |

## Unverified boxes (no source was opened in this build)

- **F1-16.** Real setup time, hold time, clock-to-Q delay and metastability figures are part-specific. No numbers are given. Also: the simulator oscillates or resolves deterministically where real hardware may go metastable (stated as a model caveat).
- **F1-17.** How long a real ripple counter's in-between values last depends on the part. The delays in the listings are exercise values.
- **F1-18.** How real boards make clocks (crystal, PLL, spread spectrum) and every number about it. The periods in the listings are exercise values.
- **F1-19.** Real traffic signals follow national rules and safety standards (see SS402). The timings in the chapter are made up.
- **F1-20.** The 6T / 1T1C cell descriptions are textbook-level. Refresh intervals, retention, timings and protection against row disturbance come from the JEDEC standards and part data sheets. No numbers are given. 95 % per tick, the 0.5 threshold and 8 rows are exercise values.
- **F1-21.** The letter-by-letter reading of `$_SDFFE_PP0P_` is from memory of the Yosys manual's naming scheme. The cell counts themselves are real.
- **F1-22.** All of the following come from textbooks only: place and route, timing reports, bitstreams, constraint formats, programming, block RAM behaviour, and real logic-block structure. The LUT mapping used is Yosys's generic `-lut 4`, not a device mapping. **Part B is untested on hardware.**

Every named book is cited as "Title only — not opened during this build" (gate G1 open). These are:
- Harris & Harris, "Digital Design and Computer Architecture";
- Nisan & Schocken, "The Elements of Computing Systems";
- Patterson & Hennessy, "Computer Organization and Design";
- Petzold, "Code";
- IEEE Std 1364;
- the Yosys, Verilator and Icarus Verilog manuals.

Source ids are per chapter: D3 and D4 mean different works in different chapters. Each chapter's Sources list is authoritative.

## Analogy proposals for the Dean (F1 kitchen world)

These mappings are new and need approval for the analogy table:

| Concept | Kitchen mapping | Chapter |
|---|---|---|
| Latch | The serving hatch: open while the hatch is up | F1-16 |
| Flip-flop | A ticket clip on the rail, swapped only when the bell rings | F1-16 |
| Clock | The pass bell rung by the expediter (Rosa) | F1-18 |
| Register | A row of clips | F1-17 |
| Counter | The "take a number" display | F1-17 |
| Shift register | The plate conveyor | F1-17 |
| FSM | The order status card plus the expediter's rule card | F1-19 |
| SRAM | A flip sign that stays set while the lights are on | F1-20 |
| DRAM | Pantry jars that evaporate; refresh = the porter (Sam) topping up row by row | F1-20 |
| HDL | A floor plan, not a recipe | F1-21 |
| FPGA | The event hall of movable stations | F1-22 |
| LUT | A station's answer card | F1-22 |

Named characters used: Ana (head chef), Rosa (expediter: rings the pass bell in F1-18, keeps the status board in F1-19), Sam (porter).

Forensic authors: Kofi (F1-19), Lena (F1-20), Priya (F1-21), Omar (F1-22), plus those in F1-16 to F1-18.

## Glossary: overlaps for the Integrator

- `glossary.json` has 71 entries.
- **Builder rule:** the builder merges entries by exact term, and course folders load alphabetically. So for a duplicate term, the first course's definition wins and the chapter lists are merged.
- **Terms that already exist in an earlier course.** I kept entries for these so that my chapter ids merge into their chapter lists; the earlier definition wins:
  - Combinational circuit, Critical path, Hardware description language (HDL), Logic simulator, testbench, Netlist, Synthesis tool and Timing diagram (HW102);
  - Oscillator (HW101);
  - Wrap-around (MA102).
- **Terms owned by HW202:** Register (CPU), Program counter (PC), Register file.
  - My chapters link to their anchors, but I deliberately wrote **no** entry. HW201 loads before HW202, so an entry of mine would replace HW202's definition.
  - As a result, F1-17, F1-20 and F1-21 do not appear in those entries' chapter lists. The Integrator may add them.
- **Qualified terms, to avoid clashes:**
  - "Counter (hardware)": KID102's "Counter" is a programming loop variable.
  - "Register (hardware)": RB101 / KID103 use "Register" in another sense.
  - "Feedback (in a circuit)".
  - "Module (Verilog)", "Port (Verilog)", "Parameter (Verilog)".
- **Anchor slugs:** glossary links in all seven chapters now use the slug without a trailing dash, as FRAGMENT_FORMAT.md section 7 requires.

## Decisions for the owner

1. **FPGA board for F1-22 Part B.**
   - No board, vendor or product is named.
   - Choosing one requires a low-voltage, USB-powered board with at least 4 LEDs and 1 button, and its toolchain.
   - Then someone must run Part B and replace the "untested on hardware" box with the recorded run. `board_top.v` defaults to `DIV = 4`, which suits simulation only, and uses a 32-bit `ticks` counter that the lab asks students to size.
2. **Verilog standard edition (IEEE 1364).** The edition is unconfirmed; the Source Researcher must confirm which one is cited. The labs compile with `-g2005`.
3. **Curriculum milestone.** HW201 has no curriculum milestone. It maps only to "Prepares HW202" (curriculum 1.3). Please confirm that this is intended.
4. **Course project split.**
   - The PC is built in F1-17 and reviewed and extended (relative jump) in F1-21.
   - The memory unit is built in F1-20.
   - The F1-22 mini-project adds a resource budget.
   - Please confirm this split matches the HW202 integration plan, and that its 8-bit addresses and 8-bit words match HW202's CPU. HW202's U16 uses 16-bit registers, so the widths may need to change: `program_counter.v` has a WIDTH parameter, but the RAM is fixed at 8-bit words.
5. **Verilator output filtering.** Web-link lines are removed with `grep -v` and the removal is stated in the output. Confirm this is acceptable under the no-URL rule.
6. **Exam P ("design an FSM from a word description").** F1-19's method section and mini-project (vending machine) are written as practice for it. The exam itself was not written.

## Owner rulings applied

1. **FPGA board for F1-22 Part B** — rulings D1, C3 and B2. `build/KIT.md` did not exist when this unit was verified, so no board is named (decided by verifier: name none until KIT.md names one). Part B stays "untested on hardware"; its box now says exactly what is needed to test it (board with ≥ 4 LEDs and 1 button, vendor toolchain, constraints file, DIV from the board clock, a recorded run). `board_top.v` keeps `DIV = 4` for simulation.
2. **Verilog standard edition** — ruling C2. Cited as IEEE Std 1364-2005 (catalogue page opened: superseded by IEEE 1800-2009; active successor IEEE 1800-2023, catalogue page opened). Its text was not opened, so no claim rests on it alone; the labs keep `-g2005`.
3. **Curriculum milestone** — decided by verifier: none. The course card (guide 5.6) itself says "Maps to: Prepares HW202"; kept as is.
4. **Course project split and widths** — ruling B3 (the proposed split is approved): PC in F1-17/F1-21, memory unit in F1-20, resource budget in F1-22. Decided by verifier: keep 8-bit addresses and words in HW201; widening to HW202's 16-bit CPU is HW202's integration step (`program_counter.v` has a WIDTH parameter; the RAM's word width is the one change HW202 must make).
5. **Verilator output filtering** — decided by verifier: acceptable. Removing the tool's web-link lines keeps chapters free of URLs (AH-30), and every filtered output says so; the warnings themselves are unchanged.
6. **Exam P ("design an FSM from a word description")** — ruling B4: exams are written in this pass under the separate exam brief, not in this verification task. F1-19's method section and mini-project remain the practice material.
7. **Analogy proposals** (latch = serving hatch, flip-flop = ticket clip at the bell, ...) — ruling A3: approved; each chapter's "Where the analogy breaks" was re-checked.
8. **Glossary duplicates** — ruling A7: earlier courses' definitions win; qualified names kept ("Counter (hardware)", "Register (hardware)", "Feedback (in a circuit)", "Module/Port/Parameter (Verilog)"). The `source` field of every entry now names the opened source and chapter (or the chapter's Sources list) instead of "pending verification".
9. **Exercise values** — ruling A4: the delays, periods, traffic timings, 95 % leak per tick, 0.5 threshold and 8 rows remain labelled "exercise value" in the chapters.
10. **Recorded runs** — ruling A5: all seven labs re-run; outputs identical, only log dates differed, so the recorded files were restored.
11. **Licence** — ruling A10: no `LicenseRef-Uni-Lab` placeholder exists in `labs/F1-16` to `labs/F1-22`; nothing to change (the lab code falls under MIT by the ruling). **Teaching keys** (A8): none in this unit.
12. **Hardware status** — rulings C3, C4: only F1-22 Part B is untested on hardware; F1-16 to F1-21 have no hardware steps.

## Verification pass

Done on 2026-10-10 by the Fact-Checker agent (verification brief `build/verify/VERIFY_BRIEF.md`).

**Opened** (URLs in the dossiers `_dossiers/F1-16.dossier.html` … `F1-22.dossier.html`):
- Harris & Harris, *Digital Design and Computer Architecture, RISC-V Edition* (Elsevier catalogue page: 1st edition, 2021) and the authors' own lecture slides for chapters 1, 3, 4 and 5 ("© 2021 Sarah Harris and David Harris"). The book text itself was not opened; claims cite the slide headings.
- Nisan & Schocken, *The Elements of Computing Systems*, chapter 3 "Sequential Logic" (authors' site; no edition printed).
- IEEE 1364-2005 and IEEE 1800-2023 catalogue pages (text not opened).
- Vendor documents: TI SN74HC161 (SCLS297D), Nexperia 74HC393 (Rev. 9) and 74HC164 (Rev. 11) data sheets; Intel/Altera Quartus Design Recommendations (683082, 683323) and Timing Analyzer (683243); AMD UG470 v1.17, UG474 rev 1.9, UG953 2021.1 (RAMB18E1); Lattice MachXO2 family page.
- David Harris, "Lecture 15: SRAM" (CMOS VLSI Design 4th Ed. slides); Kim et al., "Flipping Bits in Memory Without Accessing Them" (DRAM background and disturbance errors).
- Built-in help of the installed Yosys 0.33, Icarus Verilog 12.0 and Verilator 5.020 (plus the online Yosys cell-library page).
- **Not opened** (the shared web-access budget of the session ran out): Patterson & Hennessy, *Computer Organization and Design*; Petzold, *Code*. No claim rests on them any more.

**Checked:** 222 tagged claims (F1-16 37, F1-17 27, F1-18 32, F1-19 26, F1-20 40, F1-21 35, F1-22 25) plus all unverified boxes. Every "Title only — not opened" note is gone; tags now name the slide heading or section.

**Main corrections:**
- F1-18: clock gating is done with a register on the inactive edge feeding the gate (not "special cells, not an AND gate"); the synchronizer gives the first flip-flop the period minus setup time, not a full cycle; "clock network is one of the largest power consumers" softened.
- F1-20: DRAM read/restore, banks and commands now follow Kim et al. §2; SRAM-vs-DRAM speed and cost claims moved into the unverified box; "memories in processors are synchronous" limited to FPGA block RAM; the firmware sentence removed; `cell` as a reserved word confirmed by a check run with `-g2005`.
- F1-21: the `$_SDFFE_PP0P_` reading is confirmed by Yosys's own help (box turned into a note); simulator race claims moved into a new unverified box.
- F1-22: FPGA-vs-ASIC trade-off and uses moved into the unverified box; volatile configuration and reload, synchronous block RAM, 6-input LUTs of one family now cited to vendor documents; `integer` = 32 bits shown by the R4 run.
- F1-16/F1-17/F1-19: SR latch jargon no longer says "(or NAND)" with active-high S/R; ripple-counter timing-analysis difficulty sourced to vendor guidance; Gray encoding for clock-domain crossing moved into an unverified box.

**Left unverified (boxes):** part-specific timing numbers (F1-16, F1-17, F1-18); metastability of a released NOR latch (F1-16); clock generation on real boards (F1-18); Gray encoding and real traffic rules (F1-19); SRAM/DRAM speed, cost and JEDEC numbers (F1-20); simulator races and sim/synth mismatches (F1-21); FPGA vs ASIC and real toolchain details (F1-22).

**Labs:** all seven re-run with `run_lab.sh`, exit 0; outputs identical to the recorded ones (expected-fail forensic steps still fail as intended). F1-22 Part B remains untested on hardware.

**QA records:** `university/qa/F1-16.json` … `F1-22.json`, factcheck "done".
