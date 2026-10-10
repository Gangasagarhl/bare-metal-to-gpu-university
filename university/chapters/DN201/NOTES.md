# DN201 — Flight physics and multirotor dynamics: author notes

Chapters F10-01 to F10-06, level L2, 3 credits. Prerequisites: MA201, RB202.
Labs are in `university/labs/F10-01` to `university/labs/F10-06`.
Each lab passes `university/labs/run_lab.sh university/labs/<ID>` with exit code 0. All six were re-run at the end of this build, on 2026-10-10.

F10-02 and F10-03 include the shared simulator header `../F10-04/quadsim.hpp`. If that header changes, re-run all three labs.

The numbers quoted in the prose, figures, worked examples and answer keys were checked against the `.out` files. Each lab also has a `checks.cpp`, which recomputes every number in the text that no listing prints. It is run but not shown as a listing.

## Toolchain and local evidence

- **g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0.** Every `.cpp` is built with `-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined` by `run_lab.sh`.
- **No hardware.** No flight controller, motor, ESC, propeller, thrust stand, battery or aircraft was used.
  - Every vehicle number belongs to "the course quad", "the course motor" or "the course aeroplane". These are exercise values and are labelled that way.
  - Course quad: m = 1.0 kg, arm 0.15 m, J = diag(0.010, 0.010, 0.018) kg·m², kT = 1e-5, kQ = 1.5e-7.
  - Course motor: R = 0.1 Ω, Ke = Kt = 0.01, Jr = 2e-5 kg·m².
  - Course aeroplane: 1.5 kg, S = 0.30 m², CLmax = 1.2, CD0 = 0.03, k = 0.05, ρ = 1.225 kg/m³.
- **No PX4, ArduPilot or Gazebo parameter names, commands or values appear as facts.** Everything about them is in "Not verified" boxes.

## Listings run

**Nothing was run on hardware. No run is an expected failure.** Every run exits with code 0. The forensic programs produce evidence logs and also exit 0.

| Chapter | Run | Status |
|---|---|---|
| F10-01 | forces | pass (hover thrust and thrust-to-weight; tilt table) |
| F10-01 | vertical1d | pass (1D vertical simulator; terminal climb 6.264 m/s) |
| F10-01 | hover_bug | forensic evidence, exit 0 (feedforward fixed at 9.81 N; settles at 1.387 m with a mass of 1.25 kg) |
| F10-01 | checks | pass (recomputes text numbers) |
| F10-02 | mixer | pass (forward and inverse mixer, six requests) |
| F10-02 | yaw_climb | forensic evidence, exit 0 ("It yaws when it climbs": motor 3 carries a propeller with kT ×1.2 and kQ ×1.6) |
| F10-02 | checks | pass |
| F10-03 | attitude | pass (R, round trip, Euler-rate check, 1/cos(pitch) table) |
| F10-03 | loop_log | forensic evidence, exit 0 (roll and yaw jump by 180° at ±90° pitch) |
| F10-03 | checks | pass |
| F10-04 | sim2d | pass (planar simulator, roll pulse) |
| F10-04 | sim3d_tests | pass (tests T1–T6, "ALL TESTS PASSED") |
| F10-04 | no_gyro_bug | forensic evidence, exit 0 (gyroscopic term left out; energy constant but world angular momentum changes) |
| F10-04 | checks | pass |
| F10-05 | motor_prop | pass (steady state, hover 5.320 V, step t63 = 17.14 ms against 17.41 ms linearised) |
| F10-05 | fit_thrust | pass (least-squares fit kT = 1.0035e-5 from a seeded synthetic stand table) |
| F10-05 | stand_linear | forensic evidence, exit 0 (straight-line fit, smile-shaped residuals, hover predicted at 427.4 rad/s) |
| F10-05 | checks | pass |
| F10-06 | fixed_wing | pass (level flight, best L/D 12.91, stall 8.17 m/s, turn table) |
| F10-06 | glide | pass (point-mass glide converging to −4.429°, 10.153 m/s) |
| F10-06 | turn_stall | forensic evidence, exit 0 ("It fell out of the turn": CL needed exceeds 1.2 beyond 56.5° bank at 11 m/s) |
| F10-06 | checks | pass |

**Scratch runs.** Modified copies of listings were run in scratch space to check the predictions in "Check yourself" and lab "Expected observations". Their results are quoted in the text, each marked "scratch run, not part of the lab logs":

- F10-01: ratio 1.1 gives 3.849 m/s at 8 s; the clamp gives 30.19 m/s²; halving dt changes only the last decimal.
- F10-03: pitch −89.9° gives rates near ±262 rad/s; the two rotation orders differ by up to 0.18.
- F10-04: six break tests. Removing the gyroscopic term fails the momentum and flip checks. The conjugate quaternion fails T3. Leaving out the renormalisation still passes. Swapped motorY fails T4. Forward Euler fails T2 and T6. A flipped gyroscopic sign fails only the momentum check.
- F10-05: a +0.1 V step gives 17.38 ms; a +3 V step gives 16.66 ms.
- F10-06: starting the glide in its steady state stays at −4.43°.

## Unverified boxes and claims that could not be verified, per chapter

All book sources are cited by title only ("not opened during this build; dossier gate G1 open"):

- B1, Beard and McLain, "Small Unmanned Aircraft: Theory and Practice", registry 4.8.
- B2 in F10-04, Lynch and Park, "Modern Robotics", registry 4.7.

**F10-01 (1 box).** How real rotor thrust changes in climb, in descent, in forward flight and near the ground (ground effect), and a real airframe's drag coefficient. The drag coefficient 0.05 is an exercise value. The value g = 9.81 m/s² is cited as V1 and needs a registered source.

**F10-02 (2 boxes).**
1. How PX4 and ArduPilot prioritise thrust, yaw and roll/pitch when the mixer saturates, and the names of the related settings.
2. The motor numbering and spin directions are the university's own. PX4 and ArduPilot define their own per airframe, and these may differ.

**F10-03 (1 box).** The axes, rotation order and quaternion convention that PX4, ArduPilot and MAVLink use in their messages and logs. DN201 uses z-up axes (x forward, y left), not the NED axes of Beard and McLain. Converting between the two needs care, and this is flagged.

**F10-04 (2 boxes).**
1. How large the effects left out of the model are on a real vehicle: rotor gyroscopic torques, airflow effects on thrust, off-diagonal inertia.
2. **Untested in this build: validation against a named simulator.** The course card asks for this. No external simulator (Gazebo, the PX4 or ArduPilot SITL) was available or run. The simulator is checked only against hand-derived results (tests T1–T6). The chapter states this and describes what a validation run would compare.

**F10-05 (4 boxes).**
1. The dimensionless propeller coefficients and their dependence on airspeed; how to convert manufacturers' propeller data.
2. The definition of "Kv" on data sheets. Using Ke = 60/(2π·Kv), Ke = 0.01 corresponds to about 955 rpm/V. How close real motors come to Kt = Ke is also unverified.
3. Which ESC protocols report rotor speed, and in what units.
4. That the raw `std::mt19937` sequence for a given seed is the same on every implementation. This comes from the standard's engine specification; the clause was not opened.

**F10-06 (4 boxes).**
1. ρ = 1.225 kg/m³ as the ISA sea-level density, and how density changes with altitude and temperature.
2. VTOL support and transition handling in PX4 and ArduPilot.
3. Reynolds-number effects, tip stall and spins. These are general aeronautics recalled while writing.
4. Airspeed sensors and how flight software uses them.

**Recalled general claims a Source Researcher should check against B1 once it is opened:**

- the quadratic static propeller model;
- the ideal DC-motor model for a BLDC with its ESC;
- the drag polar CD0 + k·CL²;
- the phugoid name for the speed–height oscillation;
- the three VTOL layouts;
- the statement that Beard and McLain include airspeed in their propeller models.

## Decisions left to the owner

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

## Proposed source-registry additions

- **Kevin M. Lynch, Frank C. Park, "Modern Robotics".** Cited in F10-04 as registry 4.7. If it is not registered there, it should be added.
- **A source for g = 9.81 m/s²** (F10-01, V1). The SI brochure or a standard-gravity reference.
- **An International Standard Atmosphere source** (F10-06, ρ = 1.225 kg/m³).

## Proposed analogy mappings (world F10: carrying a tray of drinks while walking)

None of these is registered yet.

| Concept | Mapping | Chapter |
|---|---|---|
| Thrust, weight, drag | arms pushing the tray up; the glasses pulling down; the breeze pushing back (Amara; Kofi in the forensic) | F10-01 |
| Rotors and reaction torque | four fingertips under the tray's corners; the twist of fingertip friction (Jonas, Leila) | F10-02 |
| Euler angles | Mei's spoken "turn, tip forward, tip sideways" instructions, in order (Mei, Ravi, Sven) | F10-03 |
| Inertia, rigid body | Kofi's feel that a long or heavy tray is harder to turn; the tray with glued-on glasses (Kofi; Ana in the forensic) | F10-04 |
| Motor dynamics and power | Priya's arms: tiring faster than the extra load suggests; the dip before arms catch up with a new jug (Priya; Tomás in the forensic) | F10-05 |
| Lift, stall, banked turn | Nadia's tilted empty tray lifted by the wind and flapping when tipped too far; leaning the tray into the fountain corner, glasses pressing harder (Nadia; Ibrahim in the forensic) | F10-06 |

The gyroscopic term has no tray mapping. The chapter says so and uses a spinning book instead.

## Glossary

`glossary.json` holds 38 four-part entries generated from the chapters' jargon boxes. The entries contain no claim tags, and every source is marked "pending verification (dossier gate G1 open)".

**Terms from other courses are linked, not redefined:**

- MA201: Attitude, Euler angles (yaw–pitch–roll), Gimbal lock, Quaternion, Unit quaternion, Rotation matrix (3D), Body frame (robot frame), World frame.
- RB201: Torque.
- HW302: Back-EMF, Motor constants (Kt and Ke), Brushless DC motor (BLDC), ESC (electronic speed controller), and the IMU entry (IMU (inertial measurement unit)).
- MA302: the Euler methods.
- HW101 and RB202: Time constant (τ). Both courses define this term, which is outside DN201's scope to fix.

**Two names were chosen to avoid clashes:**

- CU401 already defines "Thrust" (the C++ library), so this course uses "Thrust (propeller force)".
- HW301 defines "Stall" (a GPU warp stall), so this course uses "Stall (aerodynamic)".

A check against every other course's glossary found no term defined twice. Every `#gl-` link in the six chapters resolves.

## Build check

`python3 university/build/build.py` ran with exit code 0. None of its PROBLEM lines names DN201, F10-01 to F10-06, or an anchor or glossary term of this course. The PROBLEM lines that remain belong to other courses.
