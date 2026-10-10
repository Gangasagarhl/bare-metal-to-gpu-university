# RB402 Control II: state space — build notes

Author and Lab Engineer notes for F9-59 to F9-64. Labs are in `university/labs/F9-59` to `university/labs/F9-64`.

All runs used `g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0` with `-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined` in the Linux x86_64 build container, through `university/labs/run_lab.sh`. Every run exited with code 0.

## Shared code

- `lin.hpp`: small dense matrix library. It has inv, rank, expm, c2d (zero-order hold through the block exponential), charpoly, roots, eig and symEig.
- `cartpole.hpp`: the nonlinear cart-pole (RK4) and its linearisation.

Both files are identical in every lab folder that uses them. They were written for teaching and are not a numerical library. Their eigenvalue routine goes through the characteristic polynomial, which is fine for n ≤ 8 here. Clustered roots lose accuracy, so the F9-63 separation check compares polynomial coefficients instead of eigenvalues.

Later chapters add more headers:

- `place.hpp`: Ackermann's formula (F9-61, F9-63).
- `lqr.hpp`: Riccati iteration (F9-62 to F9-64).
- `noise.hpp`: a seeded mt19937 with Box–Muller, plus quantise (F9-63).
- `mpc.hpp`: a condensed QP solved by FISTA with a soft wall (F9-64).

Exercise model, used everywhere unless stated:

- M = 1.0 kg, m = 0.2 kg point mass, l = 0.5 m, b = 0.1 N·s/m, g = 9.81 m/s².
- T = 0.01 s.

These values are not measurements of any kit.

## Per chapter

### F9-59 State-space models

**Listings run (all pass, host only):**
- R1 `model.cpp`
- R2 `linear_vs_nonlinear.cpp`
- R3 `checks.cpp`
- R4 `push_log.cpp` (forensic evidence: a sign fault injected in B(3,0))

**Unverified claims and boxes:**
- B1 statements are not checked against the book: the solution formula, diagonalisation of e^At, the block exponential for the ZOH, and the non-uniqueness of realisations.
- The cart-pole equations are derived in the chapter (D1) and checked by the energy test in R3. They are not compared with a textbook.
- Untested-on-hardware box: no rig. Real mass, inertia and friction must come from the kit dossier.

**Safety box:** yes (rig motion, end stops, emergency stop).

### F9-60 Controllability and observability

**Listings run (all pass):**
- R1 `ctrb_obsv.cpp`
- R2 `two_poles.cpp`
- R3 `checks.cpp`
- R4 `rig_log.cpp` (forensic: design with nearly equal poles, l1 = 0.50, l2 = 0.48)

**Unverified claims and boxes:**
- B1 theorems: rank tests, PBH, Kalman decomposition, duality, Gramian and minimum-energy formula.
- The name "Popov–Belevitch–Hautus" is given from memory.
- Untested-on-hardware box: the 20 N drive limit is an exercise value.

**Safety box:** yes.

### F9-61 Pole placement

**Listings run (all pass):**
- R1 `place.cpp`
- R2 `sat_sweep.cpp`
- R3 `checks.cpp`
- R4 `bump_log.cpp` (forensic: the 3× design with a 10 N clamp)

**Unverified claims and boxes:**
- B1: canonical form, Ackermann's formula, multi-input freedom.
- From general knowledge: "libraries use orthogonal-transformation methods for placement". No library was consulted.
- Untested-on-hardware box.
- The answer to Check yourself 8 (40 N limit) is given as a prediction to be run. It was not run in this build.

**Safety box:** yes.

### F9-62 The linear-quadratic regulator (course lab and course forensic)

**Listings run (all pass):**
- R1 `lqr.cpp`
- R2 `qr_sweep.cpp`
- R3 `checks.cpp`
- R4 `delay_log.cpp`: course forensic, a 4-sample actuation delay injected. The result is a limit cycle at about 4.6 Hz.
- R5 `balance_lab.cpp`: course lab, LQR balancing of the simulated cart-pole with a 10 N limit and a ±0.5 m track.

**Unverified claims and boxes:**
- The guaranteed margins of continuous-time LQR (gain margin from 1/2 to infinity, phase margin of at least 60°) are given from memory. They are in an unverified box and are not used for any number.
- B1: Bryson's rule, DARE existence conditions.
- Untested-on-hardware box: the delay sources are a checklist, not a measurement.

**Safety box:** yes (limit cycle).

### F9-63 Observers

**Listings run (all pass):**
- R1 `observer.cpp`
- R2 `output_feedback.cpp` (fixed seed 2026)
- R3 `checks.cpp`
- R4 `buzz_log.cpp`: forensic, with W scaled from σ = 0.2 N to 20 N (fixed seed 7).

**Unverified claims and boxes:**
- B1/B2: LQR margins are not guaranteed with an observer; mean-square optimality of the steady-state Kalman gain; predictor and current-estimator forms.
- The margins of the combined controller and observer were not computed. Lab step 4 asks students to compute them.
- Encoder resolutions are exercise values.
- Untested-on-hardware box.

**Safety box:** yes (diverged observer; innovation check).

### F9-64 Model predictive control (concept)

**Listings run (all pass):**
- R1 `mpc.cpp`
- R2 `wall_log.cpp` (forensic: horizon 5 against 20)
- R3 `checks.cpp`: worked example, MPC = LQR check, solver accuracy, and a horizon sweep for N = 5, 10, 15, 20. The sweep was added in this build after a scratch test showed that N = 10 stays below the wall.

**Timeouts:** this folder has `checks.timeout`, `mpc.timeout` and `wall_log.timeout`, each set to 60 s. The sanitizer build of the FISTA solver is slow.

**Unverified claims and boxes:**
- From general knowledge, not tagged to any book: MPC stability theory (terminal cost and terminal set, recursive feasibility), the sparse formulation, the name FISTA, and where MPC was first used in industry.
- The solver's run time was not measured. The lab asks students to measure it.
- Untested-on-hardware box.

**Safety box:** yes (a soft limit is not a safety device).

## Listings untested on hardware

None of the listings drives hardware. All are host C++ simulations and all passed. Every chapter has an untested-on-hardware box covering the rig values, which are exercise values.

## Decisions for the owner

1. **No MPC text in the source registry.** F9-64's MPC content is tagged D1 (own derivations plus runs) or sits in an unverified box. Please add an MPC book or survey to the registry, or accept a concept chapter backed by derivations only.
2. **"Maps to" milestone.** The card maps to "Feedback Systems", which is a book (registry 4.7), not a SYSTEMS_CURRICULUM milestone. The meta "maps" line says "no Systems Curriculum milestone". Confirm, or name a milestone.
3. **Exercise parameters.** The cart-pole values, the 10 N and 20 N limits, the encoder resolutions (1 mm, 2048 counts per turn), the 2 N parking cart and the 4-sample delay are all invented for teaching. If the RB lab kit has a real cart-pole, its datasheet values should replace them, and all runs must be repeated.
4. **Glossary naming.** DS402 already defines "Observability" with the monitoring meaning. RB402 uses "Observability (control)" and its definition points to the other meaning. "Steady-state Kalman gain (observer design)" was chosen to leave "Kalman filter" to RB302.
5. **B2 in F9-63** cites "Probabilistic Robotics" and the Kalman 1960 paper, both from the registry and title only. Confirm these are the intended sources for the Kalman material.
6. **Course project placement.** The course project ("a balancing controller with a written stability argument") is written as F9-62's mini-project, with a rubric. F9-63's mini-project extends it to output feedback. Say if a separate course-level project page is wanted.

## Analogy mapping proposals (F9 world: riding a bicycle; a body with senses and muscles)

These are proposals for the analogy registry, used in this course:

| Technical idea | Analogy | Chapter |
|---|---|---|
| State vector (x, v, θ, ω) | the rider's "where am I, how fast, how far am I leaning, is the lean growing" | F9-59 |
| Unstable equilibrium | balancing upright: any small lean grows | F9-59 |
| Controllability | can my muscles reach every part of my motion? Two bikes held by one bar cannot lean apart | F9-60 |
| Observability | can my senses tell me everything? Eyes closed: lean felt, position unknown | F9-60 |
| State feedback gain K | the reaction rule: so much steering per lean, per lean rate, per drift | F9-61 |
| Wrong-way response | steering slightly right to turn left | F9-61 |
| LQR weights Q, R | "on the bridge, staying on the line matters; on gravel, smooth arms matter" | F9-62 |
| Loop delay | slow reactions make a rider wobble | F9-62 |
| Observer | the model in the head, corrected by the senses | F9-63 |
| Innovation | "it is leaning more than I expected" | F9-63 |
| MPC, receding horizon | looking ahead: "can my weak brakes still stop me before the wall?" | F9-64 |

Where each analogy breaks is stated in each chapter's "Where the analogy breaks" section.

## Scratch

Scratch work is in `/tmp/claude-0/work/RB402/`: the figure scripts, the source fragments with `{{FIGn}}` placeholders, and the validator. Scratch is not part of the deliverable. The chapter HTML files are self-contained.
