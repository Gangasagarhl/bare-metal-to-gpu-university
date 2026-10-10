# Decisions for the owner

Collected by `university/build/decisions.py` from each unit's `NOTES.md`. Each section is the author's list, copied as written; the full notes (unverified claims, runs, analogy proposals) stay in the unit's own `NOTES.md`.

## KID101
Source: [`chapters/KID101/NOTES.md`](chapters/KID101/NOTES.md)

### Unverified boxes (AH-18; need owner approval to publish, AH-19)

- F0-01: none.
- F0-02: "billions" of two-state parts is a sense of scale, not a measured number.
- F0-03: samples per second of real recordings and bytes per pixel of real image files (none given).
- F0-04: the names "selection sort" and "sorting network" and any claim about which methods
  real programs use: no algorithms textbook in the F0 source registry.
- F0-05: register counts, cache sizes and relative speeds (none given; learner may write own number).
- F0-06: none.
- F0-07: none.
- F0-08: none.
- F0-09: real packet sizes and loss rates (none given).
- F0-10: real control-loop rates and motor/battery ratings (none given).

### Decisions left to the owner or the Dean

1. **Analogy registration (guide 8.2).** F0-05 uses the registered F1/F2 restaurant-kitchen
   map (head chef, hands and cutting board, shelves near the stove, big pantry down the
   corridor, warehouse across town, corridors and service lifts). It adds two mappings that
   are not in the registry: **input = order slips through the order window** and **output =
   plates out through the serving hatch**. The Dean should register them or choose others.
   F0-06 and F0-10 use "senses / voice / muscles" for input devices, output devices and
   actuators, consistent with the registered F9 "sensors / actuators = senses / muscles".
2. **Algorithms textbook.** The F0 source registry has none; F0-04 needs one (its D2 is a
   placeholder entry asking the Dean to choose).
3. **Glossary overlaps for the Integrator (10.3).** These KID101 terms are also proposed by
   other courses: Bit, Binary, Place value, Byte (MA102); Robot, Sensor, Actuator, Simulator,
   Motor, Program, Loop, Condition, Control loop (RB101); Simulator, Sensor, Motor, Control
   loop (KID103). KID101 is a prerequisite of those courses, so its chapters should be listed
   first as the defining chapters; the wording must be merged into one definition per term.
4. **Builder behaviour to confirm.** Forensic evidence code is shown with `data-src`
   (F0-07 `fill_forgot.cpp`, F0-10 `robot_stale.cpp`) without a line-by-line table, because
   it is evidence, not a teaching listing. The three small simulators
   (`F0-01/literal_cook.hpp`, `F0-08/grid_robot.hpp`, `F0-10/robot_world.hpp`) are compiled
   with the listings but not shown in the chapters; the chapters say where they are.
5. **Cross-links** point to chapters and courses of other agents' batches (`#KID102`,
   `#F0-29`, `#F0-32`, `#F0-33`, `#F0-36`, `#F0-37`, `#MA101`, `#MA102`, `#KID103`, `#RB101`,
   `#HW101`, `#HW102`, `#DS201`), and to `#safety` and `#analogies`; the Integrator should
   confirm they resolve.
6. **Course-level items not in the chapter files.** The course exam (Q per chapter is covered
   by each chapter's "Check yourself"; the practical "write an algorithm another person can
   follow without asking questions" is practised in F0-01 and F0-08) and the course project
   (the paper binary adder: started in the F0-02 mini-project, rules and lab in F0-07, rules
   verified for all 256 four-bit pairs by `add_rules.cpp`) still need the Exam Writer's
   separate files and rubric in `_keys/`.

## KID102
Source: [`chapters/KID102/NOTES.md`](chapters/KID102/NOTES.md)

### Decisions for the owner

- **Word counts.** The validator counts all prose outside `pre`, `code`,
  tables and SVG. By that count F0-29, F0-30, F0-31, F0-34, F0-35, F0-36, F0-37
  and F0-38 are 3,038–3,640 words. Without the jargon box, Sources and Answers,
  every chapter is 2,000–2,700 words. If the 3,000-word limit includes Sources
  and Answers, F0-37 and F0-38 need trimming of about 500–650 words.
- **Fixed seed in F0-38.** The game uses seed 2026 so that the recorded run
  (guesses from `game.in`) can be repeated; the chapter explains this and shows
  `std::random_device` for real play.
- **gdb and LeakSanitizer.** Inside gdb the sessions set
  `ASAN_OPTIONS=detect_leaks=0`; normal runs keep every check. The chapter and
  `F0-37/README.md` explain why.
- **Course project and exam.** F0-38's Mini-project holds the KID102 project
  brief (≥ 3 functions + a test function that runs first) and the practical exam
  practice (times table). Rubric weights are proposals.

## KID103
Source: [`chapters/KID103/NOTES.md`](chapters/KID103/NOTES.md)

### Course-wide decisions and open items (for the owner / Dean)

- **Kit not chosen.** No kit, board, microcontroller, LED, resistor, sensor, motor, driver,
  battery pack or simulator product is named. Every chapter's D3 (or D2/D4) source is "the
  datasheets/instructions of the kit chosen for this course (dossier part F) — not yet
  chosen". Decision needed: the exact kit, its datasheets, its board toolchain.
- **"Simulator" = our own C++ text programs** in `university/labs/F0-xx/` (circuit truth
  model, meter model, button/toggle model, blink with a pretend clock, scripted sensor table,
  H-bridge truth model, grid line-follower, safety checklist). No commercial simulator.
- **All LED/battery/pin/motor numbers are pretend exercise numbers**, labelled as such in
  text and code: battery 6 V, LED forward voltage 2 V, LED max 20 mA (F0-40); pin limit
  10 mA, motor 300 mA (F0-44); light 0–100 "simulator units", distances in a scripted table
  (F0-43). Thresholds (30, 20 cm, 26/34) are our own exercise choices.
- **Books are title-only** (no internet in this build): Platt "Make: Electronics"; Horowitz
  & Hill "The Art of Electronics"; Petzold "Code"; Åström & Murray "Feedback Systems"; Lynch
  & Park "Modern Robotics". All marked "title only — not opened during this build; the Source
  Researcher must confirm the edition and section (dossier gate G1 open)". Several claims
  attributed to Horowitz & Hill (debouncing, H-bridges, PWM, inductive spikes, ADCs,
  microcontroller pins) are standard but must be located in the edition the Researcher picks;
  if a claim is not in the chosen book, re-tag it to another registry source.
- **Physical (kit) steps were not performed** in any chapter — no kit was available. Each
  physical part has a safety box at the top of the lab (and repeated at the physical step)
  plus an unverified box saying the steps are kit-independent and must be checked by the
  adult against the kit's instructions.
- **Prerequisite note:** KID103 lists only KID101, but the labs read and edit small C++
  programs. Each chapter says "a teacher or parent may run the build command"; KID102 helps
  but is not required. The Dean may want to add KID102 F0-29 as a soft prerequisite.
- New analogy mappings used (please register or reject): LED = one-way drain that glows;
  resistor = water saver on a tap; motor driver = Yusuf's lever valve (built on the
  registered "transistor = tap opened by a small control valve"); H-bridge = four valves
  around a water wheel; LiPo = pressure cooker; mains = water main under the street;
  short circuit = burst pipe from tank to drain; line follower = walking a ribbon; F9's
  "control loop = bicycle" and "sensors = senses" are used only as pointers.
- I accidentally re-ran `run_lab.sh` over *all* `labs/F0-*` folders once (including other
  courses' F0-01…F0-38) while checking mine. It only regenerated their `.out`/`.log` files
  (all passed; dates changed). Their authors may want to re-run their own folders.

## MA101
Source: [`chapters/MA101/NOTES.md`](chapters/MA101/NOTES.md)

### Things I was unsure about / decisions for the owner or Dean

1. **New analogy mappings** (guide 8.2 says the Dean registers them): beans in cups and
   bowls (place value), children in cars (rounding up), flatbread slices (fractions), lift
   floors and thermometer (negative numbers), paper folding (powers of two), labelled jar
   for a maths variable (registered for *program* variables; used here with the stated
   difference), balance scale (equations), function machine, numbered jar shelf (allowed
   range / bounds check), "tables and seats → tickets" (index formula). Please register or
   replace.
2. **"Halfway rounds up"** (45 → 50) is stated as *this course's* convention; the chosen
   textbook may present it differently.
3. **"Billion = a thousand million"** is stated as this course's definition, not as a
   universal fact.
4. **Bit and byte** appear in F0-15's jargon box but are not proposed in `glossary.json`,
   to avoid two definitions (owners: F0-02 for bit, F0-23 for byte). The F0-15 text says
   "on the machine that built this chapter, a byte had 8 bits" (measured).
5. **F0-15 mini-project = course project starter** (powers-of-two poster, each example
   sourced). F0-19 mini-project = practice for the practical exam (helpers and jobs).
   Course-level exams (Q, M, F, P) and the project brief are not written here (not in
   my deliverables).
6. **Forensic labs**: all nine use the course's "The wrong total" theme; each evidence pack
   is the real output of a deliberately broken program (R3). Answer keys are inline under
   "Forensic lab answer key" as the brief requires (guide 7.1 would put them in `_keys/`
   for graded use).
7. **Lab Engineer catch**: the F0-16 number-check run caught an arithmetic slip in my draft
   (3a + 2b with a = 1, b = 10 was written as 32; the run printed 23). Fixed before hand-off.
   This is the reason every chapter's numbers are recomputed by `checks.cpp`.
8. **Reading level**: Layer 1 average sentence length by a simple script is 7.5–10.2 words
   per chapter (L0 target about 12, L1 about 15). A few heading+sentence joins show as
   "long" in the script; the Editor should still do the read-aloud test (10.4).
9. **Prose length**: running prose (excluding code, SVG, tables, jargon lists, numbered
   lists, quiz, sources and answers) is 2,054–2,512 words per chapter (F0-14 lowest,
   F0-18 highest), inside the 1,500–3,000 band; including all of those it is 3,400–4,200.
10. **Cross-links** to chapters of other courses (#F0-02, #F0-20, #F0-23, #F0-24, #F0-30,
    #F0-31, #F0-32, #F0-33, #F0-34, #F0-36, #KID102, #MA102) assume those ids exist in
    the integrated page; glossary links #gl-bit and #gl-byte assume the KID101/MA102
    entries.
11. The SVG in F0-15 and F0-18 uses a `transform="rotate(...)"` attribute on axis labels
    (geometry only, no colour); acceptable under 9.1 as I read it.

## MA102
Source: [`chapters/MA102/NOTES.md`](chapters/MA102/NOTES.md)

### Decisions left to the owner

1. Approve publication with the 5 unverified boxes (F0-22 ×1, F0-23 ×2, F0-24 ×1, F0-25 ×1)
   (AH-19), or have the Source Researcher close them first.
2. Add "Introduction to Algorithms" (or another text) to the registry for F0-28.
3. Glossary overlap: **Bit**, **Byte**, **Overflow** and possibly **AND/OR/NOT** may also be
   defined by KID101 (F0-02, F0-03). The Integrator must merge so that one wording is used
   (guide 10.3). MA102's definitions are in `glossary.json` with `chapters` listing the
   defining chapter first.
4. Levels: F0-20 is L0, F0-21 and F0-26 are L0–L1, the rest L1 (course card is L0–L1).
5. Course project (binary-to-hex-to-decimal converter with tests, written in KID102) and the
   course exams (Q, F, P "decode a made-up 8-bit status register") are not part of this
   deliverable; F0-25's worked example and mini-project prepare for exam P, and F0-28's
   Transition box introduces the round-trip test for the project.

## RB101
Source: [`chapters/RB101/NOTES.md`](chapters/RB101/NOTES.md)

### Per chapter: claims to verify, boxes, decisions

#### F9-01 What makes a robot (L0)
- Conceptual claims to confirm in D2/D1/D4: robot = sensors/computer/actuators in a loop;
  controllers with loops inside loops; a sensor changes a voltage that a microcontroller
  reads; a control pin cannot power a motor directly, so a motor driver switches it.
- "Most people would not call a timer toaster a robot" is framed as the course's own
  criterion, not a fact. The cleaning-robot and automatic-door examples are written as
  "imagine…" (no product claims).
- Unverified boxes: none. Owner decisions: none.

#### F9-02 Senses: sensors (L0)
- To confirm: light sensor pointed at the floor "often next to a small lamp" measures reflected
  light (D4); "some distance sensors send a pulse of sound or light and measure the echo"
  (D3); bump sensor = switch (D4); noise model "true value + random error", bias, calibration (D3).
- All sensor numbers are simulator units; no real-part numbers (AH-21).
- Unverified boxes: none. Owner decisions: none.

#### F9-03 Muscles: motors (L0)
- To confirm: motor converts electrical energy to rotation; motor driver; gearbox trades speed
  for force; battery running down reduces turning for the same command (D4); differential
  drive turns toward the slower wheel; real encoders give many counts per turn; wheel slip (D2).
- The simulator does not model the turn geometry (stated in Layer 2); `drive(5, -5)` prints
  "curves to the right", which the lab uses deliberately as a limitation to discuss.
- Unverified boxes: none. Owner decisions: none.

#### F9-04 Brains: programs and loops (L0–L1)
- To confirm (D1/D7): real controllers run at a fixed rate; late loops behave badly. The
  "watchdog" is mentioned only as something later courses meet (no claim about any product).
- Unverified boxes: none. Owner decisions: none.

#### F9-05 Steering back to the line (L1) — the course's control-loop chapter
- Figure 1 adapts guide Figure 4 (PID loop) to a kid-level sense–compare–correct loop, P only.
- Figure 2 (zig-zag vs gentle plot) was drawn from the printed values of `zigzag.out` and
  `steer.out`; coordinates computed by a small script from those files (x = step, y = distance,
  14 px per unit).
- Forensic "The robot that zig-zags": evidence = real run with gain 1.8 (text plot), fix =
  real run with gain 0.5; both in the chapter via `data-run`.
- To confirm (D1): delays make feedback loops more prone to oscillation (also *demonstrated*
  by Listing 3's run); the terms setpoint/error/proportional control.
- To confirm (D2): "real line followers can use two or more light sensors side by side and
  infer the offset from which ones see the line" — conceptual, kept in "How the hardware
  actually does it" with a tag; if D2 does not support it, move it into an unverified box.
- The simulator model is deliberately simple (robot moves sideways by exactly the steer
  amount; new = (1 − gain) × old). This is said in Layer 2, Layer 3 and "Where the analogy
  breaks"; RB202 should model heading properly.
- Unverified boxes: none. Owner decisions: none.

#### F9-06 A robot in the simulator (L1)
- Course P-exam task ("program the simulated robot through a course") is the lab here.
- `grid_world.hpp` is shown with `data-src` (a header, not a `.cpp`) and `grid_sim.in` is also
  shown with `data-src`: **the builder must accept non-.cpp files in `data-src`**.
- The maze has exactly one corridor route; the lab asks learners to argue that.
- To confirm: grid/cell maps (D3); heading, plans plus feedback, drift, simulation (D2);
  open loop vs feedback (D1).
- Layer 3 mentions "professional robot simulators" generically and says later courses name
  one in their dossiers — no simulator is named anywhere.
- Unverified boxes: none. Owner decisions: none.

#### F9-07 Robot safety (L0–L1)
- All safety rules are cited to S1 (the guide): simulator first; physical kill switch;
  clear zone; adult supervision (Honesty first: L0–L2; 5.14: below L3 — the chapter uses
  "below L3", which covers everyone in RB101); kits designed for children, no LiPo, no mains
  (5.5 / Honesty first). Kit charging/storage rules are deferred to the kit maker.
- "A switch between battery and motors cuts motor power" — conceptual, D4.
- **Unverified box U1 (AH-18/AH-26): optional supervised kit lab, Part B — untested on
  hardware.** No kit was available and none is named. Needs: the owner's kit choice, its
  maker's instructions and datasheets (motor, battery, charger, switch) in the dossier, and a
  supervised trial following the checklist, recorded in the QA record.
- **Owner decision box (inside U1):** (1) which kit, if any (battery-powered, designed for the
  learners' age, two driven wheels, at least one floor light sensor, physical power switch
  cutting motor power); (2) whether Part B is offered at all or RB101 stays simulator-only;
  (3) how the kit is programmed (must accept a program equivalent to F9-05 Listing 1).
- The F9-07 mini-project is the RB101 course project (line follower in simulation, explained
  to a friend), started in F9-05's mini-project.

## HW101
Source: [`chapters/HW101/NOTES.md`](chapters/HW101/NOTES.md)

### Decisions for the owner

1. **Choose the HW101 kit and meter** (battery pack, breadboard, resistors, capacitors incl. any
   polarised ones, LEDs, a Schmitt-trigger inverter or a timer chip for the course project, a
   multimeter). Their datasheets/manual become D4 in every chapter; all "Part B" lab steps wait for this.
2. **Course project part**: the chapters describe the RC blinker with a Schmitt-trigger inverter
   (formula derived and simulated in F1-08). If the owner prefers an integrated timer chip, its formula
   must come from that chip's datasheet (not written from memory here).
3. **Analogy registrations (proposals to the Dean, guide 8.2)** — used in the chapters and marked
   "proposed":
   - Capacitor = "a tank with a stretchy rubber wall" (F1-05). Note guide 8.1 says capacitors have
     no simple pipe equivalent; the chapter quotes that and lists three breaks.
   - Diode = "a one-way flap valve in the drain" (F1-06), continuing F0-40's "one-way drain".
   - Multimeter = "the plumber's pressure gauge (voltage), flow meter (current) and pipe tester
     (resistance)" (F1-07).
   - Power = "how hard and how fast the spice mill is turned" (F1-03) and the "warm narrow pipe" idea.
   - Analog / digital = "the oven thermometer / the order bell" (F1-08).
4. **Approve publication with unverified boxes** (AH-19) — every chapter has at least one (the kit parts).
5. **Glossary overlaps with KID103**: terms Voltage, Current, Resistor, Ohm's law, LED, Diode,
   Anode and cathode, Forward voltage, Multimeter are also defined in F0-39/F0-40. The HW101 entries
   are consistent with them but more precise; the Integrator should merge them (guide 10.3).
6. **Exams** (Q, M, F, P) are not written here (Exam Writer's job); the practical exam "predict, build
   and measure a small circuit" maps naturally onto F1-07's lab plus F1-06's LED drive sheet.
7. Chapters run at the upper end of the L1 length range (≈3,400–4,200 words including jargon box,
   lists, answers and keys; prose alone is within 2,000–3,500). Average sentence length 14–17 words.

## HW102
Source: [`chapters/HW102/NOTES.md`](chapters/HW102/NOTES.md)

### Toolchain decision (owner please confirm)

- The Nand2Tetris hardware simulator is **not installed** here. Nothing in these chapters shows Nand2Tetris HDL syntax as verified.
- All circuits are written in Verilog. They run with Icarus Verilog 12.0 (`iverilog -g2012 -Wall`, then `vvp -n`). Gate counts come from Yosys (`synth; abc -g NAND`).
- F1-09 also has a small C++ ideal-switch model.
- The practical exams and projects are set in Verilog. If the owner wants Nand2Tetris `.hdl` files instead, that needs the simulator installed, and every listing must be re-run in it.
- **All runs are simulation only.** Every log carries "untested on hardware". No breadboard or FPGA was used.

### Decisions for the owner

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

## HW201
Source: [`chapters/HW201/NOTES.md`](chapters/HW201/NOTES.md)

### Decisions for the owner

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

## HW202
Source: [`chapters/HW202/NOTES.md`](chapters/HW202/NOTES.md)

### Decisions for the owner

1. **The U16 teaching ISA** (F1-23 onward): 16-bit words, eight registers with r0 = 0, Harvard
   memories of 256 data words, 12 instructions (HALT, ADD, SUB, AND, OR, SLT, ADDI, LW, SW, BEQ, BNE,
   OUT), three formats; branch target = pc + 1 + imm. It is invented for this course so the whole CPU
   fits in one header (`labs/F1-23/u16.h`) that F1-24 … F1-28 reuse. Alternative: a RISC-V subset
   (more transferable, but larger and needs byte addressing). Please confirm or choose.
2. **The pipeline timing model** (`labs/F1-26/pipe.h`): classic five stages, register file written in
   the first half-cycle and read in the second, forwarding rules (ALU → EX+1, load → MEM+1), predict
   not taken, branches resolved in EX (2-cycle penalty). It computes timing from rules rather than
   simulating pipeline registers. The identity cycles = instructions + 4 + stalls + branch cycles holds
   for every run. A gate-level HDL pipeline is suggested as a mini-project, not provided.
3. **Verilog in F1-25**: the control unit is written in Verilog and checked against the C++ table with
   Icarus Verilog, and synthesised with Yosys for a cell count. This needs `iverilog` and `yosys` on
   the student machine. Keep, or make it optional?
4. **Invented delay units** (F1-24, F1-26): stage delays are made-up "du", stated as such.
5. **Real timings** (F1-29, F1-31, F1-30 counts) vary between runs; prose is qualitative. Owner may
   prefer to drop real timings from course pages entirely.
6. **perf unavailable** in the build container; F1-28/F1-29 say so. If a machine with working `perf`
   is available, a measured branch-miss lab would strengthen F1-28.
7. **Cross-compiled code not executed** (ARM64, RISC-V). Installing `qemu-user` in the build image
   would let the labs run it.
8. **Compiler Explorer** named on the course card was unreachable; F1-31 uses local compilers and says
   so in the lab text.
9. **Course-card title**: F1-30 uses the card's title "Multicore processors".

## HW203
Source: [`chapters/HW203/NOTES.md`](chapters/HW203/NOTES.md)

### Decisions for the owner

1. **CS:APP Cache Lab.** The card names it; it was unavailable. Options: obtain the official handout
   and add it as an external lab in F1-34 (licence check needed), or accept the course simulator labs as
   the substitute.
2. **Counters.** The card asks to "explain from perf counters"; this build used Cachegrind (simulated)
   instead. A rerun of F1-34 (and F1-36/F1-38) on a machine with working `perf` should add real counter
   evidence and close the related unverified boxes.
3. **NUMA lab.** Needs a two-socket machine; until then the NUMA step stays "untested on hardware".
4. **Rerun policy.** Decide whether timing outputs are frozen with the quoted numbers (current state) or
   the prose should be rewritten to quote ranges so reruns stay consistent.
5. **Analogy proposals** above, especially the two outbox trays.
6. **Prerequisites.** F1-36 and F1-37 list SP201 as "may run in parallel" (atomics are used in the labs
   and explained locally); confirm with the SP faculty or move the atomics material.

## HW204
Source: [`chapters/HW204/NOTES.md`](chapters/HW204/NOTES.md)

### Decisions for the owner

1. **Course kit.** No microcontroller board, I2C sensor or logic analyser is named anywhere. The three hardware parts are written so that they work with any board, and each is marked untested on hardware. Choosing a kit would let a Lab Engineer run them and replace the unverified boxes.
2. **Where the course labs live.** The labs from the course card are placed as follows:
   - UART echo, polling then interrupts: F1-42.
   - Logic-analyser I2C capture: F1-47, Part B.
   - QEMU monitor inspection of PCI: F1-45.
   - The forensic lab "The missing bytes": F1-42.
   - Exam task P, decoding a captured I2C transaction: F1-47, Worked example 1. It uses the text capture, because no real capture exists yet.
   - The course project: the F1-48 mini-project.
3. **Extra QEMU evidence beyond the course card.**
   - F1-43: x86 `info pic`/`info irq`, plus the RISC-V and Arm memory maps.
   - F1-44: `intel-iommu` memory tree.
   - F1-46: USB pcap capture.

   Please confirm that QEMU 8.2.2 and tshark are acceptable tools for the course.
4. **Glossary.**
   - `glossary.json` has 74 terms. "Bus" is left out on purpose: it is already defined (KID101), and F1-40 links to `#gl-bus`.
   - F1-41's "Port I/O" no longer carries the abbreviation "PIO", which F1-44 uses for "Programmed I/O".
   - "Enumeration" (PCI, F1-45) and "Enumeration (USB)" (F1-46) are separate entries. They could be merged.
5. **The F1-45 build-machine output depends on the machine.** `cfg_read` and `lspci` describe whatever machine runs the lab. The prose names the build machine's devices and says that the reader's will differ.

## HW205
Source: [`chapters/HW205/NOTES.md`](chapters/HW205/NOTES.md)

### Decisions for the owner

1. **Doorbell analogy conflict** (above): accept the per-chapter wording or register a separate image.
2. **Wireshark lab**: accepted as loopback + QEMU capture + a student step on their own machine. If a
   capture of a physical NIC by the author is required, a machine with a network the author may
   capture is needed.
3. **F1-53 depth**: written as a preview with a toy model and an honest "untested on hardware" record;
   real verbs runs (Soft-RoCE or RDMA NICs) are left to DS401. Confirm, or provide a machine where
   Soft-RoCE may be loaded.
4. **F1-54 scope**: the chapter covers power (V²f, DVFS, P/C/S-states as a map), thermal throttling and
   ECC/MCA. Sensor and EDAC reading is a student step; the build VM exposes none. Confirm that the
   physics of heat flow stays at the one-line model level (the HW1xx F1-03 chapter covers power and heat).
5. **Toy models as forensic evidence**: all four network/memory forensic packs (RGMII, rx stall, stale
   key, ECC log) come from the course's simulators and say so. The NVMe and RGMII register maps in
   them are invented and labelled; check this is acceptable for "PHY register dump description".
6. **Word counts** exceed the 3,000 (L2) / 4,000 (L3) targets once tables and figure text are counted;
   prose alone is close to target. Trim if the owner wants shorter chapters.
7. **`.cc` listings**: `nvme_trace.cc` (F1-50) uses the `.cc` extension so that `run_lab.sh` does not
   compile it with the default flags; `build.py`'s `LISTING_EXT` does not list `.cc` or `.asm`, but the
   listings rendered in this build (checked in UNIVERSITY.html). Confirm that convention.

`python3 university/build/build.py` exited 0; none of its PROBLEM lines (320 "broken link" lines at the last run, all
`#gl-…` targets of other courses) mentions F1-49 … F1-54 or an HW205 glossary link.

## HW301
Source: [`chapters/HW301/NOTES.md`](chapters/HW301/NOTES.md)

### Decisions for the owner

1. **Named product (F1-63; course card "one named product").**
   - Proposal: gfx90a / AMD Instinct MI200 series. Documents: the "AMD CDNA 2 Architecture" whitepaper and the "AMD Instinct MI200" ISA reference guide.
   - Alternative: an sm_80 product with the "NVIDIA A100 Tensor Core GPU Architecture" whitepaper.
   - All titles were written from memory and need gate G1.
   - The choice should match the lab GPU that is actually available.
   - Until the owner decides, the chapters say "the named product" and quote no product numbers. The F1-63 decision box states this.
2. **TG-1, an invented teaching GPU.**
   - Used for every worked example and model.
   - Values: 4 SMs, warp of 32; per SM: 16 warps, 4 blocks, 16,384 registers, 32 KiB shared memory; model latencies of 100 and 4 cycles; 256-bit memory at 8 GT/s; host link of 16 GB/s with 10 µs per copy.
   - Labelled "invented" everywhere.
   - Proposal: register TG-1 in the guide as a shared teaching device, so that other GPU courses reuse the same numbers.
3. **Analogy.**
   - The chapters use F1's restaurant world.
   - The GPU is the "banquet hall", which reuses the registered mappings of F6's great kitchen hall: section = SM/CU, row of helpers = warp/wavefront, supervisor = scheduler, tray = registers, side table = shared memory/LDS, warehouse = device memory, corridor = PCIe.
   - Proposal: add "banquet hall" to the analogy registry as F1's name for the F6 mapping, or rename it to "great kitchen hall" in these chapters if the owner prefers one name.
4. **Forensic placeholders X1–X5 (F1-63).**
   - The colleague's passport in the forensic lab has its hardware numbers replaced by X1–X5, so that no unverified number is printed.
   - The checker's rules are unaffected.
   - The owner may replace them with tagged numbers once the named product's documents pass G1.
5. **No-device outputs are kept as evidence.**
   - Runs on CUDA and HIP print the runtime's own error and are recorded with exit code 1, not hidden: `cudaErrorNoDevice`, and `hipErrorInvalidDevice` as printed by HIP 5.7.
   - The builder accepts them (each `.log` has "exit code:").
   - On a GPU machine they should be re-run and their expected outputs confirmed. Each chapter's unverified box gives the expected line.
6. **WMMA listing target.**
   - `wmma_tile.cu` is guarded with `__CUDA_ARCH__ < 700 → __trap()` because the runner builds for sm_52.
   - The chapter tells students to add `-arch=sm_80` to run it on a GPU.
   - The owner may prefer that `run_lab.sh` accept a per-lab architecture.
7. **Measured CPU numbers.**
   - F1-55 (`chains`) and F1-59 (`host_bw`) are real measurements of the build container and change on every re-run.
   - The prose quotes only rounded or qualitative values ("about 1.9 ns", "ratios close to 4", "read the median from the output").
   - The exact numbers are in the inserted `.out` files.
8. **Course forensic "Why is my GPU idle?"** This is the F1-62 forensic lab: a copy/compute model of the recorded job, 3.6 % GPU busy, explained from the link, the DMA engines and pageable memory.
9. **Curriculum seed glossary terms.** `glossary.json` uses the seed's exact term names, so our four-part entries take the seed ids: SM, Compute Unit (CU), Warp / wavefront, LDS, HBM, Occupancy, Tensor Core / Matrix Core, MFMA / WMMA, NVLink / NVSwitch, Infinity Fabric / xGMI, PTX / SASS, Bank conflict, CDNA / RDNA.
   - Five terms also exist in HW202 or HW203 and are merged by the builder: Latency, Throughput, Instruction-level parallelism (ILP), Register file, Little's law. DRAM also appears in HW203.

## HW302
Source: [`chapters/HW302/NOTES.md`](chapters/HW302/NOTES.md)

### Decisions left to the owner

1. **Kit parts.** The microcontroller, IMU, encoder (and motor with encoder), LiDAR or depth camera, camera module, DC gear motor, servo, BLDC and ESC, motor-driver IC, battery pack, charger and logic analyser are all still to be chosen. Every "Part B" hardware step, and the H2 acceptance tests on hardware, wait on these choices and on dossier part F.
2. **Safety rules.** The F1-71 LiPo rules state the course card's safety line (maker's instructions; supervised for L2; no propellers; no mains) plus common-sense handling rules (proper charger, not unattended, no damaged packs, no shorts, fuse near the pack). The owner should confirm these against the chosen battery maker's instructions and the university's safety policy.
3. **Laser safety (F1-67).** Confirm which laser class is allowed for L2 learners once a sensor is chosen.
4. **Course project.** The course project (F1-72) is defined as the course card's "sensor board logger with calibrated output, reused in RB201 and DN202". It uses H2's acceptance tests verbatim for the hardware part. The owner should confirm that RB201 and DN202 expect this scope and log format.
5. **Chapter titles.** The chapter titles follow the course card. F1-71's meta title is "Batteries (LiPo) and power distribution", and F1-72 is "Noise, grounding and interference"; the course project lives in F1-72's mini-project.

## HW303
Source: [`chapters/HW303/NOTES.md`](chapters/HW303/NOTES.md)

### Analogy proposals (F1 restaurant world). Owner approval needed.

These are proposed extensions of the F1 analogy world. They are used in the chapters but are not yet in the guide's analogy table.

| Concept | Proposed mapping | First used |
|---|---|---|
| Microcontroller | the take-away kitchen in one room | F1-73 |
| Flash (program memory) | the laminated recipe book screwed to the wall | F1-73, F1-74 |
| SRAM | the counter top | F1-73, F1-74 |
| Peripherals | the appliances (kitchen timer, hot plate, order window) | F1-73, F1-75 |
| Vector table | the first page of the recipe book | F1-73 |
| Reset handler / start-up code | the opening routine of the stand | F1-73 |
| Clock tree | the kitchen's rhythm (the beat everyone works to) | F1-75 |
| Clock gate | the wall switch of an appliance | F1-75 |
| Timer / PWM | the kitchen timer; flicking the lamp to dim it | F1-76 |
| Debug probe / halt | the inspector's pause button and hatch | F1-77 |
| Reference manual / datasheet / errata | the appliance's service manual / leaflet / "known issues" page | F1-78 |
| Schematic / datasheet limits | the stand's wiring plan / the rating plates | F1-79 |

### Decisions for the owner

1. **The real lab microcontroller and board.**
   - Choose them, and supply or name the datasheet, reference manual, errata sheet, board schematic and user guide, with revisions.
   - Every "untested on hardware" box and the board-support-note project depend on this.
   - The emulated stand-ins are `mps2-an385` (Cortex-M3) and QEMU's STM32F100 model.
2. **The GDB requirement.**
   - The course card and H1 say "GDB". This build used LLDB because no Arm-capable GDB was installed.
   - Either install `gdb-multiarch` in the build container so the session can be re-run with GDB, or accept LLDB as equivalent for the emulated acceptance.
3. **The debug probe.**
   - Choose the probe and probe software (GDB server) for the lab board.
   - Nothing about JTAG/SWD signalling was exercised.
4. **QEMU limits.**
   - The `mps2-an385` CMSDK GPIO is not modelled, so the LED is the FPGAIO register.
   - `stm32vldiscovery` RCC, GPIO and timers are not modelled (logged as unimplemented).
   - Confirm that the emulation-plus-model approach is acceptable until hardware exists.
5. **Approve or change the analogy proposals** above.
6. **Approve the fictional teaching parts** U-MCU-1 and U-BOARD-1 as course conventions. They are also used by exam P (F1-78 excerpt) and the course project (F1-79).
7. **Unverified register layouts** need the Source Researcher (dossier gate G1):
   - the CMSDK UART and timer;
   - the STM32F1 USART, RCC and GPIO;
   - SysTick;
   - the JTAG TAP table and SWD format.

## SP101
Source: [`chapters/SP101/NOTES.md`](chapters/SP101/NOTES.md)

### Decisions for the owner

1. **Length.** Prose word counts are 2,245 to 2,679 per chapter. They were
   counted to Sources, without code, tables or jargon box. All are above the
   L1 target of about 2,000. Keep or trim?
2. **Normalised sanitizer output.** The F2-05 ASan reports show `0x<addr>`,
   `<pid>` and `<hash removed>` instead of real values, so reruns stay
   byte-stable. Accept this, or show one raw report?
3. **Unit factors and the °C/°F formula** (F2-07, F2-08). They are not
   verified against a standard. Assign them to the Source Researcher, and
   decide which "cup" the course uses.
4. **Course project.** F2-08's mini-project defines the SP101 final project:
   a grade book or inventory, at least three files, and a test program with a
   10-point rubric. It builds on the F2-06 and F2-07 mini-projects. Please
   confirm this matches the course card.
5. **Forward links.** Chapters link the planned chapters F2-09 (Classes and
   invariants), F2-24 (Object representation) and F2-26 (Build systems with
   CMake), and the curriculum's Foundations section, including the P1
   acceptance line quoted verbatim in F2-08 Layer 3.
6. **`assert` exit code 134 and ASan exit code 1** are recorded as intended
   failures in `run.sh` ("fail" mode), not with `.expect-fail` (which is for
   compile failures only). The runner treats them as passes.

## SP102
Source: [`chapters/SP102/NOTES.md`](chapters/SP102/NOTES.md)

### Decisions for the owner

1. **Length.** The L2 target is about 3,000 words of prose, but the chapters run about
   3,700–4,600. The extra length is mostly the hardware sections (disassembly and symbol
   listings), the sabotage runs and the forensic answer keys. Options: accept the length,
   or move the line-by-line tables to a collapsible part.
2. **Exam P preview (F2-17).** The format is invented from the course card: a short
   specification, write the class, defend the invariant. It needs confirming by whoever
   writes the exams.
3. **Course project.** The ring buffer is the default, and the course card allows a
   matrix instead. It is built up chapter by chapter in F2-14 to F2-17: `snapshot`,
   `peek` as `std::optional`, `[[nodiscard]]`, and `for_each_oldest_first`. The final
   brief is in F2-17.
4. **C++23 content.** `std::expected` (F2-16 Listing 5) is outside the course's C++20
   flags. It is shown via `run.sh` with `-std=c++23` as Layer 3 material. Keep it or drop it.
5. **Analogy register.** Please accept or reject the mappings above.
6. **F2-17 lab step 5** (capture `limit` by reference in the factory) is offered as an
   optional exercise and was **not run**. The chapter says so.

## SP201
Source: [`chapters/SP201/NOTES.md`](chapters/SP201/NOTES.md)

### Decisions for the owner

1. **Length.** Chapters run about 5,000–5,700 words including tables and answers, above the
   ~3,000-word L2 prose target. The extra is mostly code-walk tables, sanitizer outputs and
   answer keys. Trimming would cut those first.
2. **Trimmed outputs (AH-25).** Long sanitizer reports are shown trimmed, produced by
   `run.sh` from the full `.out`, with every cut marked "[... N line(s) trimmed ...]". The
   trimmed record's log says "exit code: 0 (of the trimming step …)" and points to the
   program's own log. Files: deep_trimmed (F2-20), cycle_trimmed, deep_list_trimmed (F2-22),
   view_dangle_trimmed (F2-23), stack_overrun_trimmed, off_by_one_trimmed (F2-25).
3. **Builds without `-Werror` or sanitizers in run.sh.** Forensic evidence needs "release"
   builds: F2-23 `moving_crash.cc` is built at -O2 without `-Werror` (with it, GCC's
   -Wuse-after-free stops the build — that warning is itself shown as evidence); F2-18
   `addr_lab_plain`, F2-20 `frames_plain`, F2-25 `off_by_one_plain` are built without
   sanitizers. `moving_crash.cc` uses the `.cc` extension so run_lab.sh does not compile it
   with the course flags.
4. **Crashing runs recorded as evidence.** moving_release / moving_debug exit 139 (SIGSEGV)
   by design; their logs say so.
5. **Grader-only reference.** `labs/F2-25/freelist_reference.cpp` (first-fit free list with
   headers, split and coalesce, 20,000-step randomised test) is the course project's reference
   solution. It is built and run by run_lab.sh but not shown on the page; F2-25 only mentions
   that it exists. Move it elsewhere if graders' material must not sit in the public lab
   folder. Likewise `ptr_lab_answer.cpp` (F2-19), `asan_lab_fixed.cpp` (F2-21), the list
   reference (F2-22, shown in the answers), `ub_fixed.cpp` (F2-23), `header_fixed.cpp` (F2-24).
6. **Emulated architectures.** F2-24 shows AArch64 and RISC-V 64 layouts from QEMU user mode,
   labelled "untested on real hardware". They show the compilers' ABI choices only.
7. **Course forensic "The crash that moves"** is in F2-23 (lifetime and UB) rather than F2-21,
   because it needs the UB framing; F2-21's forensic is a leak instead.
8. **Exam P (fix three memory bugs from sanitizer output)** is not written as an exam paper;
   the F2-21, F2-23 and F2-25 labs are framed as practice for it. An exam author can reuse
   asan_lab, view_dangle and off_by_one with their reports.
9. **Course project** brief and 20-point rubric are in F2-25 ("Course project: two allocators
   with tests"); the mini-project there (placement-new `make<T>`) builds toward it.

## SP202
Source: [`chapters/SP202/NOTES.md`](chapters/SP202/NOTES.md)

### Decisions for the owner

1. **perf.** The curriculum's natural Linux profiler cannot run in the build
   container. F2-33 teaches callgrind and gprof with real runs and presents
   perf as "try it on your own machine". If a perf-capable runner becomes
   available, add a `perf stat`/`perf record` step to F2-33's `run.sh`.
2. **gprof disagrees with callgrind** on `hotspot.cc` (gprof ranks the sort and
   `make_lines` high, `parse_line` low; callgrind: `parse_line` 60.82 %). The
   chapter keeps the disagreement as a teaching point with an unverified box.
   Option: add a static-link gprof run to confirm the explanation.
3. **P1 acceptance** ("clean clone builds on a second machine or container") is
   only half shown: same container. Needs a second runner.
4. **Cross compiler for P1** is not built here (no `x86_64-elf` toolchain in the
   container); F2-26 uses host g++ with freestanding flags and says so.
5. **Prerequisite links.** Chapters link F1-31 (reading assembly), F1-23–F1-25,
   F1-30 (multicore, for data races/TSan), F2-23 (object lifetime and UB),
   F0-36 (reading compiler errors) and MA102. Please confirm these are the
   intended cross-references; F1-30 is used as the home of data races because
   no SP203 chapter existed when this was written (F2-35 "Data races" is
   SP203's; the owner may prefer to retarget TSan pointers to F2-35).
6. **Word counts.** Chapters run about 4,100–5,900 words including tables,
   transcripts excluded; the brief's ~3,000 words of prose for L2 is exceeded
   mainly by the jargon boxes, tables and the forensic answer keys. Trim if the
   owner wants a strict limit.
7. **Forensic names** (Diego, Priya, Tomás, Lena, Amara, Wei, Rosa) are
   invented characters, not real people.

## SP203
Source: [`chapters/SP203/NOTES.md`](chapters/SP203/NOTES.md)

### Decisions for the owner

1. **Noisy shared machine.** All timing results were measured while many other agents were building on
   the same 4 vCPUs. Several measurements (F2-34 speedup, F2-38 false sharing, F2-41 large tasks) did not
   show the textbook effect reliably. The chapters say so instead of quoting the expected numbers. Decide
   whether to re-run the timing programs on a quiet or dedicated machine before publication.
2. **Arm memory ordering untested on hardware (F2-39).** Only compiler output and a QEMU user-mode run
   exist. An Arm board (Raspberry Pi 4/5) run of `publish_config.cc` in a loop, and of a store-buffering
   litmus test, would turn the untested box into evidence.
3. **libatomic.** The course build command cannot link `atomic<T>::is_lock_free()` for 16-byte types; F2-38
   keeps that as an expected-fail listing and runs a second build with `-latomic`. Decide whether the
   course command should add `-latomic` (it would hide this lesson).
4. **Benchmark sizes reduced for build time.** `pc_bench.cc` moves 400,000 items per configuration
   (2 million took over five minutes under load); `pc_test` keeps the curriculum's 10 million items and
   needs `pc_test.timeout` = 120 s. TSan run of `pc_test` takes 21–32 s here.
5. **Cross-lab include.** `F2-42/ring_bench.cc` includes `../F2-37/bounded_queue.hpp` to compare with the
   same queue. If labs must be self-contained, copy the header into F2-42.
6. **Forensic programs are reproductions.** Scenarios (the 3 a.m. ledger, the oven board, the theatre)
   are illustrative; each key ends with "How the evidence was made". The F2-40 dump is a real GDB attach to
   a really deadlocked process, trimmed only as its log says.
7. **Glossary text for shared terms.** Where SP203's Jargon box words a shared term differently (e.g.
   Ring buffer, Read-modify-write), the published glossary keeps the earlier course's text (build order).
   The Glossary Keeper may want one merged wording.
8. **Curriculum seed "TLS (thread-local storage)"** is used (F2-40's checker uses `thread_local`) but has
   no four-part entry yet; F2-34/F2-40 could take it in a later pass.

## SP301
Source: [`chapters/SP301/NOTES.md`](chapters/SP301/NOTES.md)

### Decisions for the owner

1. **P1 toolchain:** no GCC `x86_64-elf` cross compiler was built. The P1 skeleton uses Clang +
   LLD with `--target=x86_64-unknown-none-elf`, which the curriculum's toolchain table allows
   ("GCC cross compiler (x86_64-elf) or Clang + LLD"). Building GCC is offered as an extension.
2. **P1 "second machine":** the lab clones into a second folder of the same container; the
   chapter says so and asks students to repeat on a second computer before counting P1 done.
3. **P2 fuzzing:** no libFuzzer/AFL runtime exists in the container (no `clang_rt` fuzzer
   library), so `elf/fuzz.cpp` is a small mutation fuzzer under ASan/UBSan with a fixed seed,
   not coverage-guided. The chapter states this. The one-hour run is a separate script
   (`fuzz_hour.sh`) so that `run.sh` stays at about two minutes; its log is recorded once.
4. **Boot kernels are 32-bit (i386 Multiboot via QEMU `-kernel`)** for F2-46/48/49/50, while
   P1 targets x86_64. This keeps the labs free of an x86-64 bootloader before track A; the
   chapters note it. The owner may prefer Limine or a UEFI loader later.
5. **F2-45 UEFI app avoids UEFI tables** (prints through QEMU's port 0xe9, exits through
   isa-debug-exit) because UEFI structure layouts could not be verified in this build. A1 will
   use the system table properly.
6. **F2-50 poll count is timing-dependent;** the kernel prints only "at least one" / "none"
   so the expected output stays reproducible. The recorded run printed "none (already done)".
7. **`README.md` files inside `labs/F2-48/p1/`** are lab content (the skeleton repository the
   student copies), not documentation of the build.
8. **Exit codes in logs:** each `.log` records the exit code of the step's *last* command; where
   the interesting status belongs to an earlier command (QEMU exits 33/35/67, crashes 139),
   the R-source text in the chapter says "in the transcript".

## SP302
Source: [`chapters/SP302/NOTES.md`](chapters/SP302/NOTES.md)

### Lab conventions (decision taken in this course)

- `.cpp` files are correctness programs that `run_lab.sh` builds with sanitizers.
- `.cc` files are timing programs that `run.sh` builds at `-O2` / `-O3` without sanitizers, because sanitizers would distort timings. `run_lab.sh` ignores `.cc` files.
- `run.sh` writes a `.log` record for each output: listing, toolchain, command, date, machine, exit code and note. Every timing log carries the AH-23 note "measured on the build container (a shared cloud virtual machine) … not a specification".
- `bench.hpp` is the shared timing harness from F2-51 (Listing 1 there). Byte-identical copies are in each lab folder, so that every folder runs on its own. md5 was checked: one distinct hash. If the harness is ever changed, change all six copies.
- `F2-56/roofline.in` is a **generated** file. `run.sh` writes it with `grep` from `peak.out`, `stream.out` and `sgemm.out`. It is kept because `run_lab.sh` feeds `.in` files to `.cpp` programs; `roofline.cc` is a `.cc` file, so `run_lab.sh` does not use it.
- Chapter prose describes recorded timings qualitatively ("several times", "about ten times") or quotes the recorded `.out` exactly. The labs were **not** re-run after the chapters were written, so every number in the prose matches the committed outputs. Re-running changes the timing outputs (noisy VM); if that happens, re-check the numbers quoted in the worked examples and answer keys:
  - F2-54: 0.560 / 4.335 ns.
  - F2-55: 16.33 / 26.40 ms.
  - F2-56: all roofs and the speed table.

### Decisions for the owner

1. **Roofline intensity convention.** F2-56's tool uses compulsory traffic, so every SGEMM step sits at the same intensity. The chapter explains why this is an upper bound and misleading as a diagnosis. Measured DRAM traffic needs hardware counters (unavailable). Accept this, or ask for a Cachegrind-based traffic estimate as an extra listing?
2. **"The 2× regression" is 2.16× in instructions but about 1.6× in time** on the build VM. The forensic scenario keeps the card's title, and the answer key teaches reporting both. Keep it, or tune the program so that time is also about 2×?
3. **AVX2 roof versus AVX-512 roof.** The VM reports AVX-512, but the SGEMM micro-kernel is AVX2 (portable to more student machines), so the AVX2 peak is drawn as the roof and the AVX-512 peak as a dashed line. An AVX-512 micro-kernel is offered as a lab extension.
4. **perf.** Its content is written as untested on hardware. A machine with working counters should run the F2-55 lab before release to confirm the perf commands, and to measure real IPC and DRAM bytes for the F2-56 roofline.
5. **Shared `bench.hpp` copies** (six identical files). Alternative: one shared include folder, but the lab runner builds per folder.
6. **Links to chapters that do not exist yet** are given as plain text, not links: CU201, BR-03 (F2-54 meta), and "a topic for F11" (F2-54). The CU302 and SP302 course anchors are linked because those course pages exist.

## MA201
Source: [`chapters/MA201/NOTES.md`](chapters/MA201/NOTES.md)

### Decisions for the owner

1. **New analogy mappings for world F0 (everyday life at home).** Please register or reject each one.
   - Vector: steps across the kitchen floor tiles.
   - Dot product: the shadow of a walk on the direction you face.
   - Matrix: a recipe table (rows are ingredients, columns are dishes).
   - Matrix product: recipe table × weekend plan.
   - Identity, inverse and transpose: "leave it", "undo it" with socks-and-shoes order for two steps, and flipping the recipe card.
   - Rotation: a sheet of squared paper turned about a pin.
   - Coordinate frame: "my left or your left".
   - Transform chain: nested directions, the drawer in the cupboard to the left of the fridge.
   - Quaternion: a skewer twisted in a potato.
2. **Textbook edition.** B1 (Strang) is cited without an edition. One edition should be fixed for the whole maths faculty.
3. **Conventions for the course and the transform library.** These need confirming, because RB301, DN201 and DN301 reuse the library:
   - right-handed frames;
   - x forward and y left in the body frame;
   - angles anticlockwise-positive;
   - R(θ) maps body to world;
   - T_AB maps B coordinates to A;
   - Hamilton quaternions stored (w, x, y, z), mapping body to world;
   - q₂q₁ means "q₁ first";
   - yaw–pitch–roll is R_z·R_y·R_x.
   If DN201 adopts NED/FRD, a conversion chapter or box is needed.
4. **Project reference solution.** The mini-projects build a transform library across F0-47 to F0-55, with rubrics. No reference solution or hidden test `_keys` were written. Decide whether the Lab Engineer should add them.
5. **The std::apply incident (F0-52).** A helper named `apply(const Mat3&, const Vec3&)` failed to compile because argument-dependent lookup found `std::apply`. It was renamed `times`. The real compiler error is quoted in F0-52's "Common mistakes" section, and the failing log was kept in scratch space only. Decide whether this deserves a C++ chapter cross-reference (ADL).
6. **Hand-kept line numbers.** The line numbers in the code walk-through tables are maintained by hand. Any edit to a listing must update its table. A build check that compares them would help.
7. **Forensic labs beyond the card.** The card names one forensic lab ("The robot turns the wrong way", F0-53). The other eight chapters have their own small forensic labs, each with a deliberate-bug program. Review whether they should all be graded.
8. **Exam item P.** The "implement and test a 3×3 rotation utility" item maps to F0-52 `rotation_util.cpp`. The listing is the worked reference, so the exam version should change its rotation axes or its tests.

## MA202
Source: [`chapters/MA202/NOTES.md`](chapters/MA202/NOTES.md)

### Decisions for the owner / Dean

1. **New F0 analogy mappings to register.** Each chapter's `analogy:` meta line lists its
   mappings:
   - dice faces and socks in a drawer (F0-56);
   - the family scoreboard (F0-57);
   - the balance point on a ruler (F0-58);
   - the kitchen thermometer's wobbling last digit (F0-59);
   - sunshine and kitchen temperature (F0-60);
   - the barking dog and the door (F0-61);
   - the kitchen scale with a bias and a calibration bag of rice (F0-62);
   - walking times to school, with the closed level crossing (F0-63).
2. **IMU:** no kit or IMU was chosen, so the F0-62 lab ran on the university's simulator.
   Choose a learner-safe kit and IMU, and record its datasheet as the F0-62 D4 tier-1
   source. A Lab Engineer then needs to do a real run.
3. **Exams:** the course card lists Q, M, F and P exams, including P "analyse a provided
   measurement file". No exam files were produced; that is outside the chapter task.
   `labs/F0-62/imu_rest.csv` and `labs/F0-63/claim_check.in` are ready-made candidates for
   the P exam's measurement file.
4. **Timing builds:** F0-63's timing programs are `.cc` files built by `run.sh` with -O2
   and no sanitizers, following the pattern of SP302 F2-51. Please confirm this exception
   to the sanitizer flags.
5. **Glossary merging:** terms already defined by other courses were copied verbatim
   instead of being redefined. The F0-62 jargon box uses its own analogies for Bias and
   Drift ("10 g" scale, walking with eyes closed), which differ from HW302's glossary
   analogies ("3 grams", Lucía). The precise definitions are identical.
6. `python3 university/build/build.py` rewrites `university/UNIVERSITY.html`, which is a
   tracked file and shows as modified. It reports no PROBLEM line for MA202 or
   F0-56 … F0-63. The remaining PROBLEM lines are broken glossary links in other courses
   whose glossaries were not yet written at build time.

## MA301
Source: [`chapters/MA301/NOTES.md`](chapters/MA301/NOTES.md)

### Decisions for the owner

1. **Calculus textbook (B2).** Registry 4.2 asks the Dean to choose one. Until then every B2
   tag in MA301 is pending.
2. **"Feedback Systems" (B1).** The edition and the chapters used must be confirmed. MA301
   relies on B1 for:
   - the Laplace table and the final value theorem;
   - block diagrams and the sensitivity functions;
   - Routh–Hurwitz, root locus and time delays;
   - Bode plots and margins;
   - the DC motor model.
3. **The pretend motor** (R = 2 Ω, L = 1 mH, K = 0.01, J = 1e-5 kg·m², b = 1e-6 N·m·s,
   from F1-69).
   - MA301 uses it throughout: τm = 0.19608 s and Kdc = 98.039 rad/s per V.
   - F0-71 and F0-72 add an assumed driver lag of 10 ms and a speed-filter lag of 20 ms.
   - If RB202 or DN301 use a real motor, their dossiers should record the measured values.
     MA301's numbers are exercise values.
4. **Settling-time band.** F0-68 uses 2 % with 4/(ζωn). RB202 and DN301 should use the same
   definition, or say which one they use.
5. **Analogy registry.** All nine chapters use the F0 world "everyday life at home". The
   specific images need registry entries if the registry keeps them:
   - toy cart and sticky notes (F0-64);
   - bath and tap (F0-65, F0-66);
   - laundry trolley (F0-67);
   - kettle and kitchen scale (F0-68);
   - recipe conversion (F0-69);
   - shower mixer tap (F0-70);
   - karaoke howl (F0-71);
   - washing machine (F0-72).
6. **Figure scripts.** The SVGs are embedded in the fragments. The scripts that generated
   them live only in the scratch folder. Decide whether figure sources should be kept in
   the repository (for example `university/build/figures/MA301/`) so they can be
   regenerated.
7. **Lab listing edits after the first runs.** `#include <algorithm>`, `<cmath>` and
   `<cstddef>` were added where `std::max`, `std::sqrt` and `std::size_t` were used without
   their headers. All listings were re-run afterwards and the line-number tables match the
   final files.
8. **`university/UNIVERSITY.html`** was regenerated by `build.py`, as the brief requires.
   Other runs were rebuilding it at the same time, so the copy on disk reflects whichever
   build ran last.

## MA302
Source: [`chapters/MA302/NOTES.md`](chapters/MA302/NOTES.md)

### Decisions for the owner / Dean

1. **Textbooks (registry 4.2).** Proposed:
   - Higham, "Accuracy and Stability of Numerical Algorithms", as the MA302 core text (B1 in most chapters);
   - Trefethen & Bau, "Numerical Linear Algebra", for F0-77 and F0-78;
   - Hairer–Nørsett–Wanner, "Solving ODEs I", and Hairer–Lubich–Wanner, "Geometric Numerical Integration", for F0-76;
   - Åström & Wittenmark, "Computer-Controlled Systems", for F0-79 (Åström & Murray is already registered).
   Editions and sections must be recorded when the dossier opens G1.
2. **FP8 source of record.** OCP OFP8 (S2 of F0-80) is proposed, with the Micikevicius et al. paper as background.
   - Once checked, `kE4M3` in `F0-80/lowp.hpp` should probably follow OFP8 (max 448, no infinities). That changes the E4M3 rows of `formats.out` and `dot.out` and the figure. The chapter flags the model explicitly.
3. **C++23 cross-check step.** `stdfloat_check.cc` needs `-std=c++23`, outside the runner's C++20 contract, so it runs from `run.sh`. Keep it this way, or extend `run_lab.sh` to honour a per-file standard.
4. **CPU model of GPU order.** F0-75 and F0-80 test a CPU model of the kernel's addition order instead of the GPU. The PTX (run R9) shows only `add.f32`, which supports the model, but bitwise equality on hardware is an open prediction. It should be run on a GPU when the faculty has one (F0-80 lab step 5).
5. **E3 size.** E3 says 2^28 floats; the course labs use up to 2^22 (sanitized CPU runs). The tolerance formula scales, and the worked answer shows how. A 2^28 run belongs on the GPU in Track E.
6. **Glossary overlaps.**
   - MA202 defines "Catastrophic cancellation" and MA301 "Truncation error". MA302 links to them; MA302 does not redefine them.
   - MA301's "Sample time (Δt)" and MA302's "Sample period (sampling rate)" are near-synonyms. Merge them if the Dean prefers one term.

## OS201
Source: [`chapters/OS201/NOTES.md`](chapters/OS201/NOTES.md)

### Decisions for the owner

1. **xv6 is not in the build container, and there is no internet.**
   - The course card's labs (6.1810 "util" and "syscall") are therefore untested in this build.
   - F3-08 gives their steps from memory, inside an unverified box, and quotes the P4 acceptance verbatim: "xv6 lab grading scripts pass."
   - Decide whether a later build gets the xv6-riscv source (pinned commit) and the RISC-V toolchain its Makefile expects, so the labs can be run and F3-08's file map checked.
2. **uni-rv stands in for xv6.** uni-rv is the university's own small RISC-V kernel, in machine mode with PMP, under 350 lines. It provides the forensic "frozen machine" (a real two-hart deadlock), the system-call path, and "add a system call". Decide whether uni-rv stays as a permanent teaching kernel, or only as a bridge to xv6.
3. **The course card asks for "an xv6 trace where two processes deadlock on locks".** The forensic lab instead uses a uni-rv trace with the same structure: rename and unlink on two harts, with directory and inode locks. Its bad interleaving is forced by `step` flags so that the run is reproducible, and the chapter says so. Decide whether the xv6 version is still required once xv6 is available.
4. **The answer key `solution_hartid.cc` sits in the lab folder** (`university/labs/F3-08`), so learners can see it. It is marked "ANSWER KEY" at the top. The lab runner needs it there to produce the expected-observation run. Decide whether answer keys should move to an instructor-only location.
5. **New lab run `spin_loop` in F3-05.** It is the objdump of `acquire` and was added late in this build. Its log's "machine" line mentions the emulated machine, although objdump runs on the host. This is harmless, but it is noted here.
6. **Glossary overlap with SP203.** SP203 is being written concurrently and now defines Deadlock, Spinlock, Critical section, Lock ordering, Wait-for graph and Mutex.
   - OS201's glossary does not redefine them, and the F3-05 chapter links to them.
   - If SP203 drops any of these, F3-05's links break. A rebuild would show it.
   - OS201 adds "Lock" and "Sleeping lock" as kernel-side terms.
7. **Analogy registrations.** The table above lists the mappings proposed for the guide's section 8.1 map.

## OS301
Source: [`chapters/OS301/NOTES.md`](chapters/OS301/NOTES.md)

### Decisions for the owner

1. **A1's literal acceptance test fails (F3-10).** Read literally, with conventional memory only, it fails. Conventional memory is 48.9 MiB below the `-m` value at both 256 and 512 MiB, because the firmware, the shell and the program hold boot-services and loader memory while A1 runs (`check_a1`).
   - Read as "memory usable after ExitBootServices", the test passes, 6.2 MiB below at both sizes.
   - The 6,368 KiB the firmware keeps for good is runtime 3,348 KiB, ACPI 2,124 KiB, reserved 512 KiB and the VGA hole 384 KiB.
   - The chapter reports both readings. The owner should choose one and reword the curriculum's test.
2. **No FPDT in this OVMF (F3-12).** The boot timeline uses host time stamps plus the TSC, three boots per configuration. FPDT content is left in an unverified box. Should a later edition use a debug or performance build of OVMF?
3. **P3's "boot an existing Linux image and break inside it" (F3-09)** was not done: there is no image and no internet. The page walk was done on the firmware's own page tables. Provide an image in the lab environment, or drop the step.
4. **A2's USB-stick boot (F3-13) and A1's USB run (F3-10)** are optional items. They are untested on hardware.
5. **A3's "as its first act, writes a known pattern to the framebuffer" (F3-15).**
   - The kernel stub prints "kernel entered" first and paints the framebuffer last, so the serial port shows progress even if the framebuffer address is wrong.
   - The acceptance test checks the painted pattern, not its order.
   - Accept this, or reorder `kernel.cc` (F3-14 lab, lines 171–176) to paint first.
6. **A3's direct map** covers max(top of RAM, 4 GiB) with 2 MiB pages. It was only exercised with 256 MiB of RAM.
7. **A4 (F3-17) cannot be built here.**
   - The chapter does what is possible and gives the build as an untested plan: it compares three packaged OVMF builds module by module, and compares the debug-port output of SeaBIOS and OVMF.
   - To complete A4, a lab image needs the EDK II and coreboot trees, iasl (acpica-tools) and network access for submodules, or prepared tarballs.
8. **F3-16 deviates from the curriculum's exercise.**
   - The exercise is "enroll your own PK/KEK/db in OVMF and sign your loader (sbsigntools)". sbsigntools and a variable-store tool are not installed.
   - The lab instead uses the distribution's pre-enrolled snakeoil store. It signs with a hand-written Authenticode signer (`sign.py`), which OVMF accepts.
   - Install sbsigntools and python3-virt-firmware (or efitools) so the original exercise can be done. `sign.py` should then be cross-checked against `sbverify`.
9. **F3-15 forensic variant 1.** A 64-byte AllocatePool between GetMemoryMap and ExitBootServices did *not* invalidate the map key on this OVMF. Variant 2, AllocatePages, did. Both are kept, and the chapter explains the difference as a hypothesis (pool served from an existing page).
10. **Verbatim curriculum quotes that contain banned words.**
    - "just" in P3's text (F3-09).
    - "trivial" in A4's acceptance text (F3-17).
    - They are kept verbatim inside quotations. Reword the curriculum, or accept them as quotes.
11. **Chapter length.** The chapters run 4,900 to 6,600 prose words, against a target of about 4,000 for L3. The extra length is mostly in the lab, forensic and answer sections. They could be trimmed if the owner prefers.
12. **F3-17 worked example.** It states that one driver and one PEIM name differ in count between the plain and AMD SEV builds, without naming them; finding them is a lab extension. Answer for the instructor: `CpuDxe` (DRIVER, 2 against 1) and `CpuMpPei` (PEIM, 2 against 1). This was checked with fvscan during authoring, not in a recorded run.

## OS302
Source: [`chapters/OS302/NOTES.md`](chapters/OS302/NOTES.md)

### Decisions for the owner

1. **Boot route.** The curriculum's B1 assumes UEFI with Limine, the A3 route. Neither Limine nor OVMF was available in the container, so OS302 boots with **Multiboot v1 through QEMU's `-kernel`**. The kernel is a flat binary with an a.out-kludge header, and its 32-bit entry code switches to long mode itself. This also means:
   - F3-23 has no GOP framebuffer. It programs QEMU's `-vga std` (Bochs DISPI) instead, which works only under emulation.
   - F3-24 finds the RSDP by a memory scan, not from the UEFI configuration table.
   - Please confirm this route, or schedule a Limine/UEFI variant.
2. **TCG only.**
   - Every timing number comes from TCG: the F3-25 sleeps, calibrations and monotonic-read costs, and the F3-22 stress-test duration.
   - The B8 KVM 1 % test is **not run**.
   - Decide whether B8 test 1 counts as passed with "TCG logged, KVM pending".
3. **x2APIC untested.** B7 test 3 passes as worded, but the x2APIC branch never executed. The same applies to the XSDT path. Running B7 needs a KVM host, or a QEMU version whose TCG implements x2APIC.
4. **Own font.** F3-23 draws its own 5×7 font, `font5x7.h`, by hand. Lower-case letters use the upper-case shapes. No third-party font is included, so there is no licence question. Replace it if a fuller font is wanted.
5. **Simplifications to carry into OS303.** Each chapter states these:
   - Page-table pages are never freed (F3-21).
   - The heap's large region uses a bump pointer, so freed virtual ranges are not reused (F3-22).
   - The heap has no lock and no per-CPU caches (F3-22).
   - The panic path does not stop other CPUs (F3-23).
   - The timer queue is a sorted array of 32 entries (F3-25).
6. **Shared files across chapters.** Later chapters compile earlier chapters' sources by relative path. Examples are `F3-18/arch.h`, `F3-18/kprint.cc` and `F3-23/log.cc`. Helpers were added to `F3-18` during later chapters:
   - `%.Ns` precision in `kformat.h`;
   - `sti_hlt`, `pause`, `save_flags_cli` and `restore_flags` in `arch.h`.

   All eight labs were rerun after these changes.
7. **Glossary.**
   - `glossary.json` has 70 entries.
   - Terms that already exist in other courses reuse their exact names so that build.py merges them: "Kernel (operating system)", "Freestanding implementation", "Exception", "Interrupt", "Page table", "Memory leak", "Use after free" and "Deadlock".
   - The F3-22 jargon box spells "Use-after-free". The glossary entry uses "Use after free", the same term as SP201, so the anchor `#gl-use-after-free` is shared.
8. **An open forensic detail.** In the F3-24 no-EOI run, QEMU's `info irq` counted 4 raises of IRQ 4, but the kernel logged 3 serial interrupts. The chapter does not explain the extra one, and says so. It does not affect the diagnosis.

## OS303
Source: [`chapters/OS303/NOTES.md`](chapters/OS303/NOTES.md)

### Decisions for the owner

1. **Kernel placement.** The OS303 kernel is a standalone, low, identity-mapped kernel. It is loaded at 1 MiB with 0–4 GiB identity-mapped, and the user half is PML4[1]. OS302's higher-half kernel was not reused, to keep the labs self-contained. Should OS303 be rebased onto OS302's kernel?
2. **One kernel from five folders.** Later chapters' sources are compiled into the F3-26 build. This keeps one binary, but F3-26's lab depends on files in F3-27 to F3-30.
3. **Reduced SMP sizes and TCG only.** See the deviations above. The tests should be re-run with KVM on a host that has it.
4. **"Misaligned length" interpretation** (F3-29).
5. **Fuzz duration** (F3-30). An hour of coverage-guided fuzzing was not done. The course project asks students to measure it.
6. **Single-thread processes** (F3-30). Multi-threaded processes would need per-process TLB shootdown at exit; the chapter explains this.
7. **Scheduling priorities** are left to the F3-26 mini-project. The scheduler is plain round robin.
8. **No FPU/SSE state** is saved at a switch. All kernel and user code is built with `-mgeneral-regs-only`. Chapter text warns about this.
9. **Outputs vary per run.** Under SMP these vary: CPU masks, which CPU pair deadlocks, tick counts and read counts. The prose numbers were refreshed against the last run and say that they vary; they must be refreshed whenever the labs are re-run.
10. **QEMU Multiboot needs a 32-bit ELF**, hence the objcopy step. This is a QEMU constraint, not part of the kernel design.

## OS304
Source: [`chapters/OS304/NOTES.md`](chapters/OS304/NOTES.md)

### Analogy proposals (F3 school world). Owner approval needed.

The registered mappings are used as they are: the file system as the library catalogue and shelves, the process as a class with its own room, the system call as the office window, and deadlock as two students each holding a book. Proposed new mappings:

| Concept | Proposed school mapping | Chapter | Where it breaks (stated in the chapter) |
|---|---|---|---|
| VFS | the front desk that routes request slips to the right library | F3-31 | A mounted file system does not know its mount point. |
| Block cache | the librarian's trolley of recently used books | F3-31 | The cache can hold the only correct copy. |
| Initial RAM disk | the box of books that arrives with the principal on day one | F3-31 | The archive is served in place, not unpacked. |
| FAT / cluster chain | the ledger of "continued on shelf n" lines | F3-32 | Two FAT copies can disagree; clusters have no owner field. |
| LFN entries | extra title cards with a check number in front of the old card | F3-32 | The tie is only a checksum. |
| Block group / bitmap / inode | a floor with its free-shelf chart and drawer of record cards | F3-33 | The bitmap is a second record of the same fact. |
| Pipe | the message tube between classrooms | F3-34 | Ends are shared after fork. |
| Shared memory | the whiteboard in the wall between two rooms | F3-34 | Memory ordering. |
| Event | the bell | F3-34 | A wake-up goes only where the code sends it. |
| Shell | the receptionist who takes spoken requests and hands work to helpers | F3-35 | The shell copies itself (fork). |
| C library / system-call layer | the school's standard forms, and the office-specific version of the forms | F3-35 | The ABI below the names must match too. |

Analogy share: stories and analogies stay inside the hook, Layer 1 and the jargon "Analogy" parts. This is estimated at well under 10% of the prose, but it was not measured precisely.

### Decisions for the owner

1. **A separate 32-bit Multiboot mini kernel for the labs (F3-31, reused in F3-35).** The learner's OS302/OS303 kernel is x86-64 with paging and user mode. To keep every line about files, OS304's in-kernel labs use a small 32-bit kernel with no paging and no user mode, booted by QEMU's built-in Multiboot v1 loader with the initrd as a module. The VFS, tarfs and shell code are written to move into the learner's kernel.
   - Decide whether to keep this, or to require labs to build on a reference OS303 kernel (none exists in the repository yet).
2. **Host-side drivers and models.** FAT32 and ext2 run as host C++ over image files, with the host tools as judges. The pipe and wait-for-multiple objects run as host-thread models. The B17 tests run on the host Linux kernel as the reference behaviour. Every chapter states this in an untested box and in the Lab Verification paragraph.
   - Decide whether this is acceptable for the "real code runs" rule at L4, or whether the in-kernel ports must be built (a large job) before release.
3. **Kernel-mode shell for the serial transcript test (F3-35).** The P exam form (a scripted serial session against a reference transcript) is demonstrated with `kshell` in kernel mode. Decide whether the exam must require a user-mode shell.
4. **Pass codes counted as success.** `build.py`'s `run_ok` accepts any log with "exit code:". Expected failures are documented in `note:` lines; these are e2fsck exit 4, watchdog exit 3, timeout 124, the bcache self-check exit 1, and isa-debug-exit 33 as pass. Consider a formal `expected:` field in the log format so the builder can distinguish expected failures from real ones.
5. **"fsck is angry" design.** The course card asks for "an ext2 image after a test run; find the bitmap-update ordering bug". It is implemented as a power-cut harness with a driver whose block-bitmap update was moved after the directory entry, then a second run without fsck. The evidence pack is the harness write log, e2fsck's multiply-claimed report and debugfs. The answer key contrasts it with the correct order's leak-only report. Please confirm this matches the intended exercise.
6. **Forensic faults are injected by scripts.** Each fault is injected by `sed` or a small Python script in `run.sh`, so the answer is not visible in the listing files. The answer keys describe the injection.
7. **128-byte inodes in the ext2 lab** (`mke2fs -I 128`). These keep the driver small. mke2fs warns that they cannot handle dates beyond 2038 and are deprecated; the chapter mentions the warning. A 256-byte-inode variant could be a later extension.
8. **`minish` deviation.** `$?` is expanded when the line is read (documented in the source and the chapter). Decide whether to fix this before release; it is a lab extension now.
9. **The FAT32 driver writes long-name entries for case-only names** instead of using the byte-12 case flags. The chapter documents this as an unverified difference.
10. **New sources named** (all title only, gate G1):
    - Card, Ts'o and Tweedie;
    - Poirier;
    - Carrier, "File System Forensic Analysis";
    - Ganger, McKusick, Soules and Patt, "Soft Updates";
    - Plauger, "The Standard C Library";
    - the System V psABI and AAPCS64;
    - dosfstools and e2fsprogs manual pages;
    - the Linux-NTFS documentation.

    Several are tier 2 or 3 and are not yet in the guide's source registry. Please add them, or replace them with registry items.
11. **Glossary qualifiers.** These avoid merging with other courses' meanings: "Shared memory (between processes)", "Event (synchronisation object)", "Pipeline (shell)", "Directory entry (ext2)", "Directory entry (8.3 short name)" and "Signal (POSIX)". "Inode", "Superblock", "Semaphore", "Shell" and "Block device" are unqualified, and will merge with any identical term from other courses.

## OS305
Source: [`chapters/OS305/NOTES.md`](chapters/OS305/NOTES.md)

### Decisions for the owner

1. **Lab board** (H1–H4). It needs:
   - a Cortex-M part with public reference manuals;
   - a USB device port (H3);
   - a documented flash layout and recovery path (H4).

   No board is named in the chapters.
2. **Debug probe and host software:** OpenOCD or pyOCD, and GDB. The container's GDB is x86-only, so the chapters use LLDB.
3. **Sensor for H2 and exam P:** an I2C temperature sensor with a datasheet. The emulated part is QEMU's tmp105 model.
4. **Logic analyser** for H2 and H3 captures.
5. **RTOS for H4:** Zephyr or FreeRTOS, and the version. Table 1 in F3-40 must then be checked against it.
6. **USB route for H3:** TinyUSB first, then our own stack, as the curriculum suggests. Also needed: the USB vendor and product IDs to use in labs. The model uses placeholder values; a policy is needed for anything plugged into a real PC.
7. **Boot loader for H4:** MCUboot, which strategy and which version, or the course's own loader. Also decide the signature algorithm, how private keys are handled (the teaching keys are derived from fixed seeds), and whether a target-side signature check becomes a required lab.
8. **EC and BMC (H5):** which Chromium EC version learners read, whether the course supplies a BMC firmware image for QEMU, and whether the optional "tiny EC on the H2 board" becomes required. F3-42's mini-project moves it to the emulator.
9. **Sources:** all D-sources are "title only, not opened" (dossier gate G1 open). The Source Researcher must confirm editions and sections.

## OS401
Source: [`chapters/OS401/NOTES.md`](chapters/OS401/NOTES.md)

### Decisions for the owner

1. **Approve publication with unverified boxes** (AH-19): every chapter has them; F3-47 … F3-49 rest on
   formats checked only by libblkid/file and our own tools.
2. **Acceptance tests that need tools this build lacked** — owner to provide an environment with:
   a Linux kernel that can mount images and a QEMU-kill harness (FS1, FS2 interchange and power cuts);
   exfatprogs and a Windows VM (FS4); xorriso and a current installer ISO, plus OVMF for the UEFI entry
   (FS5); a Windows VM to make an NTFS test image (FS6). Until then the chapters say plainly which
   clauses were not run.
3. **UDF**: the course card's goals say "ISO 9660/UDF"; FS5 makes UDF optional. F3-48 does not cover
   UDF (mentioned only as the mini-project's optional extension). Decide whether a UDF section or a
   separate chapter is wanted.
4. **Course project scale**: "FS2 passing 500 power-cut runs" is met in this build by 500 replayed
   cut points of a recorded write stream (F3-45 `harness_random`), not by 500 killed QEMU instances. The
   mini-project rubric asks for the QEMU version; confirm that this is the intended bar.
5. **Exam P image**: the F3-44 forensic image (careless writer, e2fsck exit 4, "deleted/unused inode
   12") is designed to be the exam's "provided image"; the Exam Writer should regenerate a fresh one
   with another seed so answers cannot be copied from the chapter.
6. **ext2 writer gaps** (F3-44): no triple indirect blocks, no rename over an existing name, no
   directory rename/rmdir, no long symlinks. FS1 mentions files above 4 GiB; decide whether that clause
   must be met in the chapter's own code or left to students.
7. **NTFS generator**: `ntfsgen.cc` exists only because no Windows image was available. Once one is,
   replace the generated image in the lab with a checked-in Windows-made image (small, licence-clean)
   and keep the generator only for the damage experiments.
8. **Analogy registrations (proposals to the Dean, guide 8.2)** — used in the chapters and marked
   "proposed" in each meta comment:
   - journal = the librarian's day book (F3-43, F3-45); commit block = the tick under a finished entry
     (F3-45); flush = the porter's signed delivery note (F3-43); write stream = the porter's delivery
     list (F3-44);
   - extent = a shelf range written on one card; checksum = the seal on each catalogue drawer (F3-46);
   - allocation bitmap = the free-place chart; NoFatChain = a card saying "all volumes stand side by
     side from place 5" (F3-47);
   - ISO 9660 = the printed, bound catalogue that is never changed; Rock Ridge and Joliet = margin notes
     for two kinds of reader; El Torito = the caretaker's "start here" page (F3-48);
   - NTFS = the neighbouring school's library with unpublished rules; MFT = its master register;
     update sequence = the form number at the foot of every page of a two-page form (F3-49).
   Each chapter lists where its analogy breaks.
9. **Length**: chapters are about 5,000–7,700 words including jargon box, tables, answers and keys
   (F3-43 is the longest because it carries the course's crash model). Within L4 range per the brief,
   but the Editor may want to trim F3-43.

## OS402
Source: [`chapters/OS402/NOTES.md`](chapters/OS402/NOTES.md)

### Unverified boxes (what an owner or Source Researcher must check)

No official document could be opened in this build. Every D-tag is "title only — pending verification (dossier gate G1 open)". The boxes below name the specific claims that most need checking.

#### F3-50

- The exact range of negative return values Linux C libraries treat as errors (−4095..−1 as commonly stated).
- The AArch64 and RISC-V runs used QEMU user mode, not hardware (UoH).

#### F3-51

- Which Linux system calls restart after an `SA_RESTART` handler and which always fail with EINTR.
- The organisation, build and result format of libc-test and the Open POSIX Test Suite.
- U1 inside the learner's OS: untested.

#### F3-52

- PIE and interpreter placement rules, and the auxiliary-vector entries beyond the five that U2 names.
- Relocation type names and numbers. They are deliberately not given; take them from the psABIs or `elf.h`.
- `arch_prctl(ARCH_SET_FS)`, `CLONE_SETTLS` and user-mode `WRFSBASE`.
- TLS sequences and the variant I layout on AArch64 and RISC-V were not compiled here (UoH).
- U2 inside the learner's OS, and the musl/mlibc dynamic linkers: untested.

#### F3-53

- **The `.eh_frame_hdr` layout and encoding values decoded by `phdr_walk.cpp` were written from memory.** They are cross-checked against readelf for one program only (6 = 6).
- Which lookup interface LLVM libunwind uses, and libgcc's fallback before glibc 2.35.
- C++ runtime build options for a new triple (none given).
- QEMU-only runs (UoH).
- U3 inside the learner's OS: untested.

#### F3-54

- Why Clang delegates linking to `gcc` for an unknown OS. This is inferred from `-###` output.
- GCC/Binutils target files and options: no compiler was patched.
- The recipe formats of SerenityOS Ports, xbstrap and pkgsrc, and the CMake/Meson cross-file syntax.
- No program ran inside a myos kernel (UoH).
- U4 (second machine, ported programs' test suites): untested.

#### F3-55

- GCC bootstrap details and LLVM multi-stage builds. No bootstrap was run.
- Two inferences from the trace: why cc1plus calls `readlink` so often, and why GCC calls `sysinfo`.
- The forensic answer key's statement that BFD seeks before each section write. It is marked in the text as inferred from the trace.
- U5 in QEMU or on the spare PC: untested (UoH).

#### Smaller inference, marked in the text

- F3-53 answer 5: why the sanitizer build has more FDEs (9 vs 6). This was not checked.

### Decisions for the owner

1. **The forensic of the course card versus the host's unwinder (F3-53).**
   - The card says "find the missing dl_iterate_phdr support". On this host, libgcc's unwinders (all three architectures, shared and static) call glibc's `_dl_find_object` instead (R3 of F3-53).
   - The lab therefore reproduces the defect by interposing `_dl_find_object` with a port-style table that does not follow `dlopen`. The answer key states that the diagnosis is the same for a `dl_iterate_phdr`-based unwinder.
   - Options: accept this, or reword the card ("…find the missing object-lookup support (dl_iterate_phdr or _dl_find_object)").
2. **Each other chapter has its own forensic case.** The card names only one forensic, and it is used in F3-53. The other cases were chosen to fit each chapter:
   - F3-50: a compat layer returning +errno;
   - F3-51: duplicated log lines after fork;
   - F3-52: surprise interposition;
   - F3-54: a non-reproducible package from `__DATE__`/`__TIME__`;
   - F3-55: an assembler failure caused by a missing `lseek`.
3. **The teaching "myos" toolchain uses the Linux-compatible ABI (F3-54).** The sysroot's C library uses Linux system-call numbers (F3-50 path 2), so `hello_myos` can be run in the build container. The chapter says so. If the owner prefers the course to model path 1 (own ABI), the run would become "build only, untested".
4. **A wrapper instead of a patched compiler (F3-54).** No GCC, Binutils or LLVM was patched (time and scope). The wrapper plus `ld.lld` shows the sysroot discipline and avoids the observed driver trap. Patching is left to the milestone with unverified boxes.
5. **The stage comparison in miniature (F3-55).** `minicc` cannot compile itself. The lab compares the outputs of two differently built compilers instead of compilers built by compilers, and the text says so explicitly.
6. **"Memory statistics returning to baseline" (F3-52).** A naive before/after test fails on glibc without any leak: the heap grows once during the first cycles. The listing defines the baseline as "no growth over the last three batches of 10,000". The owner may want this definition written into the U2 acceptance wording.
7. **Source registry.** F3-54 cites "Reproducible-builds documentation / SOURCE_DATE_EPOCH specification" (D5), which is not in the curriculum's reading list. It is proposed for the registry.
8. **Exam P.** The card's P exam ("rebuild a ported program inside your OS") has no separate material beyond F3-54's ports and F3-55's lab steps. The owner may want a dedicated exam brief.

## DR301
Source: [`chapters/DR301/NOTES.md`](chapters/DR301/NOTES.md)

### Decisions for the owner

1. **The lab kernel is 32-bit with paging off and uses the 8259 PIC, not the IOAPIC or MSI.**
   - This keeps every DMA address physical = virtual and reuses one small kernel for seven labs.
   - The student's OS302 kernel is 64-bit with paging. The chapters say where a real kernel differs (physical addresses for DMA, MSI-X in C7).
   - Alternative: build the labs on the OS302 lab kernel instead. That needs coordination with OS302's author.
2. **The random I/O tests run 4000 operations per configuration over a 1 GiB address range, not every block of 1 GiB.**
   - Each read is compared in full, and the host checks the whole written set plus a sample of unwritten blocks.
   - A full 1 GiB pass under TCG would take minutes per run.
   - Please confirm this meets "random I/O test over 1 GiB" or set a required operation count.
3. **All drivers poll; there are no interrupts for virtio or NVMe.** AHCI uses the PIC line. C6 allows polling until C7, and C4 and C5 do not mention interrupts for virtio. MSI-X is left to DR302 (C7).
4. **C6 runs on one CPU.** "1, 4 and 8 queues on 8 CPUs" is met for queue counts but not for CPUs. The course project asks for it on the student's SMP kernel.
5. **The C6 and course-project root is a read-only USTAR archive, not a mounted file system.** It is checked file by file against the host's `tarfile`. The project rubric requires ext2 mounted through the student's VFS.
6. **The course forensic ("the slow boot driver") uses a boot-trace CSV produced by the lab kernel itself, not a Windows ETL trace.**
   - It comes from two QEMU boots (healthy port probe against reset-every-port with a 1 s timeout), formatted like the driver tables of `DRIVERS.html`.
   - The chapter says so in a note box, and connects it to `DRIVERS.html` (List 1, `amdsata` 0.4 → 0.4 ms, `kernel_to_smss`) and to curriculum 4.3 (amdsata/storahci → C5).
   - If the owner wants a real ETL-derived CSV, one must be supplied.
7. **C4's FAT32/ext2-on-virtio test was not run.** It depends on the student's B14–B16 work (F3-31 to F3-33, OS304), which is not a DR301 prerequisite.
   - The chapter's prerequisites line names OS304 for that test only.
   - Consider adding OS304 as a co-requisite, or moving that acceptance test to the course project.
8. **AHCI depth 32 (NCQ) is a mini-project, not in the lab.**
9. **No spare-PC runs.** Every "optional spare PC" path is untested, and every chapter and log says "untested on hardware".
10. **QEMU's file chardev lost bytes during F4-03's development.** Feeding the paste test from a file chardev lost input bytes, so the paste test uses a socket chardev with a Python feeder (`paste_feed.py`) instead. F4-03 names the socket chardev but does not discuss the file-chardev loss. The loss was observed and not investigated, so it could be a QEMU limitation or a usage error.

## DR302
Source: [`chapters/DR302/NOTES.md`](chapters/DR302/NOTES.md)

### Decisions for the owner

1. **F4-10: the 5 % loss is induced in the guest** (a seeded drop of TCP segments by a hash of sequence number and retransmission count: receive side in `net.cc`, transmit side in `tcp.cc`), not by a QEMU network filter. From memory, QEMU 8.2's filters (`filter-dump`, `filter-buffer`, `filter-mirror`, `filter-redirector`, `filter-rewriter`, `filter-replay`) cannot drop a percentage of packets; this was not verified. Please accept or provide a host-side loss method (for example a TAP device with `tc netem`, which needs privileges the build container lacks).
2. **F4-10: host-to-guest ping is impossible with slirp**; the chapter shows TCP through `hostfwd` instead. A TAP or socket network backend would allow it.
3. **F4-10: the echo server runs in the kernel**, with no sockets and no DNS; the acceptance test says "user space". This follows from the DR302 lab kernel having no user mode. Either accept, or move the user-space part to the course project.
4. **F4-10: RTO minimum and TIME_WAIT are 200 ms**, far below RFC 6298's 1 s minimum and the usual 2 MSL, so that the lab finishes in minutes under TCG. The chapter labels both as lab choices.
5. **F4-11: no AML interpreter.** The lab is a declaration-only walker and a static `_PRT` reader; porting ACPICA or uACPI is the mini-project. iasl was not available, so the namespace was not cross-checked with a disassembly.
6. **F4-09: no hub driver and no B15 tests**; the stick is read at the FAT32 level by the lab's own minimal reader. **No firmware hand-off** (USB legacy support capability): with the IOMMU on, a DMAR read fault at 0x3fdec60 appeared when translation was enabled. The likely cause is SeaBIOS's xHCI rings still running; this was not proven.
7. **F4-08: NVMe MSI-X per queue is not done**; MSI-X is demonstrated on qemu-xhci. The NVMe part needs DR301's F4-07 driver extended. Consider making it the F4-08 mini-project (it is lab step 6).
8. **F4-12: the source is a generated tone**, not a WAV file read by the kernel (the lab kernel has no file system). The sample comparison is stronger evidence than a file read would add, but the acceptance wording says "a WAV file played by your kernel".
9. **F4-13 uses a mock TPM** (see above). Please decide whether the chapter may be published before a swtpm rerun.
10. **Course forensic labs.** The course plan lists "Packets vanish at 5 % loss" (F4-10's forensic: a retransmission timer that is not re-armed, with a tshark capture) and "DMA to nowhere" (F4-08's forensic: an IOMMU fault report). Both are built as described. The plan's exam item "annotate a USB enumeration capture" can use F4-09's `capture.out`.

## DR401
Source: [`chapters/DR401/NOTES.md`](chapters/DR401/NOTES.md)

### Decisions for the owner

#### 1. Tier list inconsistency (F4-15)

**Issue.** The curriculum's tier list in section 5 does not match the 5.2 table. F4-15 Layer 3 notes this.

**Current handling.** The course uses the 5.2 table, and "signed firmware" is mapped to tier 6 or to the host-interface tier.

**Owner's call.** Confirm the mapping, or fix the curriculum.

#### 2. edu device tier (F4-15, F4-16)

**Issue.** The F4-15 forensic answer classifies edu as tier 2: the QEMU project is its maker, and its documentation exists but was not opened. F4-16 deliberately treats it as tier 5 for practice and says so.

**Owner's call.** Confirm that this pedagogical choice is acceptable.

#### 3. QEMU USB keyboard descriptor (F4-21)

**Issue.** The bytes extracted from the QEMU 8.2.2 program file contain "95 06 06 75 08". With the extra 0x06, the input report is 34 bits. Without it, the report is 64 bits.

**Status.** Unresolved: the build had no guest USB stack to check what QEMU actually sends. The chapter says not to cite it as a QEMU bug. The incomplete mouse candidate at file offset 13113712 is also unexplained.

**Owner's call.** Have a Linux guest dump of the descriptor taken, then update the chapter. The `hid_dump` run is designed to exit 1 on this descriptor, so `run.sh` would need changing if the bytes turn out to be wrong.

#### 4. Zero reads with memory decoding off (F4-17, F4-18)

**Observation.** In this build, QEMU returns 0, not all ones, for reads with memory decoding off. The curriculum says "all ones" after removal.

**Current handling.** The driver checks the ID register instead, which works for both values.

**Owner's call.** Verify against the PCI specification and real hardware.

#### 5. Constructed and assumed exhibits (F4-15)

**Issue.** The INF excerpt is constructed (labelled CONSTRUCTED in the log). The Windows hardware-ID list format follows the curriculum's forms but is unverified.

**Owner's call.** Accept, or supply a real (licence-clean) exhibit.

#### 6. Register tables cite undocumented sources (F4-17)

**Issue.** M2's third acceptance test wants every register to cite a document section. F4-17's register table cites F4-16's observed specification and the unopened QEMU documentation.

**Owner's call.** This needs the Source Researcher (gate G1).

#### 7. Invented constants (F4-22)

**Issue.** The governor and thermal models use invented constants. They are labelled in the code, the outputs and an unverified box.

**Owner's call.** Accept as teaching models, or replace them with platform values when a spare PC is available.

#### 8. Timing-dependent outputs

**Issue.** Several labs depend on timing:
- F4-16 trace totals ranged from 141 to more than 3600 accesses across this build's runs;
- F4-17 and F4-18 poll counts;
- F4-20 stage ticks;
- F4-22 CPU percentages and timer intervals, whose scatter under TCG reversed between two runs.

Each rebuild of the outputs can change the quoted numbers.

**Owner's call.** Either accept the "saved run" wording, or make these labs deterministic. QEMU's `-icount` would do that, but it changes the halt/CPU-time measurement in F4-22, which depends on real host time.

#### 9. Course-level items not written as separate files

**Coverage.** The course forensic ("Unknown device": hardware IDs, an INF excerpt, a boot trace → device record and tier) is F4-15's forensic lab. The project M2 is carried by F4-17 and F4-18.

**Not written.** The exams (Q, F, P) were not asked for as files and were not written.

**Owner's call.** Say whether they are wanted.

#### 10. Real hardware

No real hardware was available. C12, C14 and C15 are complete only in their QEMU, model or host parts, and each chapter's Verification paragraph says which acceptance lines remain.

## DR402
Source: [`chapters/DR402/NOTES.md`](chapters/DR402/NOTES.md)

### Decisions for the course owner

1. **F3-18 `kformat.h` treats `%ll` as `%l`** (any number of `l`s means `long`). Also, F4-23's `neutral_tests.cc` passes `0xffffffff80000000ul` to `%lx`.
   - Both are harmless on LP64. On ILP32 the test crashes, with SIGSEGV at 0xffffffff on little-endian CPUs and 0x80000000 on big-endian ones.
   - `labs/F4-30/neutral_fix.py` writes fixed copies and does not edit the originals; F4-30's "fixed" run passes on all five 32-bit CPUs.
   - **Decision:** adopt the fix in OS303's `kformat.h` (count the `l`s) and F4-23's test (`%llx`, `ull`)? Also consider `__attribute__((format(printf, …)))` on `ksnprintf`/`kprintf`.
   - The chapters currently present the unfixed state as the forensic "before".
2. **DR301's virtio code (F4-05 `virtio.cc`) is coupled to PCI.** `vio::init` takes a `PciAddr` and walks PCI capabilities.
   - D3's acceptance test asks that the C4 virtio-blk code be "shared, not copied". That needs a transport interface below the device drivers.
   - F4-26's lab uses a separate small virtio-mmio driver. F4-26 says this and makes the interface its mini-project.
   - **Decision:** refactor DR301 with a `virtio::Transport` interface?
3. **F4-23's `fdt_tool` ignores `/aliases`.** Checklist row 6 prints "no usable stdout-path" for a Pi-style devicetree (F4-27 R2), and F4-27 uses this as a teaching point and lab step. **Decision:** fix in F4-23 or keep it?
4. **F4-23's neutral-test skip message** says "(ACPI)" even when the platform uses a board table (F4-27 `nodt`). Wording fix only.
5. **Non-atomic 64-bit atomic helpers** in `labs/F4-30/port32.cc` are single-CPU only, and the chapter says so. An X1 port on SMP must lock or avoid 64-bit atomics.
6. **Ticket lock under vCPU oversubscription** (8 vCPUs on a 4-CPU host): wall time varies by two orders of magnitude with host load.
   - F4-26 `smp8_slow` ran in 278 ms in the final run, and in about 23 s and >30 s in earlier runs.
   - F4-29 uses `rounds=500` for 8 harts; 5000 rounds hung for minutes in an early run.
   - CI should keep round counts small. `smp8_slow` is an observation, never a pass condition.
7. **One unexplained early hang:** F4-27 without a devicetree, during host load. It did not reproduce in 46 later runs. The lab now boots each configuration 5 times and reports the count. The hypotheses are listed in the chapter; there is no finding.
8. **SBI SRST returns −2 on QEMU `sifive_u`** with the bundled OpenSBI v1.3, so those runs end at the time limit (124). The pass condition is the "D6 ok" line.
9. **QEMU 8.2.2's virt machine defaults to legacy virtio-mmio** (version 1). The labs pass `-global virtio-mmio.force-legacy=false`, and the driver refuses version 1 clearly (F4-26 `legacy`).
10. **D4 and D7 acceptance tests require real boards.** This build rehearses them on `raspi3b` and `sifive_u` only. Someone with a Pi 3 and a VisionFive 2 should run them and record the logs.
11. **Analogy registry (school world).** Mappings proposed in these chapters, for the registry owner to accept or change:
    - "building plan on the principal's desk = devicetree" (F4-23, F4-27).
    - "service hatch in the caretaker's office = SBI ecall" (F4-28).
    - "mailbox watched by a waiting teacher = spin-table release address" (F4-27).
    - "fire-alarm timer = watchdog reset" (F4-27).
    - "internal post where notes to different rooms may overtake each other = weak memory ordering" (F4-26).
12. **Glossary merges.** Several DR402 terms share a slug with entries from other courses, and `build.py` merges entries with identical term strings:
    - Devicetree and Phandle (DR403).
    - Hart and Sv39 (OS201).
    - Per-CPU data (OS303).
    - Interrupt storm (OS305).

    DR402's "phandle" was renamed to "Phandle" so it merges with DR403's entry instead of producing a duplicate id. The owner may want one wording per merged term.

## DR403
Source: [`chapters/DR403/NOTES.md`](chapters/DR403/NOTES.md)

### Decisions for the owner

1. **Devicetree reader.** DR403 uses its own `F4-31/fdt.h`, not DR402's reader. Merge or keep separate.
2. **Course devicetrees are not vendor trees.** `F4-36/raspi3b.dts` and `F4-35/soc.dts` are written for the course. The `brcm,...` compatibles are unverified; the files must not be used on real boards.
3. **Non-standard bindings.** The pin request properties `pins` (F4-35) and `dr403,sd-pins`, and `/chosen/dr403,sd-selftest`, are course inventions, not the standard pinctrl binding (`pinctrl-0`, `pinctrl-names`). Accept, or rewrite to the standard binding.
4. **SoC model behaviour.** In F4-35's `socsim`, a gated block reads 0 (the forensic "device reads all zeros"). That is a modelling choice; real SoCs may hang, raise an external abort or return stale data.
5. **DT only.** The kernel uses the devicetree, never ACPI; F4-33 discusses ACPI on Arm but no ACPI path was written.
6. **D9 platform.** F4-36 uses QEMU raspi3b's SDHCI, not `sdhci-pci` on virt as D9's first acceptance test says. Decide whether to add the PCI route (the kernel has no PCI on Arm).
7. **D10 substitute.** F4-37 uses QEMU raspi3b as "the second platform" and QEMU's direct kernel boot. It is a different SoC layout but not a board and not firmware.
8. **Fictional boards.** F4-34's bring-up exercise scores fictional boards (no real product data). Real boards are named only inside unverified boxes.
9. **Glossary overlaps.** Not duplicated, only linked: Driver model, Probe (driver) (DR301); Compatible string (DR401); Configuration table (UEFI), GUID, UEFI, ACPI, Reset vector, Memory map (UEFI) and E820 (OS301); Clock tree, Clock gating (clock enable), GPIO, Errata sheet (HW303); GIC, MMIO, PIO, Cache coherence (for DMA) (HW204); Sector and LBA (HW205); Semihosting (OS305); UEFI application (SP301). F4-35's jargon term "Clock gating" and F4-36's "Programmed I/O (PIO)" are not in `glossary.json` because of these. PSCI, SMC and Devicetree are defined here; if DR402 also defines them, one of the two must be removed (duplicate glossary ids). "Devicetree" and "eMMC" replace the curriculum seed entries of the same name.
10. **Diagram types.** Guide 9.2 types used: boot chain timeline (F4-32), driver stack (F4-35).

## DR404
Source: [`chapters/DR404/NOTES.md`](chapters/DR404/NOTES.md)

### Findings of this build that the owner may want to know

1. **QEMU 8.2.2 stores SVM's VMRUN failure code as 0x00000000FFFFFFFF.** The header defines it as −1 (64 bits), and QEMU writes only the low 32 bits. The hypervisor compares the low 32 bits, and F4-40 says so. Behaviour on hardware is unchecked.
2. **QEMU 8.2.2 TCG did not apply nested paging while the guest had paging off.** `npt_probe` shows it, and the problem made the first F4-41 design crash the host. The workaround is to enter the guest at its 64-bit `long_mode` entry, with page tables built by the hypervisor. This became the F4-41 forensic lab. Someone should check a newer QEMU and the APM.
3. **The guest's calibrated TSC under F4-41 is wrong.** It reads 2.10–2.48 GHz, while the host measures about 2.05–2.10 GHz. The cause is late and dropped virtual ticks. The chapter uses this as its "lost ticks" teaching point.
4. **The build container is a KVM guest** (signature KVMKVMKVM, kvm-clock offered, virtio devices). F4-38 and F4-39 use it as a live example.

### Decisions for the owner

1. **SVM only.** Should a VMX/VT-x version of F4-40 be required? It needs a KVM host with nested VMX or Intel hardware, and neither is available to this build.
2. **Can the V2 project be graded on QEMU TCG alone**, or must it be on nested KVM or hardware, as the milestone says?
3. **The 64-bit guest entry in F4-41** depends on a QEMU behaviour that we believe is a QEMU limitation. If a newer QEMU or the APM settles the question, F4-41's Layer 3 and its forensic lab need revising.
4. **V1's kvmclock driver and the virtio-rng and virtio-console drivers** are left to the student (lab steps) and are not built here. Is that acceptable, or should the next build add them? They need KVM to be tested.
5. **The labs depend on each other** (F4-40 and F4-41 build from `../F4-39` and `../F4-40`). Keep this, or copy the shared files into each folder?

## DR405
Source: [`chapters/DR405/NOTES.md`](chapters/DR405/NOTES.md)

### Decisions for the owner

1. **New analogy mappings** in the F4 world ("the school and its visitors"). Proposed and used in these chapters, not yet in the guide's registry:
   - **GPU** = a visiting theatre company with its own crew.
   - **GPU kernel driver** = its interpreter (the registered driver = interpreter mapping).
   - **Command buffer / AQL packet** = a running order.
   - **User-mode queue (ring)** = the order board at the stage door.
     - **Doorbell** = the bell on the board. This is consistent with DR301's NVMe "bells on the trays".
   - **Firmware microcontrollers** = the company's stage managers, who work only from sealed, signed scripts.
   - **Fence** = the "done" stamp.
   - **Display** = the hall's projector, which paints a slide line by line.
     - **Page flip** = changing slides.
     - **Vertical blank** = the dark moment between showings.
     - **Tearing** = swapping the slide mid-projection.
   - **virtio-gpu resource** = a numbered slide in the company's slide store.
     - **Backing memory** = the teacher's own drawing.
   - **Expansion ROM** = the instruction booklet taped to the first crate.
     - **ROM BAR enable** = the reading lamp on the crate.
   - **Feasibility study** = the drama club's report on the scenery lift.

   Please accept or replace them.
2. **Course project options.** G2 (F4-43) and G5 (F4-46) each have a mini-project section with a rubric: 20 points for G2 and 30 for G5. Please confirm the point scales against the course's grading rules.
3. **Word counts.**
   - F4-42 is about 7,400 words of prose, above the L4 target of about 5,000. Exam P's dispatch path and the page-flip path are both there. The owner may split the "same jobs in other drivers" and memory-management subsections into a sidebar.
   - F4-46 and F4-47 (L5) are about 4,100–4,800 words including project material.
4. **Glossary.**
   - `Fence (GPU)` is defined separately from the existing `Memory fence` (SP203/HW203).
   - `EDID` is defined here (F4-43) and linked from F4-46.
   - No slug clashes with other courses' glossary files were found at build time. Courses still being written by other agents may add clashes.
5. **Exam P model answer.** The model answer is F4-42 Layer 3 "The compute dispatch, step by step". Steps 2–7 are inside an unverified box until the Source Researcher opens the HSA specifications and traces ROCm on a GPU.
6. **Hardware.** Moving any G1/G3/G4/G5/G6 part from "untested" to "tested" needs specific machines:
   - a spare machine with an AMD GPU on the ROCm list (G1 trace, G3, G4);
   - an Intel-graphics PC or laptop with an external monitor (G5);
   - a Raspberry Pi 3 (G6).

## DS201
Source: [`chapters/DS201/NOTES.md`](chapters/DS201/NOTES.md)

### How the network runs were isolated (owner decision 1)

Programs that need the network are kept in `net/` subfolders, so `run_lab.sh` does not run them directly. Each lab's `run.sh` runs them instead, inside a private network namespace.

- **F5-01 to F5-05 and F5-07** use `unshare -n`. Loopback is brought up there with a Python `SIOCSIFFLAGS` ioctl.
- **F5-06** uses `unshare -n -m --propagation private`. Inside that namespace a bind mount replaces `/etc/resolv.conf` with `nameserver 127.0.0.1`. The machine's real `/etc/resolv.conf` was checked before and after the runs and is unchanged.
- **F5-04** adds its `iptables` packet-drop rule (every 40th data packet from port 41000) only inside its own namespace.

All of this needs root. If `unshare` is not allowed, each `run.sh` writes logs that say "untested in this environment" instead of failing.

**Decision for the owner:** should learners on their own machines skip `run.sh` and run on plain loopback? The chapters already say that this works without root, except the F5-06 resolver redirect and the F5-04 packet loss.

### Decisions for the owner

1. **Root and namespaces.** The real network evidence (captures, `iptables` loss, the private `resolv.conf`) needs root and `unshare`. Is that acceptable for the reproducible-build contract? A non-root fallback is logged as "untested in this environment".
2. **Capture files are kept in the lab folders** (`.pcap` and `.pcapng`, a few KB each) so learners can open them in Wireshark. F5-02's `kernel_decode` reads `../F5-01/send_one.pcapng`, so F5-01 must run before F5-02.
3. **The pcap writer is from memory.** The classic pcap header and record layout in F5-02 and F5-03 and in `dhcp_frames.cpp` are checked only by tshark accepting the files. The Source Researcher should confirm the format document.
4. **Simulated frames.** The forensic evidence in F5-03 (ARP) and the DHCP exchange in F5-06 are frames built by our own programs, not captures of real networks. Both chapters say so and carry "untested on hardware" boxes. Approve, or schedule a real-LAN capture.
5. **Run-dependent numbers.** Loopback timings, TCP port numbers, initial sequence numbers, DNS query IDs and retransmission counts change on every run. The prose quotes the final recorded run, says "in the recorded run" where needed, and avoids numbers that the rendered output does not show. A rebuild will need these quotes refreshed: F5-04 (Figure 1 port, raw ISN, retransmission and zero-window counts), F5-05 (recv count, truncated sizes, 100 MiB time) and F5-07 (all measurements).
6. **Word counts.** Chapters run about 4,500–5,500 words of prose, plus tables. This is above a short-chapter target, because each chapter carries the full template plus captures. Trim if the owner prefers.
7. **Glossary reuse.** Exact-term reuse is described above. Please confirm that the earlier definitions of Packet, Router and MAC address (from a KID course and a hardware course) are acceptable for L2 networking readers.
8. **Zone name.** The F5-06 lab zone uses `web.lab.example` instead of a name that begins with "www.", so that no text in the course looks like a URL.

## DS301
Source: [`chapters/DS301/NOTES.md`](chapters/DS301/NOTES.md)

### Decisions for the owner

1. **Forensic "Two leaders" (F5-10):** the root cause is a leader paused for 5 s plus skewed wall clocks, which lets a second leader be elected while the first still believes it leads. The logs come from three nodes with skewed clocks. The guide's F5 forensic idea, a Raft node that does not persist its term, is different. That idea fits DS302 (consensus) better and has not been used here.
2. **Course project placement:** the "primary-backup KV store with failure injection" brief is the F5-12 mini-project. F5-14 offers a sharded extension, and F5-09 and F5-11 supply the RPC library and the failure detector it builds on. No reference solution was written.
3. **Exams:** no exam or answer-key files were written; exams Q, M, F and P are not in the per-chapter template. The "order events from logs" practical can reuse `F5-10/two_leaders.out` and `F5-13/vanishing.out`.
4. **F5-13 `run.sh`:** the forensic answer key's checker run comes from an executable `run.sh` in the lab folder, which run_lab.sh calls after the normal runs. The checker's input is cut from `vanishing.out`, not copied by hand.
5. **Curriculum mapping:** DS301 has no curriculum milestones of its own (it maps to books). F5-11 points to the curriculum's 14.1 "Fault tolerance" row (`#trackF`).
6. **Links to unwritten courses:** links to DS302 (consensus, atomic commit) and to OS304, OS401 and HW205 resolve through catalogue anchors. `build.py` reported no problems for F5-08 to F5-14.

## DS302
Source: [`chapters/DS302/NOTES.md`](chapters/DS302/NOTES.md)

### Decisions for the owner

1. **Course card versus curriculum.** SYSTEMS_CURRICULUM has no consensus milestone, so DS302 was written from the course card and guide 9.2 alone. A milestone needs to be added, or the mapping confirmed.
2. **Source registry additions** (none were added in this build). Each is needed to close an unverified box:
   - Ongaro's PhD dissertation (pre-vote, single-server changes, parallel leader writes);
   - Herlihy & Wing, "Linearizability" (DS301 also proposes it as D8 of F5-13);
   - Wing & Gong (linearizability checking);
   - Dwork, Lynch & Stockmeyer (partial synchrony);
   - Ben-Or (randomized consensus);
   - Chandra & Toueg (failure detectors);
   - optionally Lamport's "The Part-Time Parliament" and the Raft TLA+ specification.
3. **Proposed new analogy mappings** (not in the registered F5 list; please accept or replace):
   - ballot / proposal number = the number printed on a café order form; promise = "I will ignore forms with smaller numbers" (F5-17);
   - term = a numbered "round of being in charge" (F5-18);
   - membership change = joining or leaving the group chat (F5-20);
   - fault injection / nemesis = a deliberately bad mail service (F5-21);
   - linearizability = all answers fit one notebook read in one order, consistent with the clock (F5-21).
4. **One Raft library, four identical copies.** The Raft library is copied byte for byte into `labs/F5-18` … `labs/F5-21`, because each lab folder must build alone. The copied files are `raft.h`, `raft_election.h`, `raft_replication.h`, `raft_membership.h`, `cluster.h` and `sim.h`. `sim.h` is also identical in F5-15 and F5-17. Any fix must be applied to all copies; their md5 sums matched at the end of this build. The owner may prefer a shared include directory, if run_lab.sh is changed to allow one.
5. **Deliberate faults in the shipped library.** `raft::Faults` holds the four forensic bugs (`lazyPersist`, `skipPrevLogCheck`, `commitOldTermByCount`, `readFromLocalState`), all off by default.
   - This makes the forensic labs and the planted-bug table possible.
   - It also reveals the fault names to a student who reads `raft.h` before a forensic lab. The forensic questions ask which fault matches the evidence, so the names act as hypotheses, not answers. The owner may still prefer to hide them.
6. **Course project = MP2 seed.** The F5-21 mini-project is the course project, a Raft-replicated key-value service. Its rubric demands an honest planted-bug table. The exam item "P: find the bug in a provided Raft trace" can reuse any of the five forensic traces, or new ones generated by switching on a different `Faults` flag with another seed.
7. **Shared glossary terms with DS301.** "Linearizability", "Stale read" and "History (of operations)" are copied verbatim from DS301's glossary. Only the `chapters` field differs, and build.py merges the chapters. All other DS302 terms are new.
8. **Lab reads go through the log.** For linearizable reads, the lab sends gets through the log. ReadIndex and leases are described in F5-21 but not implemented; this choice is left for MP2.

## DS303
Source: [`chapters/DS303/NOTES.md`](chapters/DS303/NOTES.md)

### Decisions for the owner

1. **Learning-cluster tool for F5-25 Part B.** Candidates named only in an unverified box: kind, minikube, k3s. The dossier must pick one, with its documentation pages and version.
2. **Slurm VM lab (F5-23 Part B).** Decide how many VMs, which Slurm version and packaging, and whether QEMU or another hypervisor is used. Part B is untested.
3. **GPU cluster access for F5-26 Part B and the course project.** No GPU was available to this build. Someone must run `gpu_visible.cu` and `gpu_job.sh` on real Slurm and Kubernetes GPU nodes and record the output.
4. **New sources proposed for the dossier:**
   - Burns, Grant, Oppenheimer, Brewer, Wilkes, "Borg, Omega, and Kubernetes" (ACM Queue 2016);
   - Schwarzkopf et al., "Omega" (EuroSys 2013);
   - Feitelson and Rudolph on gang scheduling (JPDC 1992);
   - Jeon et al., "Analysis of Large-Scale Multi-Tenant GPU Clusters for DNN Training Workloads" (USENIX ATC 2019);
   - NVIDIA MIG and MPS guides.
   
   All are cited as title only.
5. **EASY backfill name (F5-23).** The chapter explains backfill without depending on the name. Confirm the attribution or drop it.
6. **Proposed analogy mappings (F5 "friends in different towns").** The registered mapping is "cluster scheduler = the organiser who assigns chores". Proposed additions:
   - F5-22: "node = one friend's house with its own tools"; "job = one chore that may need several friends at once".
   - F5-23: "job script = the chore card with the tools list written on top"; "backfill = letting a short chore slip into a gap that will be over before the big chore's friends are all free".
   - F5-24: "container = a sealed chore kit: the tools and instructions packed so the chore runs the same in any friend's house, done in a room with its own door, its own house number plate and a fixed share of the house's supplies".
   - F5-25: "control plane = the shared noticeboard plus the helpers who each watch one part of it and fix what differs from the wishes pinned there".
   - F5-26: "GPU islands = benches of ovens that pass trays directly"; "fragmentation = free ovens scattered so no kitchen has enough for the big cake".
   - F5-27: "priority bands = urgent chores may interrupt hobby chores, never the reverse"; "quota = each friend's yearly allowance of helper-hours".
7. **Glossary qualifiers.** Some terms already exist in other courses, so DS303 uses qualified terms to avoid duplicates:

   | DS303 term | Existing term (course) |
   |---|---|
   | Container (Linux) | Container (SP101) |
   | Namespace (Linux) | Namespace (DR301) |
   | Node (cluster) | Node (HW101) |
   | Checkpoint (job) | Checkpoint (OS401) |
   | Preemption (scheduling) | Preemption (OS201) |
   | Starvation (scheduling) | Starvation (OS201) |

   F5-22 does not define "Scheduler" again. It links to OS201's term. Merge these if the owner prefers.
8. **Root-only labs.** F5-24's runc and cgroup runs need root and a writable cgroup v1 memory controller. The chapter gives a rootless fallback path for learners. Decide whether learners' machines (cgroup v2 by default) need a v2 variant of `run.sh`.

## DS304
Source: [`chapters/DS304/NOTES.md`](chapters/DS304/NOTES.md)

### Decisions for the owner

1. **OpenBMC in QEMU (course card: "if the dossier confirms a QEMU machine for it").**
   - The machine exists in this build's QEMU 8.2.2: 25 BMC machines are listed, among them `ast2600-evb`, `romulus-bmc` and `witherspoon-bmc`.
   - No OpenBMC image could be built or fetched here. F5-30 therefore runs a bare-metal program on `ast2600-evb` and studies the architecture with a C++ model; the OpenBMC boot is an optional, untested lab step.
   - To complete it, provide a prebuilt OpenBMC image (for example for romulus or an AST2600 board) in the lab environment.
2. **QEMU's `ipmi-bmc-sim` ignores `device_id`.**
   - With `device_id=0x42`, Get Device ID reports 0x20, which is the property's default ("default: 32" in QEMU's help). A run during authoring with `slave_addr=0x24` still reported 0x20, while SMBIOS type 38 followed the new address.
   - The chapter keeps this as a "compare configuration with what the device reports" lesson and the check prints a NOTE, not a FAIL. The cause (QEMU's `hw/ipmi/ipmi_bmc_sim.c`) was not opened. Confirm, or file it as a QEMU behaviour.
3. **Get Self Test Results returns 0xC1 (invalid command)** on QEMU's simulated BMC. It is used in F5-29 as the completion-code lesson.
4. **KVM is unavailable.**
   - Milestone V1 is covered only for QEMU/TCG, plus the container itself, which reports "KVMKVMKVM" from an unidentified KVM-based monitor.
   - The F5-32 forensic lab uses the real TCG fallback as its injected fault.
   - A lab machine with `/dev/kvm` is needed for the QEMU/KVM case; Hyper-V and bare metal need other machines.
5. **F5-32 timings vary between runs**, because the container is shared. Integer TCG/host ratio: 1.2× in one run, 1.8× in the final run. Floating point: 5.7× and 6.4×.
   - The chapter quotes the final run and the earlier one, and tells students to use ratios.
   - If `build.py` re-runs the labs, the quoted numbers in F5-32 (Layer 3, the lab's expected observations, the summary and the forensic key) may no longer match the inserted output.
   - The same applies to F5-31's forensic time stamps (7.4 s, 71.5 s, 72.0 s) and F5-29's SEL time stamp, which the text describes without quoting.
6. **`consolidate` exits with 3 by design** (one VM unplaced). `run_lab.sh` accepts it because the log records the exit code. Change the program to return 0 if the owner prefers that every listing exit 0.
7. **SMBIOS UUID byte order (F5-31).** The raw bytes show the first three fields byte-reversed relative to the `-smbios uuid=` option. The inventory keeps raw bytes and the check reports the difference; converting it is the mini-project. Decide whether the course project requires the standard text form (recommended).
8. **QEMU's SMBIOS type 17 does not follow the NUMA layout** (F5-28: one "DIMM 0" of 512 MiB for two nodes). This is noted in F5-28 as a reason to trust SRAT over SMBIOS for locality.
9. **Forensic packs that are simulations**, labelled as such in each chapter:
   - F5-29: `power_audit.cpp`
   - F5-30: `forensic_fans.cpp`
   - F5-33: `night.cpp`, the course forensic "The server that reboots at night"
   - The F5-31 and F5-32 forensic packs are real QEMU runs with injected faults.
10. **The P exam ("plan the boot and management path for a rack")** is not written as a separate file. F5-31 Layer 3 ("From two servers to a rack") and its mini-project (the course project, with a one-page rack plan) prepare for it. The owner may want an exam paper in the course folder.
11. **Banned-word checks:**
    - F5-31 uses "Trivial File Transfer Protocol", the protocol's name. It is kept.
12. **Sources needing the Researcher:**
    - DMTF Redfish (the registry records "403 Forbidden").
    - The IPMI v2.0 specification, OpenBMC documentation, the PXE specification and the RFCs.
    - ASHRAE TC 9.9.
    - A reliability textbook (D6 of F5-33 is a placeholder: "chosen by the Source Researcher").
    - A server vendor's hardware manual (D2 of F5-33).
    - F5-29 D5 is the curriculum used as a map (AH-4) and must be replaced.
13. **Chapter length:** about 5,200 to 6,500 words each, including tables, against a target of about 4,000 for L3. The extra is mostly in the lab, forensic and answer sections.

## DS401
Source: [`chapters/DS401/NOTES.md`](chapters/DS401/NOTES.md)

### Decisions for the owner

1. **RDMA hardware or Soft-RoCE for the labs.**
   - The course card says "Soft-RoCE or real RDMA NICs (stated per lab)". The build container has neither: no kernel modules, no `/sys/class/infiniband`.
   - Every RDMA program is therefore compiled, linked and run to its honest "no device" exit (F5-36), or replaced by a model.
   - Before release, someone needs to run F5-36 Listing 1 and the F5-37 perftest plan on two hosts, then record the results with machine, adapter, firmware and versions.
2. **perftest is not installed.** Install the package in the build image, or accept the unverified flag list in F5-37.
3. **The course project has no reference solution.** It was not built or run, and no `_keys/` notes were written, because `_keys/` is outside my folders. The model track (toy verbs plus shared-memory rings) makes it gradable without hardware. Please confirm that this is acceptable.
4. **The ICRC is not computed.** F5-35 writes a zero ICRC, and tshark does not judge it. Computing it needs the annex's masking rules, which are unverified.
5. **Loopback without namespaces.** DS201 isolated its network programs with `unshare -n`, but DS401's TCP programs use plain loopback on 127.0.0.1. They make no external connections and need no root. Please say whether the DS201 convention should apply here too.
6. **Run-to-run timing variation.** Small-message latency medians vary a lot on this container. Two examples:
   - F5-37: α was 12.88 µs in one run and 19.95 µs in another.
   - F5-38: the socket median was 4.50 µs in the sanitizer run and 24.27 µs in the `-O2` run.

   The chapters teach this explicitly and quote minima and system-call counts where those are stable. A rebuild will print different numbers, so the prose must be refreshed together with the outputs.

## DS402
Source: [`chapters/DS402/NOTES.md`](chapters/DS402/NOTES.md)

### Decisions for the owner

1. **"Instrument the DS302 service" lab.** The learner's own DS302 service does not exist in the repository, so F5-41 lab steps 2–6 and F5-43 lab steps 4–6 are marked untested. The DS302 simulator labs (F5-18/F5-19 Raft) do exist. A future build could instrument that simulator directly, which would make the course lab fully tested. Please decide whether to do that.
2. **Chapter-local D-numbers.** D7 and D8 mean different works in different chapters (see Sources). If the owner wants course-wide numbering, renumber them when the registry is updated.
3. **Seven proposed sources.** They should be confirmed, added to the F5 registry, or replaced. See the proposed additions above.
4. **Lines longer than 100 characters.** These are style only; every listing compiles with `-Werror`. They are in `F5-41/service.cpp` (line 43 among others), `F5-39/gfs.cpp`, `stale.cpp`, `wordcount.cpp`, `F5-40/quorum.cpp` and some F5-42/F5-43 files. Decide whether to enforce a column limit.
5. **No curriculum milestone.** No curriculum milestone or C-number maps to DS402 in the course card. "Maps to" names only the papers and the SRE book.
6. **Course project "Observability for MP2".** It is split across the chapters: part 1 in F5-41, part 2 in F5-42 and part 3 in F5-43. The practical exam ("write an SLO and alert") is practised in F5-42's mini-project.
7. **F5-43's "Next chapter" link.** It points to F5-44 (DS403). That link resolves now because DS403's F5-44 exists in the build.

## DS403
Source: [`chapters/DS403/NOTES.md`](chapters/DS403/NOTES.md)

### Decisions for the owner

1. Proposed F5 analogy mappings listed above (F5-44, F5-45, F5-46): accept into the
   registry or ask for replacements.
2. Proposed source D10 for F5-46 (AFS / NFS papers) and the Borg paper (F5-47 D3), FLP and
   "Paxos Made Simple" if not already registered for DS403.
3. Nondeterministic run values in prose: keep them (updated on every re-run, list above),
   or switch the chapters to phrasing that cites the `.out` without exact ticks. A future
   option is QEMU `-icount` for the guest clocks, but the switch's host clock would still
   vary.
4. The lab kernel is shared across chapters via `labs/F5-44/`; MP2 (the course project)
   expects learners to port it to their own MP1 kernel.
5. Practical exam P (design review) is described in F5-47 per guide 11.4; reviewer scenario
   list and time limit are left to the owner.
6. The glossary entry "TFTP" also exists in DS304; the builder merges them by term (DS304's
   text wins as the first course alphabetically). Other glossary links of these chapters
   point to terms defined in other courses' glossary.json files (checked to exist at build time).

## CU201
Source: [`chapters/CU201/NOTES.md`](chapters/CU201/NOTES.md)

### Decisions for the owner

1. **Lab GPU.** Choose the lab GPU (or cloud instance) and record toolkit and driver versions in the dossier; every "untested on hardware" box should be closed by one real run of each lab, replacing nothing in the prose but adding the real `.out`.
2. **Toolkit version gap.** The container has CUDA 12.0; the guide's source registry names the Programming Guide 13.x. The SASS/PTX shown is from 12.0 for sm_80; re-generate with the lab's toolkit and check that the line-number references in the code tables still hold.
3. **Compute Sanitizer.** It cannot run in the container; F6-05 lab step 3 must be validated on hardware, including the report format quoted from memory.
4. **HIP slot of the harness** (F6-06 `bench.h`, `timing_hip.hip`) is compiled only for gfx90a; confirm the target with the F7 courses.
5. **Bit-exact SAXPY (E1).** The acceptance test assumes FMA contraction on both sides (CPU reference uses `std::fma`). If the owner prefers a tolerance-based test, F6-04 Layer 3 and its forensic lab need adjusting.
6. **Analogy mappings (proposals for the registry, guide 8).** The chapters use the registered F6 mappings (helper = thread, row = warp, table = block, all tables = grid, hands = registers, table in the middle = shared memory, big pantry far away = global memory, fetching neighbouring jars in one trip = coalescing). These mappings are new and not yet in the registry; please accept, change or reject them:
   - constant memory = the menu board on the wall (F6-03);
   - local memory = a helper's private shelf in the far pantry (F6-03);
   - CUDA event = a slip in the order queue saying "stamp the clock when you get here"; warm-up = the first order of the morning while the ovens heat (F6-06);
   - compute capability = the generation of the hall's equipment; fatbinary = a folder of recipe cards printed for different kinds of hall (F6-07);
   - memory roof = jars per minute the pantry corridor carries when full; vector load = a crate of four jars (F6-09);
   - pinned memory = goods kept on the loading dock; pageable memory = goods in the ordinary warehouse, carried to the dock first (F6-09).
7. **Exemplar alignment.** F6-01 follows the guide-14 exemplar's shape; its PTX and runs are now real (the exemplar's placeholders are filled from this build's logs).

## CU301
Source: [`chapters/CU301/NOTES.md`](chapters/CU301/NOTES.md)

### Global facts for the owner

- **No GPU number anywhere.** No time, bandwidth, speed-up or "% of CUB / % of copy" was
  measured; every such number in E3/E4/E5 is a learner measurement ("your X").
- **No document was opened.** CUDA C++ Programming Guide (D1), CUDA C++ Best Practices
  Guide (D2), Hwu/Kirk/El Hajj PMPP (D3), Nsight Compute and Compute Sanitizer docs,
  HIP docs, Harris "Optimizing Parallel Reduction in CUDA", Merrill and Garland
  "Single-pass Parallel Prefix Scan with Decoupled Look-back", Cormen et al.
  "Introduction to Algorithms" (F6-18 D4) are cited **title only** (dossier gate G1 open).
- **Tier-1 evidence that was really read:** installed CUDA 12.0 headers and CUB 2.0.1
  headers (quoted through `run.sh` steps so the quotes are reproducible).
- **Forensic labs without real evidence tools.** The course card's forensic "Bank conflicts
  in the transpose" expects a provided Nsight Compute output. None could be produced (no
  GPU) and inventing one is forbidden (AH-25); F6-11 uses kernel source + ptxas report +
  SASS address arithmetic + a CPU bank replay instead (unverified box in F6-11).
  **Decision for the owner:** replace with a real Nsight Compute report once a GPU run exists.
  "The reduction that is wrong only for large inputs" is in F6-14 (int index overflow,
  shown with PTX and a CPU arithmetic replay).
- **Analogy registry:** only registered F6 mappings are used (helpers, rows, table in the
  middle of the row, drawers = banks, tally counter = atomics, pantry). Small story
  extensions that are not new technical mappings: "tally sheet on each table" (privatization,
  F6-17), "board by the door" (tile status words, F6-16), "plates to the shelf by digit"
  (radix sort, F6-18), "shifting jars on the table / Rahim's rule" (padding / swizzle,
  F6-19). **Proposal for the Dean:** register "a private tally sheet per table" =
  privatization and "board by the door" = tile status / look-back.

## CU302
Source: [`chapters/CU302/NOTES.md`](chapters/CU302/NOTES.md)

### Decisions for the owner

1. **No GPU numbers.** None of the E6 or E7 speed tables were measured, so no time, GFLOP/s or "% of cuBLAS" appears anywhere. When a GPU is available, run the `sgemm_*` and `wmma_gemm` programs (build with `-arch=sm_XX -DUSE_CUBLAS -lcublas`) and add a real speed table to F6-21 to F6-26.
2. **The course card's "Low occupancy, high speed?" forensic (F6-20)** expects a profile where lower occupancy is faster. No profile can be produced without a GPU. F6-20 uses the register report, the ILP kernels' SASS and a *simulated* scheduler profile from its latency model (labelled as a stand-in for a profiler export). The scenario's "nearly twice as fast" is story text, not a measurement. Replace this with a real Nsight Compute comparison once one exists.
3. **The "Register spills" forensic (F6-23)** uses real compiler evidence: `__launch_bounds__(256,4)` forces 64 registers, giving 1,336 B of spill stores, 1,204 B of spill loads, and 301 LDL and 334 STL in the SASS, against 0 without it.
4. **Example device.** F6-20 defines an "example device" (cc 8.0-like limits) that F6-22, F6-23 and F6-27 reuse. It is a model and is labelled as such. Confirm that it is acceptable, or replace it with a real device's `deviceQuery` output.
5. **Analogy registry.** The chapters use only registered F6 mappings (hall, rows of helpers, pantry, table, tray machine for tensor cores). Proposed for registration:
   - "two trays": double buffering (F6-24).
   - "measuring cups with fewer or more marks": number formats (F6-25).
   - "the seating chart": layout (F6-27).
6. **The E7 lower-level version** (fragment layouts used directly, optional in the curriculum) is not built. It appears only as an optional lab step in F6-27.
7. **cuBLAS references** in the harnesses (`cublasSgemm` and `cublasGemmEx`) compile and link against CUDA 12.0, but their semantics are unverified. The chapters say so.
8. **F6-22's forensic evidence** is a deliberate deadlock. Its log shows `exit code: 124 (stopped by the 20 s time limit)`, which build.py accepts as a completed run.

## CU303
Source: [`chapters/CU303/NOTES.md`](chapters/CU303/NOTES.md)

### Decisions for the owner

1. **Model-generated profiler evidence.** F6-28, F6-29, F6-32 and F6-33 forensic labs (stream_model, pipeline_model, timeline_stats, roofline_model with `forensic.in` inputs) and the F6-32 timeline figures use evidence produced by the course's own CPU models, labelled as such, instead of real Nsight Systems / Nsight Compute exports. Replace with real exports from a GPU run when hardware is available.
2. **Real GPU runs.** All 12 CUDA programs are untested on hardware; each chapter states the expected output. A GPU pass (record GPU, driver, toolkit) should replace the untested boxes.
3. **TG-1 compute roof (F6-33).** The chapter proposes a compute roof for the teaching GPU: 4 SMs × 32 lanes × 1 FMA × 2 FLOP × 1 GHz = 256 GFLOP/s, with the existing 256 GB/s giving a ridge point of 1 FLOP/byte. This extends HW301's TG-1 definition and needs owner approval (or the HW301 author's).
4. **Analogy mappings (F6, the great kitchen hall).** Proposed new mappings: order spikes and tokens for streams/events (F6-28); loading dock and storeroom for copies and pinned memory (F6-29); a laminated card for a CUDA graph (F6-30); runners fetching crates on demand for managed memory (F6-31); the wall chart for a timeline profiler (F6-32); the inspector for a kernel profiler (F6-33); the steward's hall-specific card for SASS (F6-34). Add to the analogy registry or replace.
5. **nvprof.** F6-32 shows nvprof's help and a run that skipped GPU profiling; whether to teach nvprof at all for current GPUs depends on NVIDIA's support status, not checked here.
6. **Glossary merges.** `glossary.json` includes copies (with CU303 chapter lists) of HW301's "Copy engine / DMA", "TG-1 (teaching GPU)", "PTX / SASS", "Spill / local memory" and SP302's "Memory-bound and compute-bound" so that the chapters' glossary links list CU303 chapters. Because entries merge by term with the first course alphabetically winning, CU303's own definitions of "Pinned (page-locked) memory", "Arithmetic intensity", "Roofline model" and "Ridge point" take precedence over HW301/SP302's; keep or reconcile.
7. **Sources.** All vendor documents are titles only (dossier gate G1 open); the Source Researcher must confirm them, in particular the CUDA Binary Utilities instruction set reference for F6-34's inferred meanings.

## CU401
Source: [`chapters/CU401/NOTES.md`](chapters/CU401/NOTES.md)

### Global facts for the owner

- **No GPU number anywhere.** No time, bandwidth, speed-up, occupancy or "% of library" was
  measured. All such values are learner measurements ("your X"). The only GPU-like numbers
  are those of the university's invented TG-1 / TN-4 models, always labelled invented.
- **No document was opened.** CUDA C++ Programming Guide, PTX ISA, CUDA Binary Utilities,
  cuBLAS / CUB / Thrust / cuDNN / CUTLASS documentation, Triton documentation, PCIe
  specification, NVIDIA/AMD interconnect documents, PMPP, Stevens and Rago (APUE), and the
  papers (BLAS: Lawson et al., Dongarra et al.; Merrill and Garland; Milakov and Gimelshein;
  Welford; Chan, Golub and LeVeque; Ba, Kiros and Hinton; Vaswani et al.; Dao et al.
  FlashAttention and FlashAttention-2; Tillet, Kung and Cox) are cited **title only**
  (dossier gate G1 open).
- **Tier-1 evidence really read:** installed CUDA 12.0 headers (`cuda_runtime_api.h`,
  `driver_types.h`, `cooperative_groups.h` and `details/info.h`, `details/sync.h`,
  `crt/device_functions.h`, `sm_30_intrinsics.h`), cuBLAS 12.0.2 headers (`cublas_v2.h`,
  `cublas_api.h`), CUB 2.0.1 headers (`device_reduce.cuh`, `device_scan.cuh`,
  `block_reduce.cuh`), Thrust 2.0.1 `version.h`. Quotes in F6-35, F6-36 and F6-37 are
  reproduced by lab steps (`header_quotes`, `p2p_api`); F6-38 and F6-39 quote
  `__expf` / `__shfl_xor_sync` from the headers directly (not yet through a lab step).
- **Invented teaching hardware:** TG-1 (the course's teaching GPU, glossary entry from
  HW301) is reused in F6-36 (4 SMs, residency model) and F6-37 (host link 10 µs,
  16 GB/s, as in F6-29). **TN-4** (four TG-1 behind two invented PCIe switches) is new
  in F6-37; its link numbers are invented for arithmetic. Decision for the owner:
  register TN-4 next to TG-1 in the glossary/analogy registry, or rename.

### Decisions left to the owner

1. Approve publishing with the unverified boxes listed above (AH-19).
2. Register the two analogy proposals (library station; table-level recipe card) and the
   invented TN-4 node.
3. Source Researcher: open the CUDA Programming Guide sections on cooperative groups,
   peer-to-peer access and IPC, the cuBLAS/CUB/Thrust/Triton documentation and the
   FlashAttention papers, and close gate G1 for these six chapters.
4. Lab Engineer with GPU access: run every `.cu` listing on a real NVIDIA GPU (and
   F6-37 on a multi-GPU node), then replace the "untested on hardware" boxes with real
   records.

## HP301
Source: [`chapters/HP301/NOTES.md`](chapters/HP301/NOTES.md)

### Decisions for the owner

1. **HIPIFY tools.** The real hipify-perl / hipify-clang were unavailable; F7-04 teaches with
   `toyhipify.py` and puts every HIPIFY detail in unverified boxes. When a machine with ROCm's HIPIFY is
   available, add a lab step that runs both tools on `saxpy.cu` and `warp_sum.cu` and compare with the toy.
2. **Hardware for the course.** Every GPU result is untested. The course needs at least one wave64 AMD GPU
   (CDNA, from the ROCm matrix) and one NVIDIA GPU, or cloud time, to run the E9 acceptance tests and to
   confirm the lane-model predictions of F7-07 (ported total 528 on gfx90a).
3. **ROCm packaging.** The container's distribution packages lack `hip-lang-config.cmake`, so CMake's HIP
   language could not be taught by running it. Decide whether the course standardises on AMD's own ROCm
   packages (then re-run F7-06 `cmake_hiplang`) or keeps `find_package(hip)` as the main path.
4. **HIP version.** All header line numbers and behaviours are HIP 5.7.1 (Ubuntu). A dossier pass should
   pick the ROCm version the university will use and re-run every lab (hipmap counts, warp functions,
   `[[nodiscard]]`, `warpSize` declaration can change).
5. **Glossary overlap.** HP301 defines "Wavefront" and "warpSize (HIP)"; HW301 has "Warp / wavefront" and
   HP302 links `#gl-warpsize-hip` and other terms. If HP302 later defines the same term strings, the builder
   merges them; check wording consistency then.
6. **Analogy registry.** Story characters (Amara, Ravi, Noor, Viktor, Tomás, Kwame, Sipho, Ines) and the
   mappings "second kitchen building = ROCm/AMD", "shared cookbook language = HIP", "prep chef = hipcc",
   "plan of work = CMake", "maintenance list = compatibility matrix", "translator = HIPIFY" extend the F7
   great-kitchen-hall world (BR-02 names "a second kitchen building"). Proposed for the registry (guide 8.1).

## HP302
Source: [`chapters/HP302/NOTES.md`](chapters/HP302/NOTES.md)

### Decisions for the owner

1. **No profiler in the build container.** The course's forensic ("ROCm kernel with low occupancy") and E10's "confirm in the profiler" use the compiler's resource report (`-Rpass-analysis=kernel-resource-usage`) as the evidence, labelled as such. Profiler confirmation is a GPU lab step (F7-14 step 6). Please decide:
   - whether to install rocprofv3 / ROCm Compute Profiler in the image;
   - or whether to provide recorded profiler output from a lab GPU.
2. **HIP version.** HIP 5.7 (Ubuntu package) is old. Several facts are version-bound and boxed:
   - the wave64-on-RDNA `#error`;
   - `__shfl` without `_sync`;
   - `__launch_bounds__` second argument = waves per EU.

   A newer ROCm in the image would change F7-09 and F7-12 evidence.
3. **Profiler tool names.** F7-14 uses the current names (ROCm Compute Profiler / ROCm Systems Profiler, formerly Omniperf / Omnitrace) and the commands `rocprof-compute` and `rocprof-sys-run`. These come from memory and must be checked against D7.
4. **Inferred occupancy model (F7-10).** It is presented as "inferred from the compiler, matches all cases". The alternative is to wait for the ISA guide or LLVM guide to be opened.
5. **Analogy mapping proposal for the F7 world (second building)**, for the analogy register:
   - wavefront = row of helpers (64 or 32);
   - SIMD = supervisor's counter;
   - VGPR = one hand of every helper;
   - SGPR = the row's shared notepad;
   - AGPR = tray for the matrix machine;
   - LDS = the table; LDS bank = drawer;
   - padding = empty slot at the end of a row of drawers;
   - swizzle = each row's own drawer order;
   - kernel descriptor = recipe header card;
   - kernarg = order slip;
   - the three profilers = door clipboard (rocprofv3), table counter (Compute Profiler), street film (Systems Profiler).
6. **Glossary overlaps.** `glossary.json` repeats four terms that HP301 or HP401 also define, with identical names so that the build merges their chapter lists:
   - LDS (Local Data Share);
   - AGPR (accumulation register);
   - warpSize (HIP);
   - Offload bundle (fat binary).

   The build keeps the first definition it reads.
7. **Course project.** The AMD hardware passport and tuning notes are assembled in F7-14's mini-project. Its fields are seeded across F7-08 to F7-13.

## HP401
Source: [`chapters/HP401/NOTES.md`](chapters/HP401/NOTES.md)

### Global facts for the owner

- **No GPU number anywhere.** Every kernel run stopped at the runtime's "no device" error.
  The only timings are CPU-side teaching runs (F7-19), labelled "this machine, this run".
- **No document was opened.** AMD ISA guides (CDNA/CDNA2/CDNA3, RDNA3), HIP, rocPRIM,
  hipCUB, rocWMMA, rocBLAS, hipBLASLt and CK documentation, Merrill and Garland, the
  roofline paper and LLVM AMDGPU docs are cited **title only** (dossier gate G1 open).
- **Tier-1 evidence really read/produced:** HIP 5.7 headers (warpSize, `__ballot`
  return type, hipDeviceProp_t), CUB 2.0.1 and cuBLAS/cuBLASLt headers, compiler ISA and
  metadata for gfx90a/gfx908/gfx94x/gfx1100, SASS for sm_80.
- **AMD library APIs are in unverified boxes** (from author memory, AH-3): rocPRIM/hipCUB
  (F7-15), rocWMMA (F7-17), CK (F7-18), rocBLAS/hipBLASLt (F7-19). Their NVIDIA
  counterparts (CUB, mma.h, cuBLAS) were compiled as the verified stand-ins.

### Decisions for the owner

1. Confirm the MFMA fragment layout (F7-17) against the CDNA ISA guide before release.
2. All AMD library APIs (rocPRIM, hipCUB, rocWMMA, CK, rocBLAS, hipBLASLt) need a check
   on a ROCm install; the unverified boxes list exactly what to confirm.
3. The E9 table in F7-20 is a template with "not measured" entries; real rows need one AMD
   and one NVIDIA GPU.
4. Course forensic "Matrix cores unused" is implemented with real compiler evidence
   (wrong guard, gfx942); the profiler screenshot mentioned in the F7-20 scenario is
   described, not reproduced.

## DG401
Source: [`chapters/DG401/NOTES.md`](chapters/DG401/NOTES.md)

### Decisions for the owner

1. **Course forensic "All-reduce at half speed" (F8-04).**
   - The card asks for a real NCCL debug output, provided. None exists in this build, and writing one from memory is forbidden (AH-17).
   - The evidence pack therefore uses the university's own ring planner output (`topo_sim`) and a model busbw curve (`busbw_curve`), both clearly labelled.
   - **Decision:** record a real `NCCL_DEBUG=INFO` (or RCCL) log and an nccl-tests curve on a two-socket machine, with the job placed across sockets, and add them as the "hardware" variant.
2. **TN-1 is invented.**
   - Keep it as the course's teaching node, or replace it with a measured lab machine once milestone F1 has been run there.
3. **No GPU run of any CUDA/HIP listing.**
   - Four listings are untested on hardware: `p2p_matrix.cu`, `p2p_probe_amd.hip`, `ring_gpu.cu`, `ring_gpu_amd.hip`.
   - They need a multi-GPU rerun of `run_lab.sh` before the course is released.
   - `ring_gpu.cu` should then also call `cudaDeviceEnablePeerAccess`; the chapter tells students to add it.
4. **Milestone F2 acceptance was done only on CPU threads.**
   - Sizes 4 B to 1 GiB were covered, but 1 GiB only for N = 2 and 4.
   - The "X % of NCCL/RCCL" target and the curve against `all_reduce_perf` need GPUs. X is left to the student's plan, as the curriculum says.
5. **Milestone F3 scaling efficiency** is not reported in this build. The chapter explains why: a tiny model on shared CPU cores.
6. **Exam P** ("predict all-reduce time from the model, then measure") is prepared by F8-03. Its prediction and measurement there use MPI on CPU processes; the GPU version needs hardware.
7. **Lab file rename.**
   - F8-05's GPU listings are named `ring_gpu.cu` and `ring_gpu_amd.hip`, not `ring_allreduce.cu`.
   - Reason: `run_lab.sh` names outputs by file stem, so `ring_allreduce.cu` overwrote the `.out`/`.log` of `ring_allreduce.cpp`.
   - The runner could warn about duplicate stems; that is outside this course's folders, so it was not changed.

## DG402
Source: [`chapters/DG402/NOTES.md`](chapters/DG402/NOTES.md)

### Other decisions for the owner

1. **Untested GPU and RDMA steps:**
   - every lab step on GPUs, RDMA networks, two nodes, OSU, perftest, nccl-tests or rccl-tests, or a profiler is written as an instruction with "not run in this build";
   - its first run on real hardware should replace these notes;
   - the five CUDA programs (two CUDA probes and three CUDA-and-MPI programs) have never executed past device discovery.
2. **The "X %" target** in F5's third acceptance test is left to the learner to set before measuring, as the curriculum does.
3. **Simulated nodes in F8-10** prove correctness, not speed. The chapter says so.
4. **Open MPI was not rebuilt with CUDA support** for a GPU-aware comparison. Doing so would need network access and a GPU to be meaningful.

## DG403
Source: [`chapters/DG403/NOTES.md`](chapters/DG403/NOTES.md)

### Decisions for the owner

1. **F8-17 level.** The course card spans L4–L5. F8-17 is marked L5 and is longer than the 3,000-word guideline (about 5,000 words including listing tables), because it carries the scheduled half of the course project. It could be shortened by moving the Monte Carlo listing to a lab-only appendix.
2. **Overlap with DS303 F5-26.** F8-17 links to F5-26 for GPU resources, GRES and device visibility, and does not repeat them. It focuses on what a distributed training job adds: gang allocation, warning signals, requeue, node-local rank and the checkpoint interval. Please confirm that the split between the two courses is acceptable.
3. **Real-scheduler testing.** `train_job.sh` ran only under bash (dry run). Its `#SBATCH` lines are unverified. A run on a real Slurm cluster is needed before the listing is promoted out of its unverified box.
4. **Outside-registry sources** (table above) need the Source Researcher to add them or to propose registry alternatives.
5. **Glossary merges.** Three exact-term merges are listed above. If the owner prefers DG403-specific wording (for example, Reduce-scatter's analogy in the kitchen world), the DG401 entry needs to be edited, because `build.py` keeps the first entry.
6. **Exam P materials.** The design-review rubric exists only inside the mini-projects. A separate exam brief (a model, a cluster, and the questions the review must answer) could be written for the course page.

## RB201
Source: [`chapters/RB201/NOTES.md`](chapters/RB201/NOTES.md)

### Decisions left to the owner

1. **Course kit.** No robot kit, motor, encoder, IMU, LiDAR, camera, battery pack, regulator or CAN interface has been chosen. Every lab's Part B and the course project's real datasheet citations depend on that choice. Per the brief, no commercial kit is named.
2. **Hughes & Drury** in the source registry (see above).
3. **Analogy mappings** for power, buses, datasheets and cameras (see above).
4. **Design rules.** The 70 % bus-load rule (F9-14) and the 80 % rail-load rule (F9-15) are this course's own team rules, stated as such in the text. Confirm them or set other values.
5. **Recorded data format.** F9-11 uses the university's text "scan log v1" instead of a ROS bag, because ROS 2 is not available and is taught in RB303. Decide whether RB201 should switch to bags once RB303's tooling exists.
6. **Forensic scenario for the course card.** "The robot drifts left" is F9-09's forensic lab: wrong wheel diameter parameter versus slip, told apart by whether the encoder count ratio is constant. The other chapters' scenarios were chosen not to repeat HW302's (rolling shutter, brown-out):
   - stepper stall;
   - calibration while moving;
   - mounting offset;
   - FIFO latency;
   - sag cut-off;
   - CAN starvation;
   - logic-level and absolute-maximum violations.
7. **Length.** Most chapters are a little over 5,000 prose words by the build's own count (F9-08 about 5,400), slightly above the L2 range of 3,000–5,000. Each chapter has three listings with line tables plus a full forensic key. Trim if the owner wants the strict range.
8. **`can_probe.cpp`** exits 0 even when there is no CAN support, so that the lab run records the kernel's real answer. On a robot computer it should be extended to bind and read, as described in the lab.

## RB202
Source: [`chapters/RB202/NOTES.md`](chapters/RB202/NOTES.md)

### Decisions for the owner

1. **Kit choice for F9-22.** The kit needs:
   - a DC motor with an incremental encoder;
   - a PWM motor driver;
   - a microcontroller board;
   - a low-voltage supply;
   - an emergency stop in the power path.

   Once the kit is chosen, someone must write `KitMotorIo` from its documents and record those documents in the RB202 dossier. Until then F9-22 ships with "untested on hardware" boxes, which need your approval (AH-26).
2. **Exercise values.** I chose these and they need your confirmation:
   - the motor's ±12 V driver limit;
   - the cart's ±10 N force limit and 1.5 N slope;
   - the F9-20 specification: S1 5 %, S2 3.0 s, S3 2 mm, S4 10 N;
   - the simulated kit's 0.5 V deadband, 1024 counts and 16-bit counter;
   - the 400 rad/s speed limit, 600 rad/s² ramp and stall watchdog (duty above 0.5, speed below 5 rad/s, 0.3 s).
3. **F9-20 robustness finding.** The chapter's own specification has no margin for a ±25 % mass error: a scratch sweep found no gain set that passes. The chapter presents this as a lesson. If you would rather have a passing robust set, loosen S2 or S3.
4. **Cross-folder include.** F9-22 includes `../F9-21/pid.hpp` instead of copying it. This keeps one library, the course project, but couples the two lab folders.
5. **Duplicated glossary terms** (listed above) use the existing wording. If you want RB202-specific wording, the merge rule keeps the alphabetically first course's text.
6. **Practical exam P.** F9-20's mini-project is written as practice for exam P. The exam itself needs a modified test bench and specification set by the instructor.
7. **Gaps in other courses.** No RB201 chapter (F9-15) existed when this course was written; the F9-16 link to `#F9-15` resolves in the current build. F9-22 links `#BR-08`.

## RB301
Source: [`chapters/RB301/NOTES.md`](chapters/RB301/NOTES.md)

### Decisions for the owner

1. **ROS 2 / tf2 / MoveIt 2 / URDF / SDF verification.** The course card asks for tf2 and URDF. This build could only teach the concepts with our own code and put every name in unverified boxes. Before RB401 relies on RB301, someone with the specifications must:
   - check the F9-23 tf2 list and the F9-29 URDF list;
   - load `course_robot.urdf` with a standard parser, and add `effort`/`velocity` attributes if the parser requires them.
2. **The course robot's dimensions** are simulator values chosen by this author: wheels r = 0.05 m and W = 0.30 m; arm links 0.30/0.30 m; planar arm 0.30/0.25 m; masses. They should be replaced by the kit's measured values once the kit is fixed in the dossier. The labs are written so that this is a one-file change per lab.
3. **The course card says "simulate; verify kinematics against simulator ground truth".** Because no commercial simulator may be named or was available, the "simulator" is our own C++ code. In F9-29, the model is verified against a closed form and against a simulated tracker. If the owner wants a physics simulator in this lab, it belongs to RB401 or needs a dossier decision.
4. **RB201 chapters F9-08 to F9-15 were not present** when these chapters were written; only their labs were. Cross-references to RB201 (motors, torque, encoders) and RB202 (control) are by chapter id only and may need wording changes when those chapters land.
5. **Maths prerequisites.** F9-27 and F9-28 use derivatives, differential equations and integrators. MA301 and MA302 are not formal prerequisites of RB301, so each of those chapters has a "maths you need here" box. Consider adding MA301 as a recommended prerequisite on the course card.
6. **Word counts** run above the L3 target of about 4,000 words: about 5,500 to 6,600 words of non-code text per chapter, including tables and answer keys. The owner may want the Layer 3 sections trimmed.
7. **The course project** (a kinematics library reused by RB401) is specified across the chapters' mini-projects:
   - `kin/frames`, `kin/diffdrive`, `kin/fk`, `kin/ik`;
   - `kin/jacobian`, `kin/dynamics`, `kin/model`.

   No reference solution is shipped; the lab headers (`frames.hpp`, `diffdrive.hpp`, `arm.hpp`, `ik.hpp`, `jac.hpp`, `dyn.hpp`, `urdf_mini.hpp`) are a starting point.
8. **Exams (Q; M; F; P: compute and verify an arm's forward kinematics).** No separate exam files were written; the F9-25 lab and the F9-29 sweep test are natural P items.

## RB302
Source: [`chapters/RB302/NOTES.md`](chapters/RB302/NOTES.md)

### Decisions for the owner

1. **Course kit sensors:** a range sensor for the F9-32 cart, a landmark sensor (camera
   markers or a scanner) for F9-34, and the IMU and encoders for F9-35. Until they are chosen,
   every number is a simulator setting, and each chapter says so in an untested-on-hardware
   box.
2. **NIS/NEES source:** the registry has no target-tracking textbook. F9-32 to F9-35 rely
   on NIS, NEES, chi-square gates and innovation whiteness. Please add one to the registry,
   or accept these as W1 derivations.
3. **Cross-course data file:** F9-35 reads MA202's `imu_rest.csv` in place, through
   `../F0-62/`. The alternative is a copy in F9-35's folder, which is more robust but
   duplicates data. The current choice matches the course card's wording.
4. **Length:** the chapters run 10–35 % above the L3 guide length. The Editor may shorten
   Layer 3 sections. No numbers depend on them.
5. **MP6 interface:** the project's class interface (`onOdometry`, `onGyro`, `onLandmark`,
   `pose`) is a proposal. MP6's authors should confirm it, or replace it, before RB303 and
   MP6 build on it.

## RB303
Source: [`chapters/RB303/NOTES.md`](chapters/RB303/NOTES.md)

### Decisions for the owner

1. **ROS 2 distribution.** The chapters are written to be distribution-neutral, and every name is in unverified boxes. Pick one LTS distribution (for example Jazzy) for the labs' Part B and the course project. The Source Researcher can then check the boxes against that distribution's documentation.
2. **Gazebo release.** The Part B lab and the project need a Gazebo release paired with the chosen distribution. Gazebo Classic should not be used. Confirm whether the faculty prefers Gazebo or allows another simulator (Webots, Isaac Sim) for students without GPUs.
3. **DDS vendor.** F9-40 and F9-41 do not name a default RMW or DDS vendor as fact. Decide whether the course standardises on one (Fast DDS or Cyclone DDS), because QoS defaults and discovery behaviour differ.
4. **No curriculum milestone id.** The course card lists none, so the meta `maps` lines say "no curriculum milestone id". Assign one if the track structure expects it.
5. **Word counts.** Prose ranges from about 4,500 to 5,700 words per chapter, against the L3 target of about 4,000. The extra comes from the unverified boxes and the Part B outlines. Trim if the target is strict.
6. **F9-39 ThreadSanitizer.** The shipped `group_threads.cpp` is race-free, so the TSan run exits 0. The lab asks students to remove the lock and see the race. Decide whether a deliberately racy evidence file should also ship.

## RB304
Source: [`chapters/RB304/NOTES.md`](chapters/RB304/NOTES.md)

### Decisions for the owner

1. **Root in the container.**
   - SCHED_FIFO, SCHED_DEADLINE (F9-46) and `mlockall` measurements ran as root. The limit is `RLIMIT_RTPRIO` 0, so an ordinary user would be refused.
   - The chapters say so, and the labs' troubleshooting explains the user limits.
   - Decide whether learners' lab machines will grant real-time priority to a lab group.
2. **VM measurements as teaching evidence.**
   - All timing numbers come from a shared Firecracker VM without PREEMPT_RT. They show the effects (load, priority, page faults, structure) clearly but are not representative of robot hardware.
   - The F9-47 two-kernel comparison and the F9-50 jitter requirement need a real machine. Consider a reference lab PC with a PREEMPT_RT kernel to produce reference histograms.
3. **The F9-50 jitter requirement fails on the build container.** R1 needs ≥ 99.9 % of periods within ±100 µs; measured 96.74 % and 97.86 %. R2 needs no period above 2 ms; measured 9 and 5.
   - This is presented honestly as a platform failure (R3, the code's own time, passes) and as the exam task on real hardware.
   - The R1–R3 values are this chapter's lab plan. The guide says "rate set in the lab plan"; the owner may want to set the official values.
4. **Own frame format "uframe" in F9-51 instead of micro-ROS.** micro-ROS and ROS 2 are not installed. The chapter teaches the node structure (tasks, ring, framing, sequence numbers) with the course's own format and keeps every micro-ROS name in unverified boxes. A real micro-ROS lab needs a supported board and the micro-ROS toolchain.
5. **uRTOS copied unchanged from F3-39 into `labs/F9-51`.** The files are `board.h`, `startup.cc`, `urtos.h`, `urtos.cc` and `os305.ld`. A copy keeps the lab self-contained, but changes to F3-39's uRTOS will not propagate. Decide between copies and a shared lab library.
6. **Miniature frameworks instead of ROS 2 and ros2_control.** These are `uexec.h` (executor and callback groups) and `uctl.h` (hardware interface, controller, controller manager). They use our own names and are marked as such. When ROS 2 is available, a follow-up should port the F9-49 and F9-50 labs to rclcpp and ros2_control and re-verify the unverified boxes.
7. **Glossary overlaps.**
   - RB303 defines "Executor (ROS 2)" and "Callback group". F9-49's jargon box redefines both for this chapter's context, but links to RB303's glossary entries and does not add duplicates.
   - "Response time", "Slack", "Tail latency" and "Page fault" are likewise left to their existing courses.
   - "Percentile" (RB304) sits next to "Median and percentile" (elsewhere).
8. **Course lab "latency histograms with and without PREEMPT_RT on the same machine"** is delivered as a model plus a procedure, with no measured RT histogram. It needs hardware time to complete.

## RB401
Source: [`chapters/RB401/NOTES.md`](chapters/RB401/NOTES.md)

### Decisions for the owner

1. **Story characters.** These names are not in any registry I could find. Please confirm or replace them:
   - Lina appears in F9-52, F9-53, F9-57 and F9-58.
   - Ravi appears in F9-54, F9-55 and F9-56.
   - Mira (Ravi's cousin) appears in F9-55.
2. **Own simulators instead of ROS 2.** The build container has no ROS 2, so the labs use our own C++ simulators. If the kit later provides ROS 2 with Gazebo, Nav2 and MoveIt 2, the optional "untested" lab steps should be written and run then, with verified names.
3. **Forensic of F9-58 exits 0.**
   - `bt_bug` exits 0 because the faulty tree reports SUCCESS.
   - `fetch_bt` also exits 0 although run 2 does not deliver the cup, because it checks only root statuses. The chapter uses this as a teaching point (lab "Expected observations").
   - If you prefer the lab program itself to check the cup, change line 14 of `fetch_bt.cpp`. That would make its exit code 1.
4. **Exam P.** The course card's practical "configure navigation for a new map" is rehearsed only as the F9-56 mini-project on the mini stack. A real exam on Nav2 needs a ROS 2 installation and verified parameter names.
5. **Word counts.** Prose without tables is about 4,800–6,300 words per chapter. F9-57 and F9-58 are slightly under the 5,000-word L4 target if tables are excluded, and above it if tables are included.
6. **Prerequisite chapters.** The `prereqs` lines name individual chapters of RB301 (F9-23 to F9-27), RB302 (F9-31, F9-34, F9-36), RB303 (F9-43) and RB304 (F9-49) where the text relies on them. Please check that these chapters, once written, cover what is assumed.
7. **Shared code.** `labs/F9-55`, `F9-56` and `F9-57` include headers from `labs/F9-52` and `labs/F9-54` by relative path (`../F9-54/planner.hpp`, `../F9-52/house.hpp`). Moving lab folders would break them.

## RB402
Source: [`chapters/RB402/NOTES.md`](chapters/RB402/NOTES.md)

### Decisions for the owner

1. **No MPC text in the source registry.** F9-64's MPC content is tagged D1 (own derivations plus runs) or sits in an unverified box. Please add an MPC book or survey to the registry, or accept a concept chapter backed by derivations only.
2. **"Maps to" milestone.** The card maps to "Feedback Systems", which is a book (registry 4.7), not a SYSTEMS_CURRICULUM milestone. The meta "maps" line says "no Systems Curriculum milestone". Confirm, or name a milestone.
3. **Exercise parameters.** The cart-pole values, the 10 N and 20 N limits, the encoder resolutions (1 mm, 2048 counts per turn), the 2 N parking cart and the 4-sample delay are all invented for teaching. If the RB lab kit has a real cart-pole, its datasheet values should replace them, and all runs must be repeated.
4. **Glossary naming.** DS402 already defines "Observability" with the monitoring meaning. RB402 uses "Observability (control)" and its definition points to the other meaning. "Steady-state Kalman gain (observer design)" was chosen to leave "Kalman filter" to RB302.
5. **B2 in F9-63** cites "Probabilistic Robotics" and the Kalman 1960 paper, both from the registry and title only. Confirm these are the intended sources for the Kalman material.
6. **Course project placement.** The course project ("a balancing controller with a written stability argument") is written as F9-62's mini-project, with a rubric. F9-63's mini-project extends it to output feedback. Say if a separate course-level project page is wanted.

## RB403
Source: [`chapters/RB403/NOTES.md`](chapters/RB403/NOTES.md)

### Course-wide decisions taken (owner to confirm)

1. **The "robot OS image" is the course's bare-metal AArch64 kernel, run on QEMU virt as the course board.** Booting a Linux image was impossible here. Each chapter says so openly. The Linux image pipeline appears only as text, in unverified boxes.
2. **The kernel base was copied from DR403's F4-31 lab.**
   - Copied files: `start.S`, `kbase.h`, `kbase.cc`, `kernel.ld`, `fdt.h`, `minidtc.py`.
   - Each copy has one provenance line added. The copies live in `labs/F9-67/`.
   - The labs of F9-68 to F9-71 source the shared helpers in `labs/F9-67/rblib.sh`. The run.sh of F9-69 rebuilds the F9-67 image from its sources.
   - If DR403's files change, these copies do not follow.
3. **The devicetree uses the vendor prefix "course,"** (`course,robot-board`, `course,usb-camera`). It is the course's own invented prefix and not a registered vendor.
4. **QEMU stands in for the e-stop.** On virt with `acpi=off`, the monitor command `system_powerdown` pulses the gpio-keys power key on PL061 line 3. This behaviour was observed in the runs; the QEMU documentation was not opened. The chapters state that this is not a real e-stop.
5. **RB304 terms** (Jitter, PREEMPT_RT, Wake-up latency, Real-time throttling, Threaded interrupt handler, Latency histogram):
   - F9-65 to F9-67 link to RB304 *chapters*, because RB304's glossary.json did not exist when they were written.
   - It exists now, so F9-70 links `#gl-real-time-throttling` directly.
   - "Latency histogram" is defined in both RB304 and RB403. build.py merges duplicates, and RB304's definition wins; F9-66 is added to its chapter list.
6. **Proposed new analogies for the registry** (faculty F9: bicycle and body):
   - reflex arc = the microcontroller loop (F9-65, F9-66);
   - the image = the bicycle prepared and packed, with its ticked checklist (F9-67);
   - getting on the bicycle in order, brakes checked before pushing off = start-up order and readiness (F9-68);
   - the riding partner who shouts "stop" when you stop answering = watchdog (F9-69);
   - the cycle computer's recording and its gap = flight recorder (F9-70);
   - the living-room trainer versus the gravel road, and the wrongly set wheel size = sim-to-real and a calibration error (F9-71).
7. **Image builder for Linux robot images: Buildroot, Yocto, or distribution packages.** This is left to the owner and the MP6 teams (F9-67 unverified box). Neither tool was run, and the chapters give no command or variable of either.
8. **Safety position.** Every chapter states that the hard-wired e-stop chain has no software in its path. F9-69 states that the course's supervisor is a teaching model, not an assessed safety function. Standards are cited by title only:
   - ISO 13850
   - IEC 60204-1
   - ISO 13849-1
   - IEC 61508
   - IEC 61800-5-2
   - ISO 10218
   - ISO 13482

   No requirement of any of them is claimed (AH-6). Faculty or safety-owner review is recommended before teaching F9-69.

## DN201
Source: [`chapters/DN201/NOTES.md`](chapters/DN201/NOTES.md)

### Decisions left to the owner

1. **Named simulator for validation.** The card's "validated against a named simulator" is not done. The owner must choose the simulator (Gazebo with PX4 SITL, ArduPilot SITL, or another) and its version. A later build should compare hover, a roll step and the torque-free spin against it. F10-04's lab describes the comparison.
2. **What "a propeller mounted on the wrong motor" means.** The forensic case models it as a propeller from another vehicle fitted on motor 3 (kT ×1.2, kQ ×1.6). That fault makes the vehicle yaw as thrust changes.
   - A wrong-hand propeller (CW on a CCW motor) would push air upwards and flip the vehicle at take-off rather than yaw in a climb, so it was not used.
   - Two propellers swapped between motors of the same spin direction would give no symptom.
   - The owner should confirm this reading of the card.
3. **Axis convention.** DN201 uses z-up axes, with positive pitch nose-down and positive yaw nose-left. The order is R = Rz·Ry·Rx, matching MA201. Beard and McLain and most flight software use NED. The owner should decide whether DN301 and later courses switch to NED, and say where the conversion is taught.
4. **Two motor time constants.** The simulator in `quadsim.hpp` uses a 30 ms motor lag, an exercise value set before F10-05. F10-05's motor model gives 17.41 ms at hover. F10-05 says so and its lab replaces the lag. The owner may want the shared header changed to the derived value. That needs F10-02, F10-03 and F10-04 to be re-run and their quoted numbers re-checked.
5. **Course project.** The course project, "a 3D multirotor simulator with tests", is defined by F10-04's `quadsim.hpp` and `sim3d_tests.cpp`, plus the extensions in the F10-04 and F10-05 labs. DN301 is expected to fly controllers in it. The owner should confirm that DN301 will reuse this header and these conventions.
6. **Exam item P** (derive the hover thrust condition and test it in simulation) is covered in three places:
   - F10-01 derives the condition;
   - F10-04 test T3 checks the tilted-hover thrust T = mg/(cos roll · cos pitch);
   - F10-01's lab runs the 1D simulation.

## DN202
Source: [`chapters/DN202/NOTES.md`](chapters/DN202/NOTES.md)

### Decisions for the owner

1. **Course kit.** Choose the flight controller, sensors, ESCs, motors, power module, LiPo packs, charger, RC system and telemetry radios. Every Part B and the course project depend on this choice. Until then, D-sources marked "Not yet chosen" stay open.
2. **Forensic log for F10-09.** The course card asks for a real PX4 ULog or ArduPilot DataFlash log. None was available, so the chapter uses synthetic CSV logs and has an unverified box saying so. Please supply a real log, with a licence to redistribute it, and the tool version used to read it.
3. **Teaching protocols.** U-Shot (F10-10) and U-RC (F10-12) stand in for DShot and real receiver protocols. Decide whether to keep them or, once the sources are opened, add a verified section on the real formats.
4. **Physical constants in F10-08.** Confirm them against the cited references.
5. **Word count.** Each chapter is about 5,500–6,000 words including tables, code tables, answers and keys. The prose alone is close to the L3 target of about 4,000 words.
6. **Cross-course links.** These chapters link to DN201 (F10-01, F10-02, F10-06), OS305 (F3-37, F3-39, F3-42), HW204 (F1-42, F1-44, F1-47, F1-48), HW302 (F1-64, F1-65, F1-69, F1-70, F1-71) and RB201 (F9-15).
   - They mention later chapters (F10-30, F10-35, F11-16) and courses (DN301, DN303, DN401) as plain text, not links, because those chapters do not exist yet.
   - The glossary links to the existing terms Damping ratio (MA301), Decibel (dB) for gain (MA301), and the HW302 battery, IMU, ESC and PWM terms. These are not redefined.
7. **Course project and exam P.** F10-12 holds the course project with a weighted rubric that combines the six dossier parts, and it mentions exam P. The owner should check the weights.

## DN301
Source: [`chapters/DN301/NOTES.md`](chapters/DN301/NOTES.md)

### Decisions for the owner

1. **Forensic evidence format.** The forensic logs (including the course forensic "Oscillation") are printed by the course's own programs in a plain-text table, not real PX4 ULog or ArduPilot DataFlash files. When PX4 SITL is available, regenerate them as real logs and add an exercise on reading those tools' formats.
2. **PX4 SITL lab.** The course card asks for "PX4 SITL tuning". It is written as an optional, untested part B in F10-17, with all names in an unverified box. It needs a build environment with PX4 and a simulator, and a verified parameter list.
3. **Recording size.** `F10-13/imu_flight.csv` is about 183 KB, generated by a listing at lab time rather than stored as a fixture. The owner may prefer a smaller recording (for example 50 Hz) or a committed copy.
4. **Gains and loop rates.** The cascade's gains and loop rates (500/250/50 Hz) are this course's exercise values for the simulated course quad. They are not autopilot defaults, and the chapters say so.
5. **Delay value.** The 6 ms loop delay in F10-19 is an exercise value; a measured delay from a real vehicle would make the margin table concrete.
6. **Dependency on DN201's simulator.** Every DN301 lab needs `labs/F10-04/quadsim.hpp`. Any interface change there breaks DN301; consider freezing that interface.

## DN302
Source: [`chapters/DN302/NOTES.md`](chapters/DN302/NOTES.md)

### Decisions for the owner

1. **Course-forensic parameter names.** The brief says "Parameter names may come only from the PX4 documentation". The documentation could not be opened, so the forensic log uses the course's own names (`UNI_GPS_OFF_X` and the other `UNI_…` parameters), clearly labelled as not PX4's. The PX4 name (believed `EKF2_GPS_POS_X`) appears only inside an unverified box. Proposal: when the Source Researcher confirms the PX4 names, either keep `UNI_…` with a mapping table, or rename them in `flight_sim.cc` and regenerate. The forensic logic does not depend on the name.
2. **ULog-style format.** `ulog_lite.h` follows the ULog structure from memory: header magic `ULog 01 12 35`, size+type messages, and only the F/I/P/A/D/L message types. Byte compatibility with pyulog or Flight Review is **not verified**, and the chapter says so. A verifier should check it with a real `.ulg` file. Option: once verified, make the writer compatible so learners can open course logs in PX4's tools.
3. **No real PX4/NuttX in the build.**
   - All labs use the course's models: the POSIX threads stand-in for NuttX tasks, the work-queue simulator, uorb_lite, mini_px4, the lockstep SITL, the ULog-style logs, and the board checker and bring-up model.
   - The real-tool parts are unverified boxes.
   - The course lab (PX4 SITL, then the bench flight controller) and practical exam P need a machine with PX4 and a flight controller before G5 can be "tested" rather than "untested-on-hardware".
   - This needs owner approval under AH-26.
4. **Our own module parameter `TG_MAX_TILT`** (F10-24) is named in a PX4-like style but is ours. The chapter says this. Topics in the course runtime end in `_model` so they are not mistaken for PX4 topics.
5. **Analogy mappings proposed** (F10 world "carrying a tray of drinks while walking"; uORB = notice board is already registered). Proposals:
   - RTOS scheduler = the body's rule that the balance reflex always wins over chatting; flat build = everyone carries from one shared tray (F10-20);
   - work queue = one helper's list of quick jobs, done in order; work item = one quick job on that list (F10-21);
   - queue length = how many of the newest notes stay pinned; generation counter = the running number written on each note (F10-22);
   - SITL = rehearsing the walk in the hallway with plastic cups; lockstep = the coach who says "step" and waits (F10-23);
   - a module = one helper with one job who reads notices and pins up their own (F10-24);
   - a ULog = a photograph of every notice each time one changes, plus the settings at the start (F10-25);
   - porting = learning a new tray for the same walk (F10-26).
6. **Chapter length.** Prose is about 5,200–6,800 words per chapter (L4 target about 5,000). F10-20 is the longest (about 6,800); trim it if the editor wants a tighter chapter.
7. **Exams.**
   - Q: the check-yourself questions are a starting pool.
   - F: each chapter's forensic lab, plus the course forensic in F10-25.
   - P: "add and test a module in SITL" is previewed in F10-24's lab. No separate exam file was written, because the brief did not list exam files as deliverables.
8. **Link to F10-27** (DN303) for "next" in F10-26. It resolves through the catalogue anchor until DN303 is written.

## DN303
Source: [`chapters/DN303/NOTES.md`](chapters/DN303/NOTES.md)

### Decisions for the owner

1. **Course card versus build reality.** The course card asks for an ArduPilot SITL lab, a companion program speaking MAVLink to SITL, and a P exam (a MAVLink client for mission upload in SITL). None of these could run here. Each chapter teaches the mechanism with the university's own programs, and makes the real SITL or MAVLink step "Part B, untested in this build". Please confirm this approach, or schedule a build with ArduPilot and pymavlink installed so that Part B can be run and its outputs recorded.
2. **Lost-link forensic.** The answer key's conclusion depends on the vehicle counting any system-255 heartbeat when no single controlling ground station is configured. This is presented as the mechanism to check against ArduPilot's documentation for `SYSID_MYGCS` and the GCS failsafe, not as a claim about ArduPilot. The Source Researcher should verify it before the chapter claims more.
3. **U-FC1 name.** F10-29 reuses the name of DN202's pretend board U-FC1, with a new pretend MCU, U-MCU1. Keep it for continuity, or rename it.
4. **Word counts.** Counts are prose plus tables, measured by the scratch validator: F10-27 about 7,370; F10-28 6,290; F10-29 5,300; F10-30 5,310; F10-31 5,160; F10-32 4,710. F10-27 and F10-28 are above the L4 target of about 5,000 words. Trimming candidates are F10-27 Layer 3 and the line-by-line tables.
5. **F10-27 correction.** An early draft claimed that stack painting under-reports whenever a buffer is left unwritten. The sweep run (R7) showed the mark stays correct while deeper writes land inside the stack. It under-reports only when the overflow jumps past the stack. The chapter now states the measured behaviour.
6. **Builder warnings outside DN303.** `build.py` reports PROBLEM lines for other courses (for example broken `#gl-…` links from SE courses, an F10-38 run, and a duplicate `gl-finite-state-machine-fsm`). These were left untouched, as instructed.

## DN401
Source: [`chapters/DN401/NOTES.md`](chapters/DN401/NOTES.md)

### Decisions and conventions

- Shared simulator: `labs/F10-33/dronesim.hpp`, namespace `dn`.
  - It has an invented battery (OCV curve, R_int 0.025 Ω), a power model of 265·T^1.5 + 8 W,
    drag 0.35 1/s and tau 0.15 s.
  - The compass interference grows with current and is fixed in the body frame.
  - The vehicle has no yaw dynamics.
  - Parameter names (`rc_timeout`, `batt_low_pct`, ...) belong to the course only. The
    chapters say so.
- Failsafes only escalate (Mission < Hold < RTL < Land). Side effect: a breach of a
  lower-rank failsafe is not logged. F10-38 Log A uses this on purpose (fence breach at
  177 m, no event).
- Course text formats were invented for teaching: safety-case and readiness-record `.in` files,
  and the CSV log with `#` header lines.
- The freshness rule (doc 1 year, day 12 h, field 60 min) is the course's teaching rule. It is
  not a recommendation from any document.
- Story continuity across chapters: Team B's v1.5/91be build with new power wiring appears in:
  - the F10-36 forensic case (hazard H5, unargued);
  - the F10-37 forensic record (2026-09-27, NO-GO);
  - F10-38 Log A (flyaway, 177 m).

  Team C's 2,000 mAh capacity setting for a 1,500 mAh pack appears in F10-38 Log B.
- The F10-34 HITL rehearsal uses the course's own framing (`0xA5 | type | seq | len |
  float32 | CRC-16/CCITT-FALSE`), not MAVLink. This is stated in the chapter.

### Decisions for the owner

1. Approve the course text formats (safety case, readiness record, CSV log) and the freshness
   rule, or replace them.
2. Unify the F10 character names across DN301 and DN401.
3. Add the three sources above to the registry, or replace them.
4. Decide whether "Safety case (flight operation)" and "Risk matrix (severity × likelihood)"
   stay separate from SS402's terms.
5. The PX4/ArduPilot HITL, failsafe and log details stay in unverified boxes until someone
   opens the documentation for the release that MP7 targets.

## SS301
Source: [`chapters/SS301/NOTES.md`](chapters/SS301/NOTES.md)

### Decisions for the owner (summary)

1. **H4's power-cut wording** (F11-04): finish the swap or undo it?
2. **A real TF-A + OP-TEE lab** on QEMU when network access is available (F11-05).
3. **swtpm instead of `mock_tpm.py`** for C13 when available (F11-03), so that quotes and sealing can run on a real TPM implementation.
4. **Committed test keys** in three lab folders. They are kept on purpose; please confirm.
5. **Exams Q and F:** use the chapters' Check yourself and forensic labs as the item pool, or write a separate exam page?
6. **Analogy proposals** above: accept, change or reject.

## SS302
Source: [`chapters/SS302/NOTES.md`](chapters/SS302/NOTES.md)

### Decisions for the owner

- **Course forensic "the crash input"** is placed in **F11-09** (fuzzing): a fuzzer-found
  input + sanitizer report, minimised and fixed. Each other chapter also has its own
  forensic lab from a real run, as the template requires.
- **The lab fuzzer is the university's own** small coverage-guided mutation fuzzer
  (`fuzz.cc` + `cov.cc`): AFL-style shared-memory edge bitmap across `fork()`, targets
  built with `-fsanitize-coverage=trace-pc`, fork-per-input so a crash kills only the
  child. No external fuzzing library (libFuzzer's runtime was not linkable in the
  container — see below). This is our code and we ran it; it is the honest teaching
  version of libFuzzer/AFL++, and the chapters say so and point to the real tools.
- **MAVLink and USB details are deliberately NOT reproduced faithfully.** The MAVLink
  frame scanner uses a toy checksum, not the real CRC-16 + CRC_EXTRA; the USB descriptor
  parser uses a simplified layout. Both carry unverified boxes naming the MAVLink
  Developer Guide and the USB specification. The taught bug (trusting a wire/device length)
  is independent of these details. A Source Researcher should confirm the real framing
  before any of this is used against real vehicles or devices.
- **SMEP/SMAP** are kernel features already built and tested under QEMU in **OS303
  (F3-29)**. F11-10 cross-references that evidence and marks them untested-in-this-lab
  rather than duplicating a kernel boot in this host-only course.

## SS401
Source: [`chapters/SS401/NOTES.md`](chapters/SS401/NOTES.md)

### Decisions for the owner

1. **Course-card forensic: "a MAVLink log".**
   - **Problem:** real MAVLink logs cannot be produced in this build (no SITL, no pymavlink).
   - **What I did:** the forensic lab uses logs from the course's own simulated link,
     "ULink". They have MAVLink-like fields (system id, component id, 8-bit sequence) but
     are explicitly not MAVLink.
   - **For you:** approve this, or schedule a SITL-generated MAVLink log (SITL plus a
     second sender) when SITL is available. F11-16 lab step 5 describes it.
2. **Teaching crypto.**
   - **Problem:** the labs needed a MAC, and HMAC had to be written for them.
   - **What I did:** SHA-256 and HMAC are implemented in `hmac.h` for teaching. They are
     cross-checked against Python. The chapter warns against product use.
   - **For you:** approve, or require the labs to call a library such as OpenSSL. That
     would add a build dependency that run_lab.sh does not have.
3. **Course vocabulary for claimed controls.**
   - **What I did:** `tm.h` uses `auth`, `mac`, `audit`, `enc`, `ratelimit` and
     `leastpriv` to mark controls that a diagram claims. This is the course's own
     vocabulary, not a standard.
   - **For you:** keep it, or replace it with a vocabulary from the book once the book is
     checked.
4. **No numeric risk scoring.**
   - **What I did:** the course deliberately avoids numeric risk formulas such as
     probability times impact. It ranks threats by boundary first, then by assets and
     attack trees, and gives the reason in F11-13 Layer 3.
   - **For you:** confirm this is wanted.
5. **Practical exam (P).**
   - **What I did:** F11-17 describes the format of the timed threat-model exam. The time
     limit is left to the exam paper, and no paper or key was written.
   - **For you:** the Exam Writer still has to produce the paper and its key in `_keys/`.
6. **Chapter length.** The chapters are about 5,800–6,500 words of page text each,
   including tables. Prose alone is near the L4 target of about 5,000.

## SS402
Source: [`chapters/SS402/NOTES.md`](chapters/SS402/NOTES.md)

### Decisions for the owner
1. **Course target scale.** Approve or replace the course risk graph (points = 2S+E+A; CT0 ≤7, CT1 8–9, CT2 10–11, CT3 12–13) and its CT rules:
   - CT1 needs its own input, a proof test, tested requirements and a second-learner review.
   - CT2 adds a diagnostic, a PFD argument, traceability on the release build, MC/DC on the decision logic, mutation tests and a reviewer from another team.
   - CT3 is never built by learners and must be designed out.
2. **Course robot safety case.** The case keeps two goals open:
   - G32: the hard-wired e-stop chain timed on the real robot.
   - G7: teach-mode pinch, which is the F11-23 mini-project.

   F11-22's trace keeps T1 (hardware) as a gap on purpose. MP6's R3 gate is the natural place to close them. Please confirm that R3 should require this.
3. **New sources.** Add D4 to D6 and STPA (listed above) to the registry, or tell me to remove them.
4. **Link to BR-08.** F11-22 links to BR-08, which is not written yet. Check that the catalogue entry resolves it, or remove the link.
5. **Practical exam.** The P exam (safety-case review) uses the checklist and rubric weights in F11-23. The exam cases and keys still need to be written and kept outside the learner pages.
6. **MP7 regulation.** The MP7 project brief tells learners to cite their national aviation authority's current rules from the authority's own document. No regulation is stated in the chapters.

## SE301
Source: [`chapters/SE301/NOTES.md`](chapters/SE301/NOTES.md)

### Decisions for the owner

1. F12-03 quotes timing numbers from `lookup_alt_O2.out` / `lookup_alt.out`; they are specific to the build
   container. Re-running the lab changes them and the prose must then be updated (the chapter says so).
2. The course forensic ("the review that missed the bug") lives in F12-04; F12-05 has a second forensic
   ("resolved, but not fixed").
3. The course project lives in F12-05's mini-project section.
4. F12-04 reviews F4-04's `rtc.cc` and `rtc_decode.h` by reference (`data-src="F4-04/..."`); those files
   are read-only for this course. If F4-04 changes, the line numbers in F12-04's comments must be checked.
5. The forensic scenarios (F12-04 robot frames, F12-05 data logger) are fictional; all evidence comes from
   real runs of the lab code. Commit ids in threads (8b41c07, e41c9b7) are story ids, not repository commits.
6. F12-05 links forward to F12-06 (SE302), a planned chapter at the time of writing.

## SE302
Source: [`chapters/SE302/NOTES.md`](chapters/SE302/NOTES.md)

### Decisions for the owner

1. **Word counts.** All chapters exceed the level targets by 25–50 %. The bulk
   is in the code walk-through tables and the forensic keys. Trim, or accept.
2. **Own fuzzer instead of libFuzzer.** clang's libFuzzer runtime is missing in
   the build container, so F12-08 uses a 140-line fuzzer built on GCC's
   `trace-pc` hook. If the dossier wants libFuzzer (or AFL++) taught by name,
   the container needs `libclang-rt-18-dev` (package name from memory) and a
   re-run; the chapter already explains how production fuzzers differ.
3. **CI service.** No hosted CI service was available or named; `ci.sh` is a
   plain script that any service can call. Decide whether the university
   standardises on one service and adds an unverified-then-verified appendix.
4. **Glossary overlaps.** "Coverage-guided fuzzing" and "Corpus" were defined
   by SS302 during this build; SE302 links to them. "Software in the loop
   (SITL)" (DN401) and "Hardware-in-the-loop (HIL)" (RB403) are the shared
   definitions; F12-07's jargon box shows them with the SIL/HITL synonyms but
   does not add glossary entries.
5. **Course-lab scope.** The card asks for "A CI pipeline running QEMU kernel
   tests with timeouts (curriculum 0)". F12-09's lab adds harness self-tests,
   a GPU build/skip stage and the robot matrix stage. Confirm this is wanted,
   or cut stages 5–7 for time.
6. **Exam P (write a test plan for a provided system).** No separate exam
   artefact was written. Each chapter's mini-project builds part of a test
   strategy; F12-10's mini-project combines them in the 5.5 format. The exam's
   "provided system" could be the F12-06/F12-07 robot (governor + parser +
   simulator) or the F12-09 test kernel; both have full sources in the labs.
7. **Story names** (Lina, Ahmed, Amira) are invented characters; no real people.

## SE401
Source: [`chapters/SE401/NOTES.md`](chapters/SE401/NOTES.md)

### Decisions for the owner

1. **Noisy real runs.** The real timing labs ran on a shared VM with other agents. Some outputs have visible noise: `predict_real` U = 0.85 spike, `ab_bench` map rounds 520–826 ms, the `chunked` kvsvc run hit by slow fdatasyncs, and a poor USL fit for `scale`. The chapters use this noise as teaching material and say so. Decide whether to keep these recorded runs, or re-record them on a quiet machine and then update the prose listed above.
2. **Constructed exercise data.** `bootstrap.in` and `usl.in` are typed-in data, marked as constructed in the file and the prose, because the build machine cannot produce clean data for these methods. Confirm this is acceptable under AH-23.
3. **kvsvc as the MP2 stand-in.** The course lab ("end-to-end profile of MP2 or MP3") uses a single-process service with no network or replication. Decide whether the MP2 lab folder should later provide a real multi-node target.
4. **"USE method" defined twice** (SE401 F12-14 and SE402 F12-17) under the same term name. `build.py` merges them, and SE401's text is shown. Choose one wording.
5. **Length.** The chapters exceed the L4 prose target, mainly because of tables and answer keys. Trimming candidates: F12-14 Layer 3 (three-mode table plus discussion) and F12-11 worked example.
6. **Build.** `python3 university/build/build.py` reported no PROBLEM line for F12-11 … F12-15 or SE401. The only PROBLEM line was `duplicate id: gl-finite-state-machine-fsm`, from another course. The build rewrote `university/UNIVERSITY.html`.

## SE402
Source: [`chapters/SE402/NOTES.md`](chapters/SE402/NOTES.md)

### Decisions for the owner

1. **Analogy extensions (F12 world, house built by a team)** — only "post-mortem = fire drill"
   is registered. Proposed and used:
   - F12-16: a burst pipe on site with roles (one person directs, one shuts the water, one
     talks to the owner, one writes down) = incident roles.
   - F12-17: tracing a water stain to its leak by tests that split the possibilities =
     debugging under pressure, bisection.
   - F12-19: reading the published inspection report of another builder's sagging roof, and
     asking "do we build roofs like that?" = learning from documented failures, transfer.
   Please register or replace.
2. **Proposed registry sources:** Dekker (Field Guide), Reason (Human Error), Ohno (TPS),
   Gregg (Systems Performance), git documentation, FEMA ICS material, and the four
   investigation documents of F12-19 (ESA/CNES Ariane 501 board report, NASA MCO MIB Phase I,
   GAO Patriot/Dhahran report, Leveson & Turner IEEE Computer). Copyright: quote short
   excerpts only.
3. **Real game day on MP2/MP6 and bisection of a real repository** are learner activities,
   untested in this build (the labs use deterministic models). Decide whether a reference
   MP2 deployment should be provided for the timed game day and Exam P.
4. **Course project** split across F12-18 (part 1: post-mortem of a learner's own failure) and
   F12-19 (part 2: transfer analysis), with the default project rubric (guide 11.5).
5. **Exam P (incident simulation)**: `gameday.cpp` accepts other scenario files; examiners
   could write unseen ones. No examiner scenario was written.
6. **SE401 links**: SE401 chapters did not exist when these were written, so F12-17 links
   to the course anchor `#SE401` rather than to a chapter id; update when SE401 lands.
7. F12-19 links forward to `#F12-20`, `#SE403`, `#SE404` (existing per the curriculum).

## SE403
Source: [`chapters/SE403/NOTES.md`](chapters/SE403/NOTES.md)

### Decisions for the owner

1. **Mapped codebase.** F12-20 maps the Python standard library (present in the container) instead of Linux/PX4/ROS 2,
   which were not available. The forensic "where does this log line come from?" uses http.server's real log line.
2. **NVMe evidence.** The "read() to an NVMe command" lab is split: the Linux half uses kernel stack samples on the
   container's virtio-blk disk (no NVMe there); the NVMe half uses QEMU's emulated controller with SeaBIOS as the host,
   not Linux. A full Linux-guest-on-emulated-NVMe run (needs a guest kernel image) or real hardware would close the gap.
3. **uORB lab as a model.** PX4 and ROS 2 could not be installed; the uORB path is a discrete-event model clearly labelled
   as such. Please decide whether to require a PX4 SITL run in a later build.
4. **Root needed.** read_stack reads /proc/self/task/<tid>/stack, which needs root (the build ran as root).
5. **Outputs that vary per run.** read_stack, pin_attempt and the server_log dates change each run; prose avoids depending
   on exact values from them.
6. **Repository file.** `labs/F12-21/sample.txt` (22,000 bytes, generated) is committed as lab input.
7. **Course-level items.** The timed code-reading exam (exam P) and the course project are described across the three
   chapters' mini-projects (project parts 1–3); no separate exam paper was written.
8. **Forensic code in F12-22** (`status_cleanup.hpp`) was written for the lab; the answer key says so.

## SE404
Source: [`chapters/SE404/NOTES.md`](chapters/SE404/NOTES.md)

### Decisions for the owner

1. **Safeguarding for the peer-mentoring lab (blocking for running the SE404 course lab).** Mentees in Years 1–2 may be children. The chapter states minimum course rules (shared visible places or approved online sessions; no private channels; a peer mentor is never the supervising adult for physical labs; same-day reporting to a named contact) and asks the owner to publish the programme's policy with the people responsible for child protection in the owner's organisation. Also: who signs the lab's verification item 5.
2. Add the ACM and IEEE codes of ethics to the source registry (4.9).
3. The status-colour rule (earned/planned: green ≥ 0.90, amber ≥ 0.75, red below) is presented as a course convention; confirm or replace it university-wide (it is also suggested for MP8 status reports).
4. Commitment percentiles (P50 for internal milestones, P80–P90 for the fixed-date MP8 defence) are presented as this course's recommendation for learner projects, not a standard; confirm.
5. Session-record fields and flag rules of `mentor_log.cpp` (F12-25) are the course's own; confirm they may be used in the peer programme and how records are stored (privacy of mentees).

## SE501
Source: [`chapters/SE501/NOTES.md`](chapters/SE501/NOTES.md)

### Unverified boxes (need owner approval, AH-19)

1. F12-28 Layer 2: "walking skeleton" and "contract test" are this course's own
   definitions; no source opened defines them or attributes them.
2. F12-28 Layer 3: the ratio-based re-forecast of Listing 1 is a model with stated
   assumptions; its predictive accuracy is not established by any source.
3. F12-30 Layer 3: how Raft (or any real protocol) guarantees that a new leader holds
   every acknowledged/committed write is not stated; the paper was not opened.

### Decisions for the owner

1. **Analogy mappings (F12, building a house as a team) proposed for registration** — not
   in guide 8.1 yet:
   - mega project = a whole house built by several trades; component = one trade's work;
     interface contract = agreed position and size of openings where pipes and cables
     cross; contract test = checking the hole with the agreed pipe before the plumber
     comes; walking skeleton = one finished room with water, power and a door; hidden
     work = moving a pipe nobody wrote on the job list (F12-28).
   - design-review gate = building inspection at a fixed stage; entry criteria = walls
     still open for the wiring inspection; pass with conditions = "move in, railing fixed
     by Friday"; not yet = "call me when the alarm works"; safety item = the smoke alarm
     (F12-29).
   - final defence = handover walk-through; demonstration script = the walk-through
     route; recorded fallback = folder of inspection records; claims ledger = promises
     with certificates; stated limitation = "the attic is not insulated yet" (F12-30).
2. **Gate content is a course proposal.** The guide fixes only the gate names, written
   approval, and the MP3/MP6/MP7 examples. The per-gate entry criteria, the three
   outcomes (PASS / PASS WITH CONDITIONS / NOT YET), "safety items are never conditions"
   and "only independent reviewers count" are this course's rules. Please confirm or
   amend; they should then go into guide 11.6.
3. **Running example.** All three chapters use one MP8: a drone ground station backed by
   a replicated store (MP7 + MP2, one of the card's examples), with team names Amara,
   Joon, Farah. The store in F12-30 Listing 1 is our own deterministic teaching model,
   explicitly not Raft.
4. **Measurement threshold.** F12-30 Listing 2 uses 20 runs (the worked example's R1
   plan, after curriculum 13.4's GPU measurement protocol). Learners are told to use
   their own plan's number.
5. **Defence format** (length, audience, who examines) is not fixed by the guide; F12-30
   gives only an order of parts (Figure 2) and leaves lengths to the reviewers.

## BR-01
Source: [`chapters/BR-01/NOTES.md`](chapters/BR-01/NOTES.md)

### Unverified boxes in the chapter (owner approval needed before publication, AH-19)

1. Layer 2: the exact list of C++ features and standard-library facilities supported in device
   code (Programming Guide, C++ language support section), and whether NVIDIA's documentation
   calls `__host__`/`__device__`/`__global__` "execution space specifiers" or "qualifiers" (the
   bridge card says "qualifiers"; the chapter uses "specifier" and flags it).
2. Code walk-through: Listings 4 and 5 untested on hardware; expected GPU outputs stated, not observed.
3. Trap 1: what an early host read of managed memory really returns on a GPU; the role of
   `cudaDevAttrConcurrentManagedAccess` (named in the header, rules not read).
4. Trap 3: what happens at run time when a kernel receives a host pointer (cudaErrorIllegalAddress
   is described in the header; whether and where this program gets it, or whether a system with
   special memory support can access host memory, was not observable).

### Decisions for the owner

1. **Length.** The prose is long for L2 (about 8,900 words counted with the jargon box, sources
   and answers; the brief asks for about 3,000). The bridge card requires the "Carries over",
   "Changes" and "Traps" lists each in depth with a real run per trap, plus a five-step lab and
   three breaks; I kept all of it. If the owner wants the lower end, the candidates to move out
   are Layer 3's binary inspection (fatbin) and the compile-error table.
2. **Error-check style.** Listing 3 throws a `std::runtime_error` from `cudaCheck` and owns device
   memory with an RAII `DeviceBuffer<T>`, to show that exceptions and RAII carry over on the host.
   CU201 (F6-05) prints and exits instead. The chapter says both styles are valid; the Dean may
   want one house style.
3. **New analogy mappings (proposals for the registry, guide 8.1; not registered yet):**
   - execution-space specifier = a label on each recipe: "chef only", "hall only", or "both";
   - nvcc = a translator who splits the recipe book, translates the hall's cards, hands the
     chef's pages to the chef's usual translator and binds everything into one book (extends the
     F1/F2 "compiler = translator" mapping);
   - last error = a note pinned at the pass between kitchen and hall that nobody reads to you;
   - device pointer and host pointer = shelf numbers in two different pantries; the note does not
     say which pantry;
   - managed memory = a shared pantry whose shelves a porter moves between buildings on demand.
   Mappings used as registered: CPU = head chef, GPU = hall of helpers, kernel = recipe card,
   order slip handed to the hall = launch (as F6-01 already uses), several cooks in one kitchen
   = threads (SP203), RAII = borrowed tool returned automatically.
4. **Folder.** The guide's 7.1 puts bridges in `bridges/`; the build reads fragments from
   `chapters/<folder>/`, so this one lives in `chapters/BR-01/` as the task asked.
5. **Teaching models.** Listings 7 and 8 are the university's own CPU models (a launch as two
   loops with AddressSanitizer; an asynchronous launch as a queue run at synchronize). Both are
   labelled "not CUDA" in the code and the text.
6. **Hardware.** A GPU run of Listings 4, 5, 9 and break_host_pointer (and Compute Sanitizer on
   Listing 5 and on a no-bounds-check variant) is needed to close the untested-on-hardware notes.

## BR-02
Source: [`chapters/BR-02/NOTES.md`](chapters/BR-02/NOTES.md)

### Decisions for the owner

1. **Starter lab choice.** "Hipify a CU201 lab" was implemented with F6-04's SAXPY (milestone E1) extended by a
   warp-vote count of negative results, because no CU201 lab uses lane-level code where a warp-size assumption
   could be planted naturally. The vote itself is CU301 material (F6-15), listed as "recommended" in prereqs.
   Alternative: plant the assumption in a CU301 lab instead and make CU301 a hard prerequisite.
2. **Translator.** AMD's HIPIFY is not installed; the lab uses the university's own `toyhipify.py` (F7-04),
   labelled as such. When a real hipify-perl/hipify-clang is available, add its output beside R4 and R17.
3. **Analogy mappings used beyond the 8.1 registry** (proposal to register): "second kitchen building = AMD/ROCm
   platform", "shared cookbook language = HIP", "prep chef = hipcc" (as already used by F7-01 and the guide's
   chapter titles); "tally card with one box per seat = lane mask (ballot result)"; "row leader counting raised
   hands = elected leader lane doing the atomic add" (raising hands = warp vote is from F6-15).
4. **Length.** About 8,800 words including sources, tables' text and answers (prose alone is within the L3
   range, toward its upper end) because the bridge card asks for every carry-over, change and trap in depth with
   a run example each.
5. **Forensic production numbers** in the scenario are illustrative ("less than one in ten thousand"); the counts
   quoted in the key come from R14. The answer key is inline (per FRAGMENT_FORMAT), not in `_keys/`.

## BR-03
Source: [`chapters/BR-03/NOTES.md`](chapters/BR-03/NOTES.md)

### Decisions for the owner

1. **Length.** About 8,000 words of prose before Sources (the guide's L3 range is 4,000–7,000;
   other chapters are about 5,000). The bridge card asks for Carries over, Changes and Traps
   "each in depth" with a run per trap, plus two worlds of code; I kept the depth. If the
   Editor wants it shorter, the natural cut is the Layer 3 profiler paragraph and the
   Listing 5/7 tables (the models are explained in the prose).
2. **Analogy.** The chapter uses two registered worlds that are both kitchens: F2's restaurant
   kitchen (cooks, key to the spice cabinet, tally counter) for the CPU side and F6's great
   kitchen hall (helpers, rows, tables, far pantry) for the GPU side; the hook moves Chef
   Amara (F2-34) from one to the other. No new mapping was invented. Proposal for the
   registry: "spin lock in a warp = the helper with the key waits for her row; the row waits
   for the key" (used in the hook and the SIMT deadlock glossary entry).
3. **Course field.** Front matter `course: BR` (as instructed); the build places it under the
   bridges group.
4. **Profiler in the lab.** The card says "measure and explain ... with the profiler". With no
   GPU, the only profiler evidence is nvprof's real "GPU profiling skipped" output; the
   CPU side uses `getrusage` because `perf` does not work in this container. The GPU
   profiling step of the Lab is written as a task with metric names left to the learner's
   version of the documentation (unverified box 2).
5. **E4 acceptance test** is quoted verbatim in the Lab's Verification; the provided
   programs test only the scrambled and skewed inputs, so the learner must extend them
   before claiming E4.
6. **sm_60 target.** The spin-lock SASS is shown for sm_60 (pre-Volta structure) and sm_80;
   CUDA 12.0 still compiles sm_60 without a warning in this build. A later toolkit may drop it.
7. **Forensic scenario symptoms** ("never returns on the oldest GPU", "slow on the newer
   one") are story, not evidence; the answer key says so. Evidence is real compiler output
   and model runs only.

## BR-04
Source: [`chapters/BR-04/NOTES.md`](chapters/BR-04/NOTES.md)

### Decisions for the owner

1. **New analogy details.** These stay within the registered F8 mapping ("chain of kitchen halls with a
   delivery service"; "passing bowls around a round table"). They are: "corridors in one building vs roads across town" for
   scale-up/scale-out, and "soup vs salad at the same table" for a collective mismatch. **Please register them, or
   ask for a rewording.**
2. **Glossary.** Four new terms: Scale-up and scale-out (GPU communication), Link roofline, Message-size sweep,
   Collective mismatch. **"Scale-up/scale-out" is this university's usage, defined in the chapter. It is not quoted
   from a source.** All other terms are linked to the existing entries: Alpha–beta model, Topology, Topology (of a node), P2P,
   PCIe, NVLink/NVSwitch, xGMI, Stream/Event (CUDA), Roofline model, Ridge point, Half-performance size,
   Collective operation, All-reduce, Ring all-reduce, Ring order, busbw, Ping-pong, Communication-computation
   overlap, and Watchdog.
3. **Shared-memory and TCP-loopback MPI processes stand in for GPUs and for a network** in the lab's measured
   part. The chapter says clearly that their numbers say nothing about GPU links. The owner may prefer that this
   part be marked more prominently as a substitute.
4. **Overlap with F6-37 and F8-03.** The peer matrix and the predict-then-measure step also appear in those chapters.
   BR-04 keeps them as its lab, as the bridge card asks. They are used here to teach the three traps, and they link
   forward rather than repeat the deeper material.
5. **Prerequisites.** BR-04's front matter names CU303 chapters and F6-37 "recommended". DG401's card requires
   only CU302 and DS201, so a learner may arrive without CU303. The owner may want CU303 added to DG401's
   prerequisites, or BR-04's list relaxed.

## BR-05
Source: [`chapters/BR-05/NOTES.md`](chapters/BR-05/NOTES.md)

### Decisions for the owner

1. **uRTOS stands in for Zephyr/FreeRTOS** in the card's lab (neither is installed; no network).
   The bridge previews uRTOS before F3-39 builds it. Approve, or provide an RTOS SDK in the container.
2. **PREEMPT_RT half of the lab is untested**: the container's kernel cannot be changed. A machine
   with a PREEMPT_RT kernel (ideally the same hardware with and without) is needed to close it.
3. Chapter length is above the L3 guideline (about 4,000 words of prose): the card asks for each
   trap in depth with a real run, plus three worlds; trimming candidates are the code tables of
   Listings 3, 4 and 12.
4. New analogy uses within the registered F3 world (no new mapping registered): "three schools"
   (one-room school = bare metal, school with a hall rule = RTOS, campus with principal = Linux),
   doorbell = timer interrupt, "locking the hall door from the inside" = interrupt lock. Please
   confirm or register.
5. Forward links to RB304 (F9-45, F9-47, F9-48, F9-49, F9-51, F9-66) and DN302 (F10-20) for the
   Linux real-time depth, instead of repeating their labs here.

## BR-06
Source: [`chapters/BR-06/NOTES.md`](chapters/BR-06/NOTES.md)

### Decisions for the owner
- **Analogy registry proposal:** "noticeboard where notes may be read out of the order they were pinned, unless stamped" = weak memory ordering; "read-earlier-notes-first stamp" = release/acquire barrier. Used in the hook, Layer 1 and the jargon box, marked as proposed. "Where the analogy breaks" lists its limits.
- Reused F4-23's Ms Okafor (visiting three schools before opening her second one).
- **Big-endian stand-in:** the forensic uses qemu-s390x as the big-endian CPU (no big-endian Arm/RISC-V user-mode target in the container). F4-30 covers big-endian more broadly.
- **Overlap with F4-23:** BR-06 fills the checklist from QEMU's monitor + entry probes; F4-23 does it from a devicetree parser. BR-06 deliberately does not parse the DTB (only its magic) and links to F4-23 for the method.
- **Length:** prose is above the ~5,000-word L4 target once line tables and the answers are counted (~11,000 words of text in total including tables, sources and answers). Trim candidates: the Listing 16 (run.sh) table and some of the worked example.
- Litmus counts change every run, so the chapter describes patterns ("in some runs") and never quotes numbers; a rebuild will show different counts.
- x86 probe uses 32-bit Multiboot (QEMU -kernel), not UEFI; UEFI+ACPI are described from OS301/F3-24 and the documents, not re-run here.
- Row 8 (SMP start-up method) of the checklist is answered "from documents" because the memory map cannot show it.

## BR-07
Source: [`chapters/BR-07/NOTES.md`](chapters/BR-07/NOTES.md)

### Decisions for the owner

1. **Board stand-ins.** The card's lab says "on the board". With no board in the build, the lab
   uses QEMU `sifive_u` (a model of a real board, already D7's stand-in in F4-29) for the
   known-good boot and dump, QEMU `raspi3b` for trap 2 and QEMU Arm `virt` with its pflash boot
   flash for trap 3. Accept, or choose a reference board for the real-hardware version and
   record it in the dossier.
2. **Teaching board tree.** `teach_board.py` writes an *imaginary* board's DTB (vendor prefix
   `univ,`) because no QEMU machine's tree has resets, pin groups, supplies or power domains.
   It is labelled imaginary everywhere. Keep, or replace with a real board's tree once a source
   can be opened.
3. **Clock-gate model.** Trap 1 is a host model (our code), consistent with F4-35's `socsim`
   ("reads as zero"); no QEMU board models clock gating. Real behaviour stays in an unverified box.
4. **Length.** Prose is above the L4 lower target (the card asks for carries-over, changes and
   traps each in depth, plus a full lab, forensic lab and worked plan). Trim candidates if needed:
   the "universal method on a board" table and the extension steps.
5. **New glossary terms (6):** Known-good image; Dependency inventory (course term); Boot mode pins
   (boot source selection); Regulator (supply in a devicetree); Power domain; dma-coherent
   (property). Everything else links to existing DR402/DR403/HW/OS305 entries.
6. **Analogy.** Reuses F4-31's *proposed* mapping "devicetree = the building plan" (not yet in the
   registry) plus registered firmware = caretaker. New story elements (lights at the main panel =
   clock gate, architect's numbers = bus addresses, spare key = recovery path) are proposals for
   the F3/F4 registry: please accept or reject.
7. **Run cost.** The flash trap copies two 64 MiB firmware files into a scratch folder that
   `run.sh` deletes; nothing large stays in the repository.

## BR-08
Source: [`chapters/BR-08/NOTES.md`](chapters/BR-08/NOTES.md)

### Decisions for the owner

1. **Stand-in model.** No kit exists in the build; the chapter runs everything against
   `kit_model.hpp` and says so in every box, caption and log. When a course kit is chosen, Part B
   of the lab needs a real `LoggedMotorIo` implementation and real runs; every number in Layers
   2–3 should then be re-checked against a real paired run (the method stays).
2. **Sign-test duty.** The first kit attempt fails F9-22's sign test at duty 0.1 (the stand-in's
   friction); the chapter teaches the change to 0.2 as "re-tune test parameters too". F9-22
   itself is unchanged.
3. **Additions in `bench.hpp`** beyond F9-22's layers: an overspeed stop on the measured speed,
   a run record, the three logged readings (sample time, supply, reference), a `timestampSpeed`
   option and an `averageSamples` option. If F9-22 should own any of these, move them there.
4. **Cross-lab includes.** `bench.hpp` includes `../F9-21/pid.hpp` and `../F9-22/motor_io.hpp`
   (deliberately: "the same files"). A change in those labs changes BR-08's outputs; re-run the
   lab after any edit there.
5. **Reaction time 1.0 s** (trap 3) and the 5 % reference-check tolerance (trap 2) are exercise
   values, marked as such; a supervisor should set real ones.
6. `forensic_gen.cpp` lines 70 and 113 exceed 100 characters; the file is not shown as a listing.
7. F9-48 expects BR-08 to "set the rules for measurement evidence": covered in Layer 3
   ("Measurement discipline") and the run record; F9-48's author may want to link
   `#BR-08-l3-record`.

## BR-09
Source: [`chapters/BR-09/NOTES.md`](chapters/BR-09/NOTES.md)

### Decisions for the owner

1. **The university's licence for lab code.** `prod/` uses the placeholder SPDX id
   `LicenseRef-Uni-Lab` and `licences.allow` contains only that. The owner should choose
   the real licence(s); then replace the placeholder in prod/ (and consider SPDX lines in
   all lab code).
2. **Maintainer / response promise** in `prod/README.md` is left "to be filled in by the
   owner"; no on-call arrangement is invented.
3. **Source registry:** add Semantic Versioning 2.0.0 and the SPDX specification + License
   List to registry 4.9 (F12), or tell authors to drop them.
4. **Analogy mappings (F12, building a house as a team) proposed for registration:**
   hobby code = a shed you build for yourself; production = a house a family lives in;
   CI = the inspector's checklist run on every change; release notes/changelog = the
   handover letter; known-issues list = the snag list; on-call = the builder's phone
   number; monitoring = the damp meter; security update = a hinge recall; dependency
   inventory = which batch of hinges went into which house; licence = conditions written
   on borrowed drawings; exposure = nights under a leaking roof. Breaks are in the chapter.
5. **Length:** about 11,000 words outside code and tables (including jargon box, forensic
   lab, answers and sources), above the L4 aim of 5,000 for prose; the bridge card asks
   for ten "changes" and four traps in depth, each with a run. The editor may move the
   Layer 3 subsections into SE301/SE302 cross-references if the page must be shorter.
6. Figure 1 and the forensic scenario are fictional framings (Lan, Ines, Kofi, Sipho's
   team, a "pasted" `fast_median.cpp` written for the lab); every number in the evidence
   comes from the recorded runs.
7. Check-yourself question 8 and the lab extension leave the `E-INTERNAL` branch of
   `prod/app/main.cpp` untested on purpose (a teaching point); `monitor.cc` keeps
   `std::stod` on its threshold argument on purpose (question 9).

## BR-10
Source: [`chapters/BR-10/NOTES.md`](chapters/BR-10/NOTES.md)

### Decisions for the owner

1. **The card's lab ("a three-node virtual cluster under Slurm") is replaced** in this
   build by the university's own three-process virtual cluster (`cluster.hpp`) plus a Slurm
   batch script run by bash, because the container has no Slurm and no VMs. The real Slurm
   version is lab step 8, untested; it reuses F5-23's VM plan (also unverified). Approve, or
   schedule a Lab Engineer run on a host with Slurm.
2. **Clock skew is injected**, not observed (one host has one clock). Approve the honest
   labelling, or provide two machines for a real measurement.
3. **Fencing is a SIGKILL stand-in** for a BMC power-off; labelled as such in the log line.
4. Timing results come from a visibly noisy shared VM (R2's speedup 0.91 versus R3's 1.72,
   one minute apart). The chapter uses that noise as teaching material (Trap 1). If the
   owner prefers stable numbers for publication, rerun on a quiet dedicated machine and
   update the prose numbers listed above.
5. Glossary: "Straggler" lists F5-39 under chapters because that chapter uses the word
   without a glossary entry; the Integrator may want F5-39 to link it.
6. The chapter is long for a bridge (about 12,500 words including tables, answers and
   sources); the Editor may move the line-by-line table of `cluster.hpp` into the lab page.

## MP1
Source: [`chapters/MP1/NOTES.md`](chapters/MP1/NOTES.md)

### Decisions for the owner

1. **One kernel versus the three teaching kernels.** The earlier courses did not build one
   kernel: OS302 (higher-half x86-64), OS303 (a separate 32-bit teaching kernel for B9–B13),
   OS304 and DR402 (AArch64) are separate code bases. Milestone 1 therefore starts with a
   merge, which the handbook makes the first task and the main R1 risk. The starter lab
   reuses each kernel unchanged as matrix rows and adds a small new arch-neutral MP1
   kernel for both architectures; it does not do the merge.
2. **Flaky B9 fairness row.** The OS303 B9 test (max/min ratio <= 2.00) fails under TCG
   with host load, and also failed with `-smp 4 -cpu max`. It runs quarantined
   (`~os303-threads`, smp 1, qemu64). The owner should decide whether OS303's bound or
   its configuration changes; this handbook does not edit OS303.
3. **Boot path of the starter.** The x86-64 MP1 kernel boots by Multiboot through QEMU
   `-kernel` (F3-18's path), not by the UEFI loader the project needs at milestone 3.
4. **Milestone 4 acceptance.** The card says "remote shell or file service"; the handbook
   proposes concrete acceptance tests (built from C9 and B18) for the owner to approve.
5. **"One kernel image".** Interpreted as one source tree and one image per architecture,
   with every other difference in the platform difference record; the rubric descriptors
   are this handbook's proposal.
6. **Signers of R0–R4.** The guide does not name signers; the handbook proposes peers and
   a mentor where available, with the hardware owner signing every safety item.
7. **Style.** `run.sh` has some lines over 100 characters (long commands kept on one line
   for the log records); C++, Python and build_all.sh were wrapped.
8. **Length.** The chapter is about 14,000 words including tables, gates, rubric and
   project material; the prose part is inside the L5 range, the rest is project material.
9. **Flaky observations vary.** R6 is recorded as an observation, never a pass condition;
   the handbook does not quote its counts in prose, because they change from run to run.

## MP2
Source: [`chapters/MP2/NOTES.md`](chapters/MP2/NOTES.md)

### Decisions for the owner

1. **Template adapted to a project.** All 21 template headings are present in order, plus six
   project sections as extra `<h2>`s: Milestones, Review gates R0–R4, Deliverables, Grading
   rubric, Risks and safety (after Layer 3), and "Incident and forensic write-up template"
   (after the Forensic lab). The Mini-project section holds the "starter extensions due at R1".
   build.py accepted this (no PROBLEM lines). Please confirm this shape for MP1–MP8, so the
   eight handbooks look alike.
2. **Level.** The card gives none; L5 chosen (after L4 courses DS402 and DS403, L4–L5).
3. **Gate checklists, signers and rubric level descriptors** are this handbook's proposal; the
   card fixes only the gate positions and the weights (kept exactly: 35/20/15/15/15). The
   guide says the owner decides who signs; the handbook asks for independent peers, a mentor
   where available, and the owner's named supervisor for R3 safety items.
4. **Seed counts.** Milestone 1 asks for at least 1,000 generated seeds per nemesis kind
   (as DS302's course project did); the starter uses 30 to stay within the lab time limit.
5. **The forensic lab reveals its root cause in `mutants.out`.** The "heartbeat 200 ms" row
   of the planted-bug table (shown in the Code walk-through, before the forensic lab) is the
   same misconfiguration. Kept on purpose: it shows the suite would not have caught the
   incident. The owner may prefer to move that row to the answer key.
6. **Library copied again (fifth copy).** As DS302 decided, each lab folder builds alone, so
   the DS302 Raft library now exists in `labs/F5-18` … `F5-21` and `labs/MP2`. A fix must be
   applied to all copies, or run_lab.sh could allow a shared include directory.
7. **Milestone 4 fallback.** The handbook requires a written decision record approved at R2
   before the Linux-VM fallback is used, and the C9 acceptance tests (quoted from the
   curriculum) as the entry criterion for the own-OS path.
8. **Source registry additions** (same as DS302/DS402/DS403 notes): Ongaro's dissertation,
   the SRE Workbook (D4), the Borg paper (D5).
9. **MP1 link.** The handbook links `#MP1` (the card or, when written, the MP1 handbook) and
   assumes MP1 delivers a kernel with C9 networking and a block driver; check against the
   MP1 handbook when both exist.

## MP3
Source: [`chapters/MP3/NOTES.md`](chapters/MP3/NOTES.md)

### Decisions for the owner
- Gates R0 and R3 are proposed (card requires R1, R2, R4); R2 placed after M2. Hold or drop?
- Rubric level descriptors (four levels) are this handbook's proposal; weights are the card's.
- The 10 % "noisy" spread threshold in Listing 4 is a handbook choice.
- Register the analogy "specialist station of the hall = vendor library" (F6-35 proposal) in the
  analogy registry, or replace it.
- Synthetic records for the report/forensic labs: acceptable as teaching data?
- Real-GPU runs of mp3_gemm (with and without cuBLAS) are needed before the handbook's GPU
  harness can be called tested.
- In the no-GPU container cudaDriverGetVersion reported 13000 (runtime 12000); the source of
  that driver library was not investigated.
- "Run fingerprint" (MP2 glossary) and "Targets fingerprint" (MP3) are related but distinct; check
  wording consistency across mega projects.

## MP4
Source: [`chapters/MP4/NOTES.md`](chapters/MP4/NOTES.md)

### Dependencies on other folders (important for the owner)

- `mp4_check.hpp` includes `../MP3/mp3_tolerance.hpp` (MP3 Listing 1) unchanged.
- `shapes.hpp` copies MP3's `kShapes` from `../MP3/mp3_suite.cpp`; `mp3_sync.py` fails the lab
  if they drift (it takes the longest `kShapes` definition, because mp3_suite.cpp had an `#if`
  with a reduced set while MP3 was being written in parallel).
- Also reused: `../F6-21/cuda_shim.hpp`, `../F6-25/lowp.hpp`, `../F7-20/hazards.cpp`.
- MP3 was being written at the same time as MP4. If MP3 renames or moves these files,
  rerun this lab; the build will fail loudly, never silently.

### Decisions for the owner

1. **Rubric split.** The card takes 15 % "from code quality and methodology" without saying how.
   This handbook splits it in proportion to MP3's weights: methodology 20 -> 10 %, code quality
   10 -> 5 % (final: correctness 30, performance 25, methodology 10, gaps 15, portability 15,
   code quality 5). Alternatives: 7.5/7.5, or all 15 from methodology.
2. **Optional gates.** The card requires R1, R2, R4. The handbook describes R0 (may be merged
   into R1) and R3 ("measurement freeze") as optional; the owner decides whether to require them.
3. **Calendar.** "About 12–16 weeks part-time" and the week plan of Figure 2 are a planning
   suggestion, not data.
4. **No AMD GPU available.** Whether M2–M3 may be passed with "untested on AMD hardware" marks
   (and a changed rubric) is the owner's decision at R0/R1.
5. **Analogy proposal** (needs registration by the Dean): "each building's master team = the
   vendor library" (F7-20 already used "each hall's master team" in prose; CU401 proposed
   "library = a specialist station"). The two proposals should be merged into one mapping.
6. **Gate signers.** Proposed: two independent reviewers at R1/R2 (a peer who passed HP401 and,
   where available, a mentor), the owner approving targets; a panel of two at R4.
7. **Targets digest vs MP3 fingerprint.** MP3 freezes targets with an FNV-1a fingerprint of a
   text file (targets_lock.cpp); MP4's `report.py` uses a SHA-256 digest of JSON because it
   handles two GPUs. A later revision could make MP4 reuse MP3's targets format per GPU.

## MP5
Source: [`chapters/MP5/NOTES.md`](chapters/MP5/NOTES.md)

### Decisions for the owner

1. **R0 and R3.** The card lists only R1, R2 and R4. The handbook describes R0 and R3 as "not on the card; owner decides", and recommends R3 before fault injection on shared nodes. The owner also decides who signs when no human mentor is available.
2. **Rubric level descriptors.** The four-band descriptors are a proposal; please approve or edit them.
3. **Targets.** X₁, X₂, the model tolerance, the overlap target and the abort time are blank in the R1 table on purpose (curriculum 13.4). No default is suggested.
4. **Length.** About 9,400 words including tables.
5. **Forensic evidence pack.** The pack is a real run of a deliberately broken build (`Mutant::split_floor`). The guide's `_evidence/` folder and README convention was not created, because no such folder exists elsewhere in the repository yet. Its "how produced" part is inline in the answer key, as the other courses do.
6. **Next link.** MP8 is linked by its card id, because no MP8 handbook exists yet. MP3 and MP4 are linked as related projects.

## MP6
Source: [`chapters/MP6/NOTES.md`](chapters/MP6/NOTES.md)

### Cross-lab dependencies (deliberate reuse; owner please note)

- `labs/MP6/mp6_robot.hpp` includes `../F9-56/nav.hpp`, which pulls in
  `../F9-54/planner.hpp` and `../F9-52/house.hpp`.
- `mp6_ekf.hpp` includes `../F9-34/mat.hpp`.
- If those RB302/RB401 files change or move, rerun this lab: the outputs and the numbers
  quoted in the handbook would change. RB401 already uses the same pattern.

### Decisions for the owner

1. **No motor power on the real robot before R3.** This is stricter than the card, which
   only says "R3 before any real-robot run". M2 and M3 therefore run as HIL with the
   motor drivers unpowered until R3. Confirm the rule, or allow supervised bench work
   earlier with its own R3-style record (the handbook's Watch-out box says so).
2. **Grading without hardware.** The card does not say how "task success … on hardware"
   (25 %) is graded if the kit or a supervisor is unavailable. Choose one:
   - adopt MP7's rule, where the gate records why;
   - or require the hardware run.
3. **Rubric level descriptors.** The descriptors (4/3/2/1) and the score formula
   weight × score / 4 are my proposal. The weights are the card's, unchanged.
4. **Signers per gate.** These are proposals. The guide says the owner decides:
   - R0: mentor plus one peer;
   - R1 and R2: two outside peers plus the mentor;
   - R3: the supervisor of the real runs (mandatory, in person, for safety items), plus
     the mentor and one peer;
   - R4: a panel named by the owner.
5. **R0 for MP6.** The card lists R1–R4. R0 is added from guide 11.6 ("every mega project
   has … R0 proposal"). Confirm.
6. **28-week plan and the plan numbers.** Ten seeds, five paired runs, ten boots and the
   M1 thresholds T1–T4 / E1–E5 / W1–W4 are proposals for each team's R1.
7. **SS402 open goals G32 (hard-wired e-stop timed on the real robot) and G7.** The
   handbook makes M6/M5 close the hardware e-stop timing and puts it into R3. This answers
   SS402's decision 2 with "yes, at R3/M5".
8. **Story characters.**
   - Noor (new) appears in the story hook.
   - Ravi (also used in RB401) appears in the forensic scenario.
9. **New analogy proposal (F9 world).** Learning a courier route on the trainer, then in
   traffic with a coach who can shout "stop" = sim first, supervised real runs, the e-stop.
   This sits with RB403's trainer/road mapping of F9-71. Register or reject it.
10. **Simulator for the real M1.** The starter uses the university's own C++ house
    simulator. The faculty must choose and record the ROS 2 distribution and simulator
    (with versions) before teams start M1.
11. **Kit not chosen.** The kit, the robot computer, the microcontroller board, the
    battery and charger, and the e-stop components must be named in the dossier (part F),
    with their makers' documents, before R3 can be held.

## MP7
Source: [`chapters/MP7/NOTES.md`](chapters/MP7/NOTES.md)

### Decisions for the owner

1. **R0 for MP7.** The card lists gates R1, R2, R3, R4; the guide's general rule lists R0–R4.
   The handbook adds a short R0 (path, feature, hardware, supervisor, country). Keep, or merge
   R0 into R1.
2. **R3 placement.** The handbook holds R3 before *any* flight, including the tethered or netted
   hover (M4b), so M4 is split into M4a (props off, before R3) and M4b (after R3). Confirm.
3. **Rubric level descriptors** (4 levels per criterion) are this handbook's proposal; the
   weights are the card's. Proposal: level 1 in "safety process" fails the project. Confirm.
4. **Who signs.** Proposed: two independent peer reviewers at every gate, mentor where
   available, and the supervisor must sign R3. The guide leaves the choice to the owner.
5. **Supervision rules for real-hardware steps** are referenced, not defined: the owner must
   define who may supervise bench, tethered and outdoor tests (age, qualification).
6. **Worked feature.** The handbook works the "custom estimator check" example in detail; the
   mission-behaviour and companion-interface options are described more briefly.
7. **Planning weeks** in Figure 2 (16 weeks) are a proposal, not a rule.
8. **Analogy sub-mappings proposed** (F10, registered world "carrying a tray of drinks while
   walking"; DN401's cast Leila, Tomás, the head waiter reused): garden banquet = supervised
   outdoor test; rope handrail = tethered hover; Leila's own "stop when my senses disagree" rule
   = the learner's module; the head waiter's three conditions = R3.
9. **Real-tool run.** To move M1–M3 from "untested in this build" to "tested", schedule a build
   with the chosen stack's source and SITL installed, and a bench flight controller (AH-26).

## MP8
Source: [`chapters/MP8/NOTES.md`](chapters/MP8/NOTES.md)

### Unverified boxes / claims (need owner or Source Researcher)

1. Layer 2 unverified box: whether a GPU vendor runtime can run on an OS other than the vendor's
   supported ones (relevant to the card's example "computer runs the learner's OS image"). No
   vendor document was opened. The handbook suggests the RB403 Linux image as the realistic
   reading and leaves the decision to R0.
2. D3 (Shostack, "Threat Modeling") — title only, used only by reference to F11-14.
3. Contract C-grid v1 (values 0/100/255, integer cell_mm, max age) is the university's own
   teaching design; any resemblance to ROS 2 / Nav2 message or costmap conventions was not
   checked and is not claimed.

### Decisions for the owner

1. **Goal-area counting rule** (proposal): an area counts only when the learner brings a finished
   MP of that area whose acceptance tests pass from a clean checkout at R0. The card only says
   "at least two goal areas"; confirm or amend.
2. **Gate checklists, signatories and rubric level descriptors** are this handbook's proposal on
   top of the card's weights and SE501's gate rules (PASS / PASS WITH CONDITIONS / NOT YET; safety
   items never conditions; independent reviewers only). Proposed signatories: two independent
   peer reviewers (+ mentor where available) at R0–R3, one reviewer per goal area from R1, R3
   safety items signed in person by the supervisor the owner designates, R4 panel named by the
   owner. Confirm who may supervise real-robot/drone runs (supervision rules are the owner's).
3. **Pass without the real-hardware run**: the handbook applies the MP7 card's rule ("may pass
   without the outdoor flight if regulations or supervision do not allow it; the gate records
   why") to any MP8 real run. Confirm.
4. **Public defence format** (length, audience, recording) is left to the owner, as in F12-30.
5. **Mini-project section** holds gates, deliverables and rubric (the template has no dedicated
   section for them). Confirm this placement for all MP handbooks or name another.
