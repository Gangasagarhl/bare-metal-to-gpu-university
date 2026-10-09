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
