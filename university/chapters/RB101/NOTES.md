# RB101 Robots for kids: sense, think, act — author notes (batch 1)

Author / Lab Engineer run for chapters F9-01 … F9-07 (faculty F9 ids, Foundation year).
Built in a sandbox without internet access, following `university/build/AUTHOR_BRIEF.md`
and `university/chapters/FRAGMENT_FORMAT.md`. Nothing here has passed the independent
Fact-Checker (G6) yet; the dossier gate G1 is open for every book source.

## Toolchain and runs (all chapters)

- Toolchain line (from every `.log`): `g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0`
- Command form: `g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined <file>.cpp -o <file>`
  via `university/labs/run_lab.sh university/labs/F9-XX`.
- All 16 listings built cleanly and exited with code 0 (no `.expect-fail`, no `.timeout`).
  All seven folders were re-run together at the end; every program is deterministic
  (fixed "noise" lists, no random numbers, no clocks), so re-runs print identical output.

| Chapter | Listing (role) | Input | Exit |
|---|---|---|---|
| F9-01 | `sense_think_act.cpp` (Listing 1) | — | 0 |
| F9-01 | `sensed_once.cpp` (forensic evidence: sensing moved outside the loop) | — | 0 |
| F9-02 | `light_sensor.cpp` (Listing 1, threshold 50) | — | 0 |
| F9-02 | `bad_threshold.cpp` (forensic: threshold 90) | — | 0 |
| F9-03 | `two_wheels.cpp` (Listing 1) | — | 0 |
| F9-03 | `circles.cpp` (forensic: simulated weak left motor) | — | 0 |
| F9-04 | `robot_loop.cpp` (Listing 1) | — | 0 |
| F9-04 | `circling.cpp` (forensic: wall-following rule never reaches the flag; step limit 24) | — | 0 |
| F9-05 | `steer.cpp` (Listing 1, gain 0.5 — the "fixed" run) | — | 0 |
| F9-05 | `zigzag.cpp` (forensic evidence, gain 1.8 — over-correction) | — | 0 |
| F9-05 | `gain_table.cpp` (Listing 2, gains 0.2 … 2.2) | — | 0 |
| F9-05 | `late_sensor.cpp` (Listing 3, one-step-late reading) | — | 0 |
| F9-06 | `grid_sim.cpp` + `grid_world.hpp` (Listings 1 and 2) | `grid_sim.in` | 0 |
| F9-06 | `mixup.cpp` + `grid_world.hpp` (forensic: one L typed as R) | — | 0 |
| F9-07 | `safe_drive.cpp` (Listing 1: speed limit + latched stop) | — | 0 |
| F9-07 | `late_stop.cpp` (forensic: stop checked every 4th step) | — | 0 |

Forensic pairs were made by copying the healthy listing and changing only what the answer
key says (checked with `diff`: `zigzag.cpp` differs from `steer.cpp` only in line 1 (comment)
and line 9 (gain 0.5 → 1.8); `bad_threshold.cpp` differs from `light_sensor.cpp` only in line 1
and line 12 (threshold 50 → 90)).

Scratch checks (not shipped, not in `labs/`): the F9-02 lab's "grey carpet" expected
observation (readings 55, 52, 52, 49, 20, 20, 21, 56, 48, 53; threshold 50 wrong on squares 3
and 8; threshold 34 correct) was confirmed by compiling and running an edited copy of
`light_sensor.cpp`. The F9-06 check-yourself answer `FFRFFRFF` (ends at row 3, column 1) was
confirmed the same way. Other lab "expected observations" (F9-01, F9-03, F9-04, F9-07) are
plain arithmetic worked out in the chapter; the Lab Engineer of G5 should run each learner
edit once and record it.

## Sources used and their status

Book sources are cited **title only**, each marked in the chapter "not opened during this
build; the Source Researcher must confirm the edition and section (dossier gate G1 open)":

- D1 Åström, Murray, "Feedback Systems" — feedback, closed/open loop, setpoint, error,
  proportional control, delays and oscillation, fixed-rate loops.
- D2 Lynch, Park, "Modern Robotics" — robot = sensors + computer + actuators; differential
  drive; encoders/odometry; wheel slip; heading; simulation; line followers as mobile robots.
- D3 Thrun, Burgard, Fox, "Probabilistic Robotics" — measurement = true value + noise; range
  sensors measuring echoes; calibration/bias; grid (cell) maps.
- D4 Platt, "Make: Electronics" — light-sensitive parts; switches as contact sensors; how a
  motor turns current into rotation; gearbox; motor driver; switch cutting motor power;
  batteries running down.
- D5 Petzold, "Code" — computers follow instructions; inputs/outputs.
- D6 Stroustrup, "A Tour of C++" — while, bool, !, &&, == (behaviour also shown by runs).
- D7 Liu, "Real-Time Systems" — fixed-rate loops (pointer to RB304).
- S1 University Authoring Guide (this repo), "Honesty first", 5.5 and 5.14 — opened and read.
- R-sources: the lab runs above (toolchain line from the `.log`).

## Per chapter: claims to verify, boxes, decisions

### F9-01 What makes a robot (L0)
- Conceptual claims to confirm in D2/D1/D4: robot = sensors/computer/actuators in a loop;
  controllers with loops inside loops; a sensor changes a voltage that a microcontroller
  reads; a control pin cannot power a motor directly, so a motor driver switches it.
- "Most people would not call a timer toaster a robot" is framed as the course's own
  criterion, not a fact. The cleaning-robot and automatic-door examples are written as
  "imagine…" (no product claims).
- Unverified boxes: none. Owner decisions: none.

### F9-02 Senses: sensors (L0)
- To confirm: light sensor pointed at the floor "often next to a small lamp" measures reflected
  light (D4); "some distance sensors send a pulse of sound or light and measure the echo"
  (D3); bump sensor = switch (D4); noise model "true value + random error", bias, calibration (D3).
- All sensor numbers are simulator units; no real-part numbers (AH-21).
- Unverified boxes: none. Owner decisions: none.

### F9-03 Muscles: motors (L0)
- To confirm: motor converts electrical energy to rotation; motor driver; gearbox trades speed
  for force; battery running down reduces turning for the same command (D4); differential
  drive turns toward the slower wheel; real encoders give many counts per turn; wheel slip (D2).
- The simulator does not model the turn geometry (stated in Layer 2); `drive(5, -5)` prints
  "curves to the right", which the lab uses deliberately as a limitation to discuss.
- Unverified boxes: none. Owner decisions: none.

### F9-04 Brains: programs and loops (L0–L1)
- To confirm (D1/D7): real controllers run at a fixed rate; late loops behave badly. The
  "watchdog" is mentioned only as something later courses meet (no claim about any product).
- Unverified boxes: none. Owner decisions: none.

### F9-05 Steering back to the line (L1) — the course's control-loop chapter
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

### F9-06 A robot in the simulator (L1)
- Course P-exam task ("program the simulated robot through a course") is the lab here.
- `grid_world.hpp` is shown with `data-src` (a header, not a `.cpp`) and `grid_sim.in` is also
  shown with `data-src`: **the builder must accept non-.cpp files in `data-src`**.
- The maze has exactly one corridor route; the lab asks learners to argue that.
- To confirm: grid/cell maps (D3); heading, plans plus feedback, drift, simulation (D2);
  open loop vs feedback (D1).
- Layer 3 mentions "professional robot simulators" generically and says later courses name
  one in their dossiers — no simulator is named anywhere.
- Unverified boxes: none. Owner decisions: none.

### F9-07 Robot safety (L0–L1)
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

## Untested on hardware
- F9-07 Part B (optional supervised kit robot) — every step. Nothing else in RB101 needs
  hardware; all labs have a simulator (and, for L0 chapters, a paper) version.

## Glossary (`glossary.json`)
- 38 four-part entries, one per Jargon-box term. Source field points to the chapter's
  D/R/S tag "(pending verification)". Terms likely to collide with other courses and needing
  the Integrator's merge (guide 10.3): Program, Loop, Condition, Sensor, Simulator, Log (trace),
  Feedback, Control loop, Gain, Encoder. "Error (control)" is qualified to keep it apart from
  program errors/bugs (F0-08).
- Glossary links in the chapters use the anchors that `build.py` actually generates
  (`gl-` + lower-case term, runs of non-alphanumerics → `-`, leading/trailing `-` stripped),
  e.g. `#gl-error-control`, `#gl-safety-limit-step-limit`, `#gl-target-setpoint`. Note:
  FRAGMENT_FORMAT.md's example `#gl-kernel-gpu-` keeps a trailing dash, but the builder strips
  it; the Dean/Integrator should align the format note with the builder.

## Cross-links assumed to exist in the integrated page
`#KID101`, `#KID102`, `#RB101`, `#RB202`, `#MA202`, `#safety`, `#F0-01`, `#F0-06`, `#F0-07`,
`#F0-08`, `#F0-10`, `#F0-33`, `#F0-34`, `#F0-36`, `#F9-09`, `#F9-16`, `#F9-17`, `#F9-18`,
`#F9-22` (ids from guide section 5). RB201, RB302, RB303, RB304, RB401 and F11 are mentioned
in text only.

## Things I was unsure about
- Reading level: L0 chapters average about 11–13 words per sentence (rough count by script);
  some table cells and forensic keys have longer sentences. Editor (G7) should check.
- F9-04 and F9-07 are tagged L0–L1 and F9-05/F9-06 L1 to keep the one-level step rule.
- Learners only have KID101 as a prerequisite, so code is read and changed, not written from
  scratch; every listing has a line table (Listing 2 of F9-06, the simulator, is explained in
  grouped line ranges as "you do not need every detail yet").
- SVG figures use `<polygon class="sv-head">` arrowheads, only `sv-*` classes, and one "✓"
  character in F9-07 Figure 1.
