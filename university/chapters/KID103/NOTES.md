# KID103 — author and lab-engineer notes

Course: KID103 Make it move: circuits, LEDs and a simulated robot (L0–L1, 3 credits).
Chapters: F0-39 … F0-46 (all 21 sections plus "Answers to Check yourself" in each).
Build date: 2026-10-09. Toolchain for every listing: `g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0`,
`-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined`, run with
`university/labs/run_lab.sh university/labs/<ID>`. All 18 listings built cleanly and exited 0.

## Course-wide decisions and open items (for the owner / Dean)

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

## Per chapter

### F0-39 Electricity is like water in pipes (and where that breaks) — L0
- Listings run: `ohms_law.cpp` (exit 0).
- Unverified boxes: none (no hardware numbers needed; worked example uses labelled exercise numbers).
- Claims not verified (title-only): units and Ohm's law (D1, D2); conventional current vs
  electron drift; P = V × I; electrons already present in the wire (D1, D2).
- Forensic evidence is a hand-written broken calculation (stated in the key), not program output.

### F0-40 Circuits, LEDs and resistors in a simulator — L0
- Listings run: `circuit_sim.cpp` + `.in`, `meter.cpp` + `.in` (forensic "The LED stays dark"
  evidence; ideal model, pretend numbers). Exit 0.
- Unverified boxes: (1) all LED/battery numbers are pretend; real forward voltage, max
  current, battery voltage and anode leg come from the kit datasheets; (2) Part B kit build
  not performed.
- Claims to verify: diode/LED behaviour, non-ohmic curve, semiconductor junction emits light,
  forward voltage depends on material (D2); LED needs series resistor (D1).

### F0-41 Buttons and switches — L0
- Listings run: `switches_logic.cpp`, `button_led.cpp` + `.in`, `button_broken.cpp` + `.in`
  (forensic). Exit 0.
- Unverified boxes: Part B (button-controlled LED, the course practical exam) not performed.
- Claims to verify: contact bounce and debouncing (D2); series/parallel = AND/OR (D1, D4 Petzold).

### F0-42 A microcontroller blinks (supervised) — L0–L1
- Listings run: `blink_sim.cpp`, `blink_broken.cpp` (forensic). Exit 0. Pretend clock; no sleeping.
- **Untested on hardware (AH-26):** the real-board blink. Shown as an unverified box after the
  simulator output and again in Lab Part B. Needed: chosen kit, board docs and toolchain,
  supervised run by the Lab Engineer, result in the QA record.
- Real-board code is **not given**; only labelled pseudocode ("not buildable, not for any
  real board").
- Claims to verify: microcontroller contents, output pins, logic levels from datasheet,
  firmware, busy wait vs hardware timer, pin current limit (D2).

### F0-43 Sensors: light, distance, tilt — L0–L1
- Listings run: `sensor_read.cpp` + `.in`, `nightlight_flicker.cpp` + `.in` (forensic),
  `nightlight_hysteresis.cpp` + `.in` (answer-key verification). Exit 0.
- Unverified boxes: (1) which sensors, units, ranges, wiring are unknown until the kit is
  chosen; (2) Part B real sensor untested on hardware.
- Claims to verify: LDR in a divider, tilt switch (D1); echo ranging, noise, hysteresis,
  ADC (D2). No speed of sound or sensor range is stated anywhere.
- Safety line "do not look into a distance sensor's light source if it uses light; follow its
  datasheet's safety notes" — owner may want to strengthen once the sensor is known.

### F0-44 Motors and motor drivers — L0–L1
- Listings run: `driver_check.cpp`, `hbridge_sim.cpp` + `.in`, `hbridge_log.cpp` + `.in`
  (forensic: wrong switching order → short). Exit 0.
- Unverified boxes: (1) pretend pin/motor numbers; whether the kit driver protects against
  shoot-through is unknown; (2) Part B real motor untested on hardware.
- Lab says "7 of the 16 settings are short circuits" — arithmetic (4 + 4 − 1), not a run.
- Claims to verify: DC motor reversal, motor current rising at start/stall, inductive spikes,
  PWM, transistor switching time / shoot-through (D1, D2).

### F0-45 A simulated robot follows a line — L0–L1
- Listings run: `line_follow.cpp`, `line_follow_swapped.cpp` (forensic). Exit 0.
- Unverified boxes: none (simulator only; no physical robot in this chapter — RB101 has it).
- Claims to verify: control loop/feedback/error (Åström & Murray), differential steering
  (Lynch & Park), reflective line sensors (Horowitz & Hill).

### F0-46 Maker safety habits — L0
- Listings run: `safety_check.cpp` + `.in`, `safety_check_incident.cpp` + `.in` (forensic;
  same code, different answers). Exit 0.
- Unverified boxes: (1) no battery/charging/storage rules given — they come from the kit and
  battery makers' instructions; (2) Part B kit walk-through not performed.
- The forensic "adult's note" is a written story, labelled as such (not a real event).
- Safety wording is general and conservative; it should be checked by the owner and
  extended with the chosen kit's own safety instructions.

## Owner rulings applied

- **Kit not chosen** → ruling **D1**. `build/KIT.md` names the kids' items: BBC micro:bit V2 (board),
  Kitronik :MOVE Motor (robot: two DC motors, line-following and ultrasonic sensors, 4 × AA holder,
  power switch), Fluke 17B+ multimeter (adult-held); kids' kits use AA cells, never LiPo. Each
  chapter F0-40…F0-46 now has a source entry **K1** (reference kit, linked to `#kit-kids`), its
  kit source (D3/D2/D4) says which items K1 names, and Part B boxes refer to them. Still open (not
  in KIT.md): the LED, resistor, battery holder and separate switches for F0-40/F0-41, and a light
  sensor for F0-43. The datasheets of the named items were not opened in this pass.
- **"Simulator" = our own C++ text programs** → ruling **A2**. Approved. A Note at the start of
  each Code walk-through (F0-40…F0-45) says the program is the course's own and names the real item
  it stands for (K1 items; Gazebo for later robot courses, ruling D2).
- **Pretend LED/battery/pin/motor numbers and thresholds** → ruling **A4**. Approved as exercise
  values; they were already labelled "exercise numbers" / "pretend" in text and code; kept.
- **Books title-only** → rulings **C1/C2**. Platt (3rd ed.) and Horowitz & Hill (3rd ed.) could be
  opened only as tables of contents; they stay as further reading (or are withdrawn where the
  contents show no matching section) and no claim rests on them. Claims were re-tagged to opened
  sources approved under C2 (OpenStax University Physics 2 and 3, College Physics 2e, Petzold
  Chapter Six sample, Feedback Systems 2e v3.1.5, Modern Robotics companion pages, example vendor
  datasheets, Arduino reference/examples, WHO checklist page, a LiPo maker's instructions).
- **Physical (kit) steps not performed** → ruling **C3**. Every Part B box is marked "Untested on
  hardware (owner ruling C3)" and now states what would be needed to test it. Status per **C4**:
  internally checked · hardware steps untested.
- **Real-board code not given in F0-42** → rulings **C3/A2**: stays labelled pseudocode until the
  micro:bit V2 documentation and toolchain are recorded and a supervised run is made.
- **New analogy mappings** (LED drain, water saver, lever valve, four valves, pressure cooker, water
  main, burst pipe, ribbon) → ruling **A3**: approved; no change.
- **F0-43 distance-sensor safety line** → decided by verifier (in the spirit of C3/D1): strengthened
  with the laser-safety warning of an example time-of-flight datasheet (never through a lens or
  magnifier; follow the datasheet's laser-safety notes).
- **F0-46 safety wording** → rulings **D1/C3**: the AA-only rule of the reference kit is stated;
  the kit and battery makers' own instructions were not opened, so their unverified box stays.
- **Soft prerequisite KID102 F0-29** → decided by verifier: not added to the chapters' `prereqs`
  (course prerequisites are the Dean's card); each chapter keeps "a teacher or parent may run the
  build command".
- **Accidental re-run of other courses' labs** → decided by verifier: no action in this unit (other
  units own their folders).
- **A8 (test keys), A10 (licence), D4 (e-stop)**: not applicable — the unit's labs have no keys,
  no `LicenseRef-Uni-Lab` placeholder and no e-stop code.

## Verification pass

Date 2026-10-10. Opened (WebFetch, URLs recorded in each dossier): OpenStax University Physics
Volume 2 (§9.1, 9.4, 9.5, 10.1–10.4, 14.2), Volume 3 (§9.7), College Physics 2e (§17.7, 20.1,
20.6, 22.8, 23.6); Petzold "Code" 2nd ed. (store page and Chapter Six sample); Platt "Make:
Electronics" 3rd ed. and Horowitz & Hill "The Art of Electronics" 3rd ed. (tables of contents
only); Åström & Murray "Feedback Systems" 2e v3.1.5 (Chapter 1 and wiki); Lynch & Park "Modern
Robotics" (title page and Chapter 13 companion pages); datasheets used as examples only — Vishay
5 mm LED and TCRT5000, Analog Devices MAX6816, TI MSP430G2x53, TI DRV8833, TI SBOA313A, ST
VL53L0X, Advanced Photonix PDV-P8001, Conrad tilt switch; Arduino reference (delay, pin modes,
analogRead, analogWrite) and examples (Blink, Debounce, StateChangeDetection, Smoothing); WHO safe
surgery page; Stefansliposhop LiPo safety instructions.

Results: dossiers `_dossiers/F0-39…F0-46.dossier.html` and QA records `qa/F0-39…F0-46.json`
written; 178 tagged claims checked; 26 corrected or rewritten (main ones: F0-42 "special memory
location" and "non-volatile" wording; F0-44 "inertia smooths PWM" and "transistor thresholds";
F0-40 "light at the joint" and multimeter "several ranges"; F0-39 capacitor sentence; F0-46
"pilots" checklist claim; F0-45 "many times per second"); every D1/D2 book tag re-tagged.
Left unverified (in boxes): LED leg names (F0-40), inside of push buttons and toggle switches
(F0-41), digital-output sensor chips and all kit sensor details (F0-43), kit pin/motor/driver
numbers (F0-44), kit and battery makers' rules (F0-46), plus every Part B hardware step
(untested on hardware, C3). Diagrams: F0-45 description corrected (rows/columns), F0-41 resistor
label added. Glossary sources updated to the opened documents. Labs: all 18 listings re-run with
`run_lab.sh` — exit 0, outputs identical; the recorded logs were restored (A5). WebFetch's shared
budget ran out several times; a Vishay LED datasheet direct link and the Energizer battery SDS
could not be opened (a distributor copy of the LED datasheet was used; no battery SDS is cited).
