# DN202 — Flight-controller hardware, sensors and ESCs: author notes

This course has six chapters, F10-07 to F10-12. It is level L3 and worth 3 credits. The prerequisites are HW302, DN201 and OS305.

Each lab is in `university/labs/<ID>`. Each one passes `university/labs/run_lab.sh university/labs/<ID>` with exit code 0, and all six were re-run at the end of this build, on 2026-10-10.

The numbers in the prose, figures, worked examples and answer keys were checked against the `.out` files. Numbers that no listing prints were checked with scratch programs or Python and were not added to the labs: the Q6 variants, the hand calculations in the worked examples, and the gap-by-turn split in the F10-12 key.

## Toolchain and local evidence

- **Compiler.** g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0. `run_lab.sh` builds every `.cpp` with `-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined`.
- **Python cross-check.** F10-09 also runs `spectrum_check.py` through `run.sh`, using Python 3.13.16 and numpy 2.5.3.
- **No hardware.** No flight controller, sensor, ESC, motor, power module, battery, charger, radio or ground station was used. Every hardware-dependent step is marked "untested on hardware".
- **No flight-stack names as facts.** No PX4 or ArduPilot parameter names, MAVLink message names, log message or field names, or menu names appear as facts. Wherever the reader needs one, an unverified box sends them to the documentation (by title).
- **Invented teaching items.** Where a real protocol or board would have needed details from memory, the chapters use invented items that are clearly labelled as such:
  - U-FC1 (a pretend board, F10-07)
  - U-Shot (a teaching digital ESC protocol, F10-10)
  - U-RC (a teaching RC frame, F10-12)
  - pretend mounts, packs and modules with exercise values
- **Real hardware numbers.** No real-hardware number is stated as fact, and no real LiPo voltage limit is given anywhere. F10-11 says explicitly that limits come only from the maker's documents.

## Listings run

**Nothing ran on hardware, and nothing is an expected failure.** Every run exits with code 0. Forensic generators write their evidence files and also exit with 0.

| Chapter | Run | Status |
|---|---|---|
| F10-07 | fc_pipeline | pass (Listing 1: estimate → rate loop → mixer → pulses 1464/1524/1533/1479 µs) |
| F10-07 | sched_sim (+ fcsim.hpp) | pass (Listing 2: design A 100 rate-loop misses, design B 0; load 75.0 %) |
| F10-07 | wiring_check | pass (Listing 3: 3 problems found on pretend board U-FC1) |
| F10-07 | jitter_log | forensic evidence, exit 0 (cycles skipped by a locked logger) |
| F10-08 | baro_alt | pass (Listing 1: 0.0843 m per Pa; temperature and weather effects) |
| F10-08 | mag_cal | pass (Listing 2: sphere fit offset +12.03 −7.00 +3.98) |
| F10-08 | gnss_local | pass (Listing 3: local metres; 5.49 m jump flagged) |
| F10-08 | bench_log | forensic generator, exit 0 (writes compass_log.csv) |
| F10-08 | compass_fit | forensic analysis, exit 0 (internal −1.102 °/A, external −0.054 °/A) |
| F10-09 | isolator | pass (Listing 1: RK4 vs transmissibility formula) |
| F10-09 | clipping | pass (Listing 2: mean bias from clipping) |
| F10-09 | flight_log | forensic generator, exit 0 (flightA.csv, flightB.csv) |
| F10-09 | vib_report | pass (Listing 3: vibration levels, clipping and spectra for both flights) |
| F10-09 | spectrum_check (run.sh, Python) | pass (agrees with vib_report) |
| F10-10 | pwm_esc | pass (Listing 1: end points, frame rates, resolution) |
| F10-10 | ushot | pass (Listing 2: U-Shot frames, timing, check value; 16/16 single-bit errors caught) |
| F10-10 | motor_test | forensic evidence, exit 0 (motor 3's ESC calibrated 1100–1900 µs) |
| F10-11 | power_module | pass (Listing 1: counts, calibration 20.015 A/V, mAh 2083/1482/2083) |
| F10-11 | battery_monitor | pass (Listing 2: warnings at 100 s raw, 564 s corrected, 638 s charge) |
| F10-11 | flight_power_log | forensic evidence, exit 0 (current scale 14 instead of 20 A/V) |
| F10-12 | rc_frames | pass (Listing 1: U-RC frame, CRC-8 rejects a flipped bit) |
| F10-12 | failsafe | pass (Listing 2: failsafe at 1281 ms / 1000 ms / never) |
| F10-12 | link_budget | pass (Listing 3: free-space range 24.6 / 11.6 / 4.4 km) |
| F10-12 | link_log | forensic generator, exit 0 (writes rc_link.csv, 5511 of 6000 frames) |
| F10-12 | rc_gaps | forensic analysis, exit 0 (losses at headings 150–240°) |

**Untested on hardware:** Part B of every lab, the bench parts of the course project, and exam P. None of these could be run in this build.

## Unverified boxes, by chapter

- **F10-07**
  - What each Pixhawk standard (DS-009 to DS-020) specifies. Only the numbers and subjects come from the guide's registry; no standard was opened.
  - The course board's microcontroller, IMUs, buses, ports and pinouts.
  - Part B: ground-station screens and how PX4 and ArduPilot schedule tasks and expose timing.
- **F10-08**
  - The physical constants in Listing 1, which were typed from memory and need checking against the US Standard Atmosphere and CODATA.
  - Sensor datasheet figures and GNSS receiver behaviour.
  - Part B: calibration screens, orientation settings and the declination handling of the flight stacks.
- **F10-09**
  - The vibration metrics and clipping counters that PX4 and ArduPilot log, with their names and thresholds.
  - The IMU's ranges and internal filtering, and the properties of dampers.
  - Part B.
  - The forensic lab uses synthetic CSV logs instead of the PX4 ULog or ArduPilot DataFlash log that the course card asks for.
- **F10-10**
  - U-Shot compared with real DShot (frame rates, bit timing, check value, bidirectional telemetry).
  - The protocols and calibration of the course ESCs.
  - Part B: motor-test screens and settings.
- **F10-11**
  - The pretend pack's open-circuit voltage table, capacity and resistance, and the 3.65 V warning level. These are exercise values and are not limits.
  - The power module's sensor, scales, limits and pinout, and how the power port maps to DS-009.
  - Part B: setting names, calibration procedures and battery failsafe behaviour.
- **F10-12**
  - U-RC compared with real receiver protocols.
  - Frame period, timeout, radio powers and frequencies (exercise values), and which bands and powers are legal for the learner.
  - Receiver failsafe modes, the range-check mode and RSSI scales.
  - Part B.

All named documents are cited "Title only — not opened during this build", with dossier gate G1 left open. The only exceptions are the University Authoring Guide and the Systems Curriculum, which were opened.

## Sources proposed for the registry (tier 3, not yet in the guide's registry)

- Singiresu S. Rao, "Mechanical Vibrations" (F10-09)
- Pratap Misra, Per Enge, "Global Positioning System: Signals, Measurements, and Performance" (F10-08)
- "U.S. Standard Atmosphere, 1976" and the CODATA recommended values (F10-08)
- Gene H. Golub, Charles F. Van Loan, "Matrix Computations" (F10-08, least squares)
- Jane W. S. Liu, "Real-Time Systems"; Giorgio Buttazzo, "Hard Real-Time Computing Systems" (F10-07). These may already be registered for OS305; please confirm.
- Alan V. Oppenheim, Ronald W. Schafer, "Discrete-Time Signal Processing" (F10-09)
- Austin Hughes, Bill Drury, "Electric Motors and Drives" (F10-10)
- Thomas B. Reddy (ed.), "Linden's Handbook of Batteries" (F10-11). HW302 also proposes this one.
- Paul Horowitz, Winfield Hill, "The Art of Electronics" (F10-11)
- Constantine A. Balanis, "Antenna Theory: Analysis and Design" (F10-12, Friis equation and free-space path loss)
- Ross N. Williams, "A Painless Guide to CRC Error Detection Algorithms" (F10-12)
- One kit-level source per chapter, still to be chosen: the board, sensor, ESC, power-module, battery, charger and radio documentation (dossier part F). F10-12 also cites the national radio-spectrum regulator's rules.

## Analogy mappings

The F10 world is carrying a tray of drinks while walking.

**Registered mappings used:**
- ESC = the nerve controller for each muscle (F10-10)
- Attitude estimation = keeping the tray level (F10-07, F10-08)
- Failsafe = "if you get lost, go back to the meeting point" (F10-12)
- MAVLink = walkie-talkie code words between drone and ground (F10-12)

**Proposed new mappings**, which need the owner's approval:

| Concept | Mapping | Chapter |
|---|---|---|
| Flight controller | the waiter's brain with its senses and nerves | F10-07 |
| Barometer | ears popping in a lift | F10-08 |
| GNSS | friends calling out from tables at known places | F10-08 |
| Checksum | repeating the last digit of a phone number back | F10-10 |
| Hard-iron offset | a lamp always on one side, mistaken for the sunset | F10-08 |
| Soft mount | a folded cloth under the glasses | F10-09 |
| Clipping | a glass that jumps off the tray, which the cloth cannot fix | F10-09 |
| Power module | the marks on a water bottle plus a count of glasses drunk | F10-11 |
| Voltage sag | feeling more tired right after a sprint than you really are | F10-11 |
| RC link | the head waiter's hand signals | F10-12 |
| Telemetry | the waiter calling back "two left on the tray" | F10-12 |
| Link budget | how loudly you must call to be heard across the hall | F10-12 |

Each chapter has its own waiter: Kwame, Lina, Mateo, Yusuf, Priya and Aiko. DN201 uses one character, Amara, for the whole course. The owner may want one character for the whole F10 track.

## Decisions for the owner

1. **Course kit.** Choose the flight controller, sensors, ESCs, motors, power module, LiPo packs, charger, RC system and telemetry radios. Every Part B and the course project depend on this choice. Until then, D-sources marked "Not yet chosen" stay open.
2. **Forensic log for F10-09.** The course card asks for a real PX4 ULog or ArduPilot DataFlash log. None was available, so the chapter uses synthetic CSV logs and has an unverified box saying so. Please supply a real log, with a licence to redistribute it, and the tool version used to read it.
3. **Teaching protocols.** U-Shot (F10-10) and U-RC (F10-12) stand in for DShot and real receiver protocols. Decide whether to keep them or, once the sources are opened, add a verified section on the real formats.
4. **Physical constants in F10-08.** Confirm them against the cited references.
5. **Word count.** Each chapter is about 5,500–6,000 words including tables, code tables, answers and keys. The prose alone is close to the L3 target of about 4,000 words.
6. **Cross-course links.** These chapters link to DN201 (F10-01, F10-02, F10-06), OS305 (F3-37, F3-39, F3-42), HW204 (F1-42, F1-44, F1-47, F1-48), HW302 (F1-64, F1-65, F1-69, F1-70, F1-71) and RB201 (F9-15).
   - They mention later chapters (F10-30, F10-35, F11-16) and courses (DN301, DN303, DN401) as plain text, not links, because those chapters do not exist yet.
   - The glossary links to the existing terms Damping ratio (MA301), Decibel (dB) for gain (MA301), and the HW302 battery, IMU, ESC and PWM terms. These are not redefined.
7. **Course project and exam P.** F10-12 holds the course project with a weighted rubric that combines the six dossier parts, and it mentions exam P. The owner should check the weights.
