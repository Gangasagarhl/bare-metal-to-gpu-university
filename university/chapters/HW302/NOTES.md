# HW302 — Sensors, motors and power electronics: author notes

Chapters F1-64 to F1-72, L2–L3, 3 credits. Prerequisites: HW101, HW204 (may run in parallel), MA202.
Labs are in `university/labs/F1-64` to `university/labs/F1-72`.
Each lab passes `university/labs/run_lab.sh university/labs/<ID>` with exit code 0. All nine were re-run at the end of this build, on 2026-10-09. After that re-run, the numbers quoted in the prose, figures and answer keys were checked against the `.out` files. Two quiz answers were checked by running a modified copy of a listing in scratch space: F1-70 Q5 (offDelay 8) and F1-72 Q5 (a = 0.2).

## Toolchain and local evidence

- **g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0.** Every `.cpp` is built with `-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined`.
- **Python 3 with numpy 2.5.3.** Used only in F1-65 (`spectrum_check.py`, run by `run.sh` with `python3 -I`) to cross-check the C++ DFT.
- **No hardware.** There were no sensors, motors, drivers, batteries, microcontrollers, logic analysers or oscilloscopes in this build. Every number about a device comes from the university's own C++ models and is labelled as a pretend exercise value.

## Listings run

**Nothing was run on real hardware.** Every hardware step in the labs is marked "untested on hardware in this build". No listing is an expected failure: every run exits with code 0.

| Chapter | Run | Status |
|---|---|---|
| F1-64 | adc | pass |
| F1-64 | aliasing | pass |
| F1-64 | forensic | forensic evidence, exit 0 ("The 2 Hz ghost": 48 Hz pickup aliased by 50 Hz sampling) |
| F1-65 | imu_driver | pass (pretend U-IMU6 over a modelled I2C bus; wrong address gives NACK) |
| F1-65 | gyro_drift | pass |
| F1-65 | jumpy_log | forensic evidence, exit 0; writes `jumpy_imu.csv` (synthetic, the course forensic "The jumpy IMU") |
| F1-65 | spectrum | pass (direct DFT in C++: 87.0 Hz at 0.2995 g, 174 Hz at 0.0795 g) |
| F1-65 | run.sh → spectrum_check | pass (numpy rfft cross-check gives the same peaks) |
| F1-66 | quadrature (with `quadrature.in`) | pass |
| F1-66 | speed | pass |
| F1-66 | polling | forensic evidence, exit 0 (missed quadrature edges at high speed) |
| F1-67 | tof | pass |
| F1-67 | scan | pass |
| F1-67 | stereo | pass |
| F1-67 | phantom | forensic evidence, exit 0 (invalid returns reported as 0 m) |
| F1-68 | pinhole | pass |
| F1-68 | exposure | pass |
| F1-68 | bayer | pass |
| F1-68 | rolling | forensic evidence, exit 0 ("The leaning pole") |
| F1-69 | dcmotor | pass |
| F1-69 | servo | pass |
| F1-69 | bldc | pass |
| F1-69 | stall | forensic evidence, exit 0 ("The motor that hums but will not turn") |
| F1-70 | hbridge | pass |
| F1-70 | pwm | pass |
| F1-70 | flyback | pass (the 10,000 V in case B comes from the model's chosen 10 kΩ; the chapter says so) |
| F1-70 | deadtime | forensic evidence, exit 0 ("The bridge that gets hot at standstill") |
| F1-71 | pack | pass |
| F1-71 | sag | pass |
| F1-71 | brownout (with `power_model.h`) | forensic evidence, exit 0 ("The robot that reboots when it drives off") |
| F1-71 | brownout_fix | pass (verification runs for the key) |
| F1-72 | averaging | pass |
| F1-72 | lowpass | pass |
| F1-72 | ground | forensic evidence, exit 0 ("The sensor that reads high when the motors run") |
| F1-72 | ground_check | pass (least-squares coupling: 0.0500 V/A, r = 0.999; star ground 0.0000, r = −0.023) |

`university/labs/F1-65/jumpy_imu.csv` (about 24 KB, 1000 rows) is kept in the lab folder as the forensic evidence file. It is regenerated identically on every run, because the noise generator is deterministic.

## Unverified boxes and claims that could not be verified, per chapter

All book sources are cited title-only ("not opened during this build; dossier gate G1 open"). No device datasheet exists yet, because no device is chosen. Each chapter's "Not verified" boxes are listed below.

- **F1-64 (2 boxes).** (1) The ADC's resolution, reference, input range, sampling time, maximum rate, input impedance, registers and conversion start must come from the microcontroller chosen for the kit. (2) Lab Part B (reading a real ADC) is untested on hardware. The 3.0 V reference and 10-bit ADC are pretend values.
- **F1-65 (2 boxes).** (1) The U-IMU6 is invented by this university: address 0x2A, ID 0xA5, its register map, ranges, bias and noise. A real IMU's values come from its datasheet. (2) Part B (a real IMU on I2C with a logic analyser, against the H2 acceptance tests) is untested on hardware.
- **F1-66 (2 boxes).** (1) The encoder's lines per revolution, output type, supply, channel order and index behaviour; whether the MCU has a hardware encoder mode. (2) Part B (timer encoder mode on the kit) is untested.
- **F1-67 (2 boxes plus a laser safety box).** (1) The LiDAR or depth camera's range, accuracy, resolution, scan rate, wavelength, laser class, invalid-value encoding, interface and frame. (2) The optional real-sensor step is untested. The safety box sends readers to the sensor's stated laser class under the laser-safety standard (IEC 60825-1, title only).
- **F1-68 (1 box).** Every camera or image sensor parameter: resolution, pixel size, full well, read noise, bit depth, shutter type, row time, Bayer order, interface and registers. The 600 px focal length, 10,000 e- full well and 1.0 ms row time are pretend values.
- **F1-69 (1 box plus safety).** Motor R, L, K, J, b, the breakaway torque, the servo pulse mapping, speed and deadband, and the BLDC Hall-to-phase table are all pretend values. A wrong Hall table can make a real BLDC jerk or overheat.
- **F1-70 (1 box plus safety).** Switching delays, the required dead time, the PWM frequency range, current limits, protection and the ESC protocol. The 20/60 ns delays, the 500 Hz and 20 kHz frequencies and the 10 kΩ off-resistance are pretend values. The "10,000 V" is a property of the model only.
- **F1-71 (1 box plus the LiPo safety box).** The cell's 3.6 V and 2.0 Ah, the OCV table, the 0.05 Ω, the 3.20 V cutoff, the 10.8 V pack, the 0.15/0.10 Ω, the 7.0 V brown-out threshold and the 100 ms boot time are all pretend values. **The chapter deliberately gives no real lithium cell voltages, current limits or temperature limits.** Every such limit is deferred to the battery maker's datasheet and the charger manual.
- **F1-72 (1 box).** The 10 mV noise, the 25 mV offset, the 0.05 Ω shared ground and the 3 A motor current are pretend values in synthetic logs. Real coupling must be measured on the real wiring.

Claims based on the general mechanism, which a Source Researcher should check against the named book sections once they are opened:

- correlated double sampling and the global-shutter storage element (F1-68, Szeliski);
- the dead time rule "off-delay minus on-delay" and body-diode freewheeling (F1-70, H&H and the driver datasheet);
- the EMA cut-off ≈ a·fs/2π (F1-72, Oppenheim & Schafer);
- star grounding and twisted-pair cancellation (F1-72, Ott);
- pouch-cell swelling as a warning sign (F1-71, Linden's handbook and the maker's instructions).

## Proposed source-registry additions

These are cited title-only and not opened. The Source Researcher must confirm the editions:

1. Alan V. Oppenheim, Ronald W. Schafer, "Discrete-Time Signal Processing" (F1-64, F1-65, F1-72).
2. Bureau International des Poids et Mesures, "The International System of Units (SI)" (F1-67). It may already be registered, since HW101/F1-01 cites it.
3. Richard Hartley, Andrew Zisserman, "Multiple View Geometry in Computer Vision" (F1-67, F1-68).
4. Richard Szeliski, "Computer Vision: Algorithms and Applications" (F1-68).
5. Austin Hughes, Bill Drury, "Electric Motors and Drives: Fundamentals, Types and Applications" (F1-69, F1-70).
6. Thomas B. Reddy (ed.), "Linden's Handbook of Batteries" (F1-71).
7. Henry W. Ott, "Electromagnetic Compatibility Engineering" (F1-72).
8. IEC 60825-1, safety of laser products (F1-67, safety box only).

These were also used, title-only: Horowitz & Hill; Blitzstein & Hwang; Beard & McLain; Lynch & Park, "Modern Robotics"; Thrun, Burgard & Fox, "Probabilistic Robotics"; Harris & Harris. NXP UM10204 (the I2C-bus specification) is named in the Systems Curriculum H2 reading list.

## Proposed analogy mappings (world F1: the restaurant building and its kitchen)

Each mapping is used in one chapter. They are proposed for the analogy registry and are not yet registered:

| Concept | Mapping | Chapter |
|---|---|---|
| Transducer, sampling, quantisation | the cold-room thermometer and Amara's log book; Kenji reads it | F1-64 |
| IMU (accelerometer, gyroscope, magnetometer) | waiter Lucía carrying soup: wrist = accelerometer, sense of turning = gyroscope, sunlit courtyard = magnetometer | F1-65 |
| Quadrature encoder | the pasta crank's clicks, with two springs offset (Ade, Ravi) | F1-66 |
| Time of flight / LiDAR / stereo | Kwame's clap echo in the cellar; Ines closing one eye at a time | F1-67 |
| Pinhole camera, rolling shutter | the storeroom camera obscura (Nadia); Farid copying the wall row by row | F1-68 |
| DC motor, back-EMF, stall | the cellar water wheel driven from the roof tank (Tomás, Yuki); push-back of the spinning wheel = back-EMF | F1-69 |
| H-bridge, PWM, flyback | four valves around the water wheel (as KID103 already proposes for the H-bridge); flicking valves = PWM; water hammer and bypass loop = inductive kick and freewheeling diode (Olu, Sven) | F1-70 |
| Battery, internal resistance, brown-out | the roof tank with a narrow outlet; the dishwasher and the coffee machine sharing one pipe (Hamid, Grace) | F1-71 |
| Noise vs interference, shared ground | Kenji's dipstick readings averaged; the soup pot's drain sharing the last pipe with the dishwasher (Ama) | F1-72 |

The water-in-pipes picture matches HW101's existing glossary analogy ("charge = the water itself in the restaurant's pipes").

## Glossary

`glossary.json` holds 75 four-part entries, generated from the chapters' jargon boxes. They contain no `<sup>` claim tags, and their sources are marked "pending verification (dossier gate G1 open)".

Existing terms from other courses are linked rather than redefined: Sampling and quantisation, Encoder, Distance sensor, Pixel, Motor, Motor driver, H-bridge, Duty cycle, Diode, Capacitor, Battery, LiPo battery, Fuse, Short circuit, Noise, Sensor noise, Ground (reference). A check against every other course's glossary found no term defined twice.

## Decisions left to the owner

1. **Kit parts.** The microcontroller, IMU, encoder (and motor with encoder), LiDAR or depth camera, camera module, DC gear motor, servo, BLDC and ESC, motor-driver IC, battery pack, charger and logic analyser are all still to be chosen. Every "Part B" hardware step, and the H2 acceptance tests on hardware, wait on these choices and on dossier part F.
2. **Safety rules.** The F1-71 LiPo rules state the course card's safety line (maker's instructions; supervised for L2; no propellers; no mains) plus common-sense handling rules (proper charger, not unattended, no damaged packs, no shorts, fuse near the pack). The owner should confirm these against the chosen battery maker's instructions and the university's safety policy.
3. **Laser safety (F1-67).** Confirm which laser class is allowed for L2 learners once a sensor is chosen.
4. **Course project.** The course project (F1-72) is defined as the course card's "sensor board logger with calibrated output, reused in RB201 and DN202". It uses H2's acceptance tests verbatim for the hardware part. The owner should confirm that RB201 and DN202 expect this scope and log format.
5. **Chapter titles.** The chapter titles follow the course card. F1-71's meta title is "Batteries (LiPo) and power distribution", and F1-72 is "Noise, grounding and interference"; the course project lives in F1-72's mini-project.
