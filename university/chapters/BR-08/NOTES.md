# BR-08 — From simulator to real robot or drone (sim-to-real) (author notes)

Bridge chapter, level L3–L4, placed before the first real-hardware lab of RB304 (F9-48) and of
DN401 (F10-33, F10-34), revisited in F9-71. Fragment: `university/chapters/BR-08/BR-08.html`
(front matter `id: BR-08`, `course: BR`). Lab: `university/labs/BR-08`. Glossary proposals:
`university/chapters/BR-08/glossary.json` (7 new terms; 26 existing terms linked, not redefined).

`university/labs/run_lab.sh university/labs/BR-08` exits with status 0 (re-run after the last
change, 2026-10-10); every `.log` records `exit code: 0`. `python3 university/build/build.py`
reports no PROBLEM line that mentions BR-08 (the 16 remaining PROBLEM lines are broken glossary
links in other chapters). The HTML passes a python html.parser balance check; every id starts
with `BR-08`; no URL; no `<script>`.

## Toolchain (as recorded in the logs)

- g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0 with
  `-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined`.
- Python 3.13.16 with numpy 2.5.3, run as `python3 -I` (compare_logs.py).

## Listings and runs

**No hardware was used.** Every "kit" in the chapter is `kit_model.hpp`, a stand-in model with
invented exercise values. `run.sh` appends "hardware: untested on hardware; the 'kit' is the
stand-in model kit_model.hpp" to every log that uses it. All programs are seeded: outputs repeat
byte for byte.

| Run (`.log`) | Listing | What | Status |
|---|---|---|---|
| paired_run | 3 (uses 1, 2) | kit attempt 1 fails the sign test at duty 0.1; one change (0.2); the pair sim_run.csv / kit_run.csv | pass; untested on hardware |
| compare | 4 | the two logs explained: timing, rest, step fit, calibration, loop, noise source, vibration; identified.txt | pass |
| resim | 5 | updated simulator, each effect alone, cumulative ladder, friction structure change | pass |
| kit_log_head | — | first 14 lines of kit_run.csv (run record) | pass |
| trap_gains | 6 | sim-tuned gains oscillate on the kit; drone rate axis with vibration (model) | pass; untested on hardware |
| trap_bench | 7 | floor-first vs bench-first on a correct, a swapped-encoder and a 512-count kit | pass; untested on hardware |
| trap_estop | 8 | hung program: nothing / in-process watchdog / driver timeout / hardware e-stop; arming interlock | pass; untested on hardware |
| trap_two_changes | 9 | two changes at once vs one at a time | pass; untested on hardware |
| worked | 10 | arithmetic of the worked example | pass |
| forensic_gen | — | forensic evidence: morning and afternoon logs | pass (stand-in) |
| forensic_replay | — | answer key: one change at a time, the timestamp fix | pass (stand-in) |
| forensic_compare | 4 | Listing 4 on the forensic pair (answer key) | pass |

No expected-fail runs. The sign-test failure of kit attempt 1 is a result printed by a passing
program (exit 0), not a failed run.

## Unverified boxes

1. Layer 3, delay margin: formula and integrator approximation recalled, D1 not opened; the
   approximation is poor near the sampling rate (the sim-tuned gains in the simulator are stable
   although ωc × delay = 1.51 rad).
2. Layer 3, drone path: flight-stack pre-arm checks, motor test, kill switch, HITL and log fields
   not described (D5, D6 not opened); sag and propeller vibration growing with thrust is physical
   reasoning, to be confirmed on the vehicle.
3. Hardware section: untested on hardware; stand-in values invented; driver timeout, ADC divider
   and speed-reference instrument must come from the kit's documents (D8).

Safety boxes: hardware section, trap 3 (interlock is a teaching program; props removed and
checked by people), lab. Local drone rules: D7, read and recorded by the learner; no rule detail
stated.

## Sources

Books (title only, not opened, gate G1 open): D1 Åström & Murray "Feedback Systems"; D2 Ljung
"System Identification: Theory for the User"; D3 Liu "Real-Time Systems". Project documentation
(not opened): D5 PX4 Autopilot User Guide; D6 ArduPilot documentation. D7 national aviation
authority rules; D8 the course kit's manufacturer documents (kit not chosen). C1 guide, C2
curriculum. R1–R11 runs. No PX4/ArduPilot parameter or command names, no commercial kit or
simulator names, no hardware numbers except those printed by our own runs.

## Analogy proposals (for the registry)

- F9 bicycle, extended: the paired run = riding the same route on the trainer and on the road
  with the same cycle computer; the run record = the line in the ride diary; bring-up gains =
  the first laps at walking pace; reference check = riding beside a friend whose cycle computer
  is known to be right. F9-22's "checking the brakes while standing still" (bench test) and
  "hand on the saddle" (e-stop) are reused, not redefined.
- F10 tray, extended: the props-off bench = rehearsing the route with empty cups (teaches the
  route, not the sloshing).
- The story hook (Rafael and aunt Nadia) uses the registered bicycle; names are new.

## Decisions for the owner

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
