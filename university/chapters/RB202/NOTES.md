# RB202 — Control I: PID: author notes

These notes cover chapters F9-16 to F9-22, level L2, 3 credits. The prerequisites are RB101 (or KID103), SP102 and MA101; MA301 deepens the course later.

The course maps to the PID chapter of "Feedback Systems" (Åström, Murray), registry 4.7, tier 3, cited by title only. The Systems Curriculum has no PID milestone, so each chapter's `maps` field names the book chapter and says that there is no milestone.

The labs are in `university/labs/F9-16` to `university/labs/F9-22`. Every lab folder passes `university/labs/run_lab.sh university/labs/<ID>` with status 0. All seven were re-run at the end of this build (2026-10-10), after the last source change.

`python3 university/build/build.py` reports no PROBLEM line for these chapters or for RB202. The build does report 154 PROBLEM lines, all broken `#gl-…` links belonging to other courses. I checked that none of them comes from an `href` in these seven files.

## Toolchain (as recorded in the logs)

- g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0 with `-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined`, on the Linux x86_64 build container.
- Formatting: clang-format with an LLVM base style, IndentWidth 4, ColumnLimit 100, braces after functions, structs and classes, and short functions allowed on one line only when inline. The config was kept in the author's scratch folder and is not part of the repo. Every line-by-line table was rechecked against the formatted line numbers.

## Listings run

All programs are the university's own simulations: the pretend motor of HW302 F1-69 and MA301 F0-68, the cart of MA301 F0-67, and a simulated motor kit. Every run is a pass. There are no expected-fail runs.

Nothing in F9-22 was run on hardware. Its runs use the `SimulatedKit` class, and the bench steps are marked untested on hardware.

| Chapter | Run (`.log`) | Exit | Status |
|---|---|---|---|
| F9-16 | load_step | 0 | pass |
| F9-16 | open_closed | 0 | pass |
| F9-16 | stale_sensor | 0 | pass (forensic evidence) |
| F9-17 | buzz_log | 0 | pass (forensic evidence) |
| F9-17 | p_sweep | 0 | pass |
| F9-17 | p_trace | 0 | pass |
| F9-18 | ki_sweep | 0 | pass |
| F9-18 | pi_load | 0 | pass |
| F9-18 | windup_log | 0 | pass (forensic evidence; this is the course card's "Windup" forensic) |
| F9-19 | chatter_log | 0 | pass (forensic evidence) |
| F9-19 | kick | 0 | pass |
| F9-19 | pd_sweep | 0 | pass |
| F9-19 | pd_trace | 0 | pass |
| F9-20 | settle_report | 0 | pass (forensic evidence) |
| F9-20 | sweep | 0 | pass (runs about 2550 simulations with sanitizers; `sweep.timeout` = 60 s) |
| F9-20 | trace | 0 | pass |
| F9-20 | tune | 0 | pass |
| F9-21 | nan_log | 0 | pass (forensic evidence) |
| F9-21 | test_pid | 0 | pass (10 unit tests, "0 test(s) failed") |
| F9-21 | windup_fixed | 0 | pass |
| F9-22 | identify | 0 | pass; simulated kit, untested on hardware |
| F9-22 | speed_loop | 0 | pass; simulated kit, untested on hardware (the program prints its run-A/B/C results; its own exit status is 0) |
| F9-22 | step_test | 0 | pass; simulated kit, untested on hardware |
| F9-22 | wrap_log | 0 | pass (forensic evidence); simulated kit, untested on hardware |

Two lab files depend on others:

- `F9-22/identify.in` is a byte-for-byte copy of `F9-22/step_test.out`. I checked with `diff` after the last run. If `step_test.cpp` or `motor_io.hpp` changes, copy it again.
- `F9-22/speed_loop.cpp` and `F9-22/wrap_log.cpp` use `#include "../F9-21/pid.hpp"`. This is the course project library, used without changes.

## Sources

- **D1**: the chapter's own derivations and design reasoning. These are true by the algebra shown and checked by the runs.
- **B1**: "Feedback Systems" (Åström, Murray), cited by title only. Every chapter carries the exact phrase "Title only — not opened during this build; the Source Researcher must confirm the edition and section (dossier gate G1 open)".
- **U1**: university chapters, cited only for what they teach.
- **R1..Rn**: the lab runs.

No URLs are used, no hardware numbers are given, and no commercial kit, board or simulator is named.

## Unverified and untested-on-hardware boxes

- **F9-19**: the "Td/N" form of the derivative filter and any recommended range of N. To check in B1, PID chapter, derivative filtering.
- **F9-20**: whether B1 presents the Ziegler–Nichols step-response and ultimate-gain methods and criticises them. No formulas or tables are quoted.
- **F9-21**: whether B1 presents back-calculation (tracking) anti-windup, and its tracking time constant. No formula is given. The library implements only conditional integration.
- **F9-22**, two boxes, both untested on hardware:
  - `KitMotorIo` is not written, because the PWM, encoder and timer APIs depend on the kit. The box names the documents to read.
  - The bench procedure and the real kit's behaviour have not been observed. Every constant (12 V, 1024 counts, 16-bit counter, 0.5 V deadband, 400 rad/s limit, watchdog thresholds) must be replaced by values the supervisor approves.

F9-16, F9-17 and F9-18 have no unverified box. All their numbers come from runs or from algebra shown in the text, and their book references are B1 title-only tags.

## Safety

- F9-22 has a chapter safety box and a lab safety box. They cover:
  - simulator first;
  - adult supervision (this is an L2 course);
  - a hardware emergency stop in the power path, tested before the first run;
  - the motor clamped and a clear zone kept;
  - the kit's own low-voltage supply, with no LiPo and no mains;
  - limits set in advance and never raised during the session;
  - one change at a time.
- F9-16 to F9-21 are simulation only. Their lab "Safety" sections say so and point ahead to F9-22.
- F9-22 refers to BR-08 for the sim-to-real gap and to RB101 F9-07 for the faculty safety rules.

## Values from scratch runs used in the text

These numbers come from runs in the author's scratch folder: lab-step variations that are not lab listings. The text calls each one "a scratch run of this build" and tells the learner that their own run is the check.

- **F9-16 lab expectations**: closed-loop values for Kp 0.05 and 0.1.
- **F9-18**:
  - Ki 0.5 gives 6.8 % overshoot.
  - A 600 rad/s step peaks at 652.9.
  - With protection, the peak is 899.7.
  - The pure-I answer is 433.5 at 0.25 s and 254.9 at 0.5 s.
- **F9-20**:
  - With a 2.5 kg cart, all seven experiments fail. Experiment 6 fails S3 at 2.5 mm; experiment 7 fails S2 at 3.72 s.
  - A −3.0 N slope makes every experiment fail.
  - The three-mass robustness sweep leaves 0 of 2550 sets passing.
- **F9-21**:
  - With conditional integration removed, all 10 tests still pass and the "on" run overshoots to 938.3 (4.3 %, I = 11.93 V). This shows a gap in the test suite, which the course project asks learners to close.
  - With ±20 V limits, the anti-windup-off run gives 957.0 (6.3 %).
- **F9-22**:
  - A duty 0.5 step test identifies 88.9 rad/s per V and τ 0.195 s.
  - A 1.5 V deadband makes the duty-0.10 sign test fail (+0.0 rad/s) in all three runs.

## Glossary

`glossary.json` has 48 entries.

There are 36 new terms, each with its four parts and the source "… of F9-xx (pending verification)". Their related terms all exist in some glossary.

There are 12 terms that already exist elsewhere. I repeated them with the existing wording copied exactly, adding only my chapter lists, so the build's merge unions the chapters:

- Plant, Static gain (DC gain), Steady-state error, Saturation (actuator), Overshoot, Running sum (discrete integrator), Damping ratio (ζ), Settling time, Noise amplification (of differentiation) and PID controller (discrete), from MA301;
- Time constant (τ), from HW101;
- Unit test, from SP101.

"Controller (control loop)" is named that way because "Controller (Kubernetes)" already exists.

## Decisions for the owner

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
