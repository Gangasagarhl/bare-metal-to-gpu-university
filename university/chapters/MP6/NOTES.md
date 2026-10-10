# MP6 — A robot software stack (mega project handbook): author's and Lab Engineer's notes

Handbook `MP6.html` (front matter `id: MP6`, `course: MP`, level L4–L5). Starter lab in
`university/labs/MP6/`. Written in the cloud build container on 2026-10-10, with no internet
access, no ROS 2, no simulator other than our own, and no robot, board or GPU. Nothing was
committed.

## Files

- `chapters/MP6/MP6.html`: all 21 template sections in order, plus "Answers to Check yourself"
  with the forensic answer key as an h3. Adapted to a project:
  - Layer 2: what the learner builds, a table mapping every stack part to the earlier
    chapter and lab it reuses, and the starter lab as a walking skeleton of M1.
  - Layer 3: milestones M1–M6, each with entry criteria, tasks, evidence and "builds on"
    chapter links; gates R0–R4 with a checklist and proposed signers per gate;
    deliverables; the rubric with the card's weights and four level descriptors per
    criterion; a risk table.
  - Forensic lab with an incident write-up template (h3 `MP6-incident`).
  - Two inline SVG figures: the architecture (Figure 1) and the 28-week milestone timeline
    with gates (Figure 2).
- `chapters/MP6/glossary.json`: 6 new four-part terms (Task success rate, Absolute
  trajectory error (ATE), Log schema, Paired run, Stopping distance, Surviving mutant).
  Every other term is linked to its existing entry: about 40 links to RB302, RB303, RB304,
  RB401, RB403, SS402, SE501, DR401 and RB101 terms. Each `#gl-` link was checked against
  the built page.
- Local checks passed:
  - html.parser balance;
  - 63 ids, all prefixed `MP6`, no duplicates;
  - no URL and no `<script>`;
  - section order;
  - every claim tag has a source and every source is tagged;
  - every `data-src` and `data-run` file exists.
- `python3 university/build/build.py`: **no PROBLEM line at all** in the final run (603 of
  603 chapters, MP6 included, its QA record generated).
- Prose length is about 8,500 words without tables and listings, and about 11,000 with
  tables. This is at the top of the L5 range (3,000–8,000 plus project material). The
  extra length is the project material: the milestones, gates and rubric.

## Listings run (`university/labs/run_lab.sh university/labs/MP6` from the repo root: exit 0)

All runs used g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0 with
`-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined`. All are
deterministic (seeded splitmix64 from F9-52). Every `.out` and both CSV logs were
byte-identical on a second full run.

| Run | Exit | Status |
|---|---|---|
| `m1_checks` (worked-example arithmetic) | 0 | pass |
| `m1_forensic` (evidence pack, `m1_forensic.in`) | 1 | **expected fail**: this is the incident, and T1 and T3 fail by design |
| `m1_forensic_fix` (answer key: gate only; re-surveyed map) | 0 | pass |
| `m1_mission` (one nominal delivery, writes `m1_log.csv`) | 0 | pass |
| `m1_suite` (10 seeds, 3 safety scenarios, 5 mutants; `m1_suite.timeout` = 120 s) | 0 | pass: 10/10 tasks, all safety checks, 4/4 required mutants killed; `radius_plus_2pc` survives (documented, informational) |

Generated logs kept in the folder: `m1_log.csv` (126 lines) and `m1_forensic_log.csv`
(125 lines). They are small, and the log is one of the deliverables being demonstrated.

**Untested on hardware: everything.** M2–M5 and the hardware goals of M6 need:

- the kit robot;
- a robot computer with a PREEMPT_RT kernel;
- the microcontroller board and a debug probe;
- a supervisor.

**Untested in this build:** every ROS 2, ros2_control, micro-ROS, Nav2 and simulator step,
because none of these tools is installed.

## Cross-lab dependencies (deliberate reuse; owner please note)

- `labs/MP6/mp6_robot.hpp` includes `../F9-56/nav.hpp`, which pulls in
  `../F9-54/planner.hpp` and `../F9-52/house.hpp`.
- `mp6_ekf.hpp` includes `../F9-34/mat.hpp`.
- If those RB302/RB401 files change or move, rerun this lab: the outputs and the numbers
  quoted in the handbook would change. RB401 already uses the same pattern.

## Unverified boxes (AH-18): 6 boxes, plus the standards Safety box (item 7)

1. Layer 2: no ROS 2, ros2_control, micro-ROS, Nav2 or simulator names. The starter's
   config keys are our own format.
2. M2: PREEMPT_RT kernel options and boot parameters, threaded IRQ priorities;
   ros2_control interfaces.
3. M3: micro-ROS supported RTOSes and boards, agent, transports, entity limits.
4. M4: no Linux image was built anywhere in this university. The image builder and boot
   chain come from the chosen tool's documentation.
5. Rubric: how the 25 % task-success criterion is graded if no hardware run is possible
   (owner decision, see below).
6. Hardware section: "Untested on hardware". The starter's numbers are the lab plan's
   choices, not kit data.
7. Safety box (M6): the standards ISO 13850, IEC 60204-1 and ISO 13849-1 are named by
   title only, and no requirement is claimed. This is a Safety box, not an unverified box,
   but it is listed here for the QA record.

All D-sources are "title only — not opened (dossier gate G1 open)":

- ROS 2, ros2_control, micro-ROS docs;
- the Linux "Real-Time Preemption" docs;
- Probabilistic Robotics;
- Zephyr and FreeRTOS docs;
- the kit makers' documents (not chosen);
- ISO 13850, IEC 60204-1, ISO 13849-1;
- Buttazzo and Liu;
- Nav2 and Gazebo docs.

C1 (guide) and C2 (curriculum) are local files, opened. They are a map, not a source.

## Claims and numbers

- Every number about the starter robot is a choice of the lab plan, stated as such. These
  are: wheel radius 0.050 m, track 0.300 m, 1024 CPR, ramp 0.8 m/s², speed limit
  0.30 m/s, watchdog 0.25 s, beacon noise, and the post positions.
- Every result number is printed by a run (R1–R5) or recomputed by `m1_checks` (W1/R5).
  These are: stop time 0.48 s, stopping distance 0.059 m, W1 0.21 s, gate 9.21 (1 %),
  and expected false rejections 8.5 against 5 observed.
- The real-time example (96.74 %, 9 periods over 2 ms) is cited from `labs/F9-50/cm_rt.log`.
  It is a measurement on the build VM (AH-23).
- The Check-yourself answers 4 and 5 are predictions by arithmetic. I ran both variants
  privately to confirm them (watchdog 0.10 s gives W1 0.05 s; limit 0.20 m/s gives E2
  0.25 s and E3 0.024 m, and all pass), but these runs are not recorded in the lab folder.
  The answers tell the learner to run them.
- The curriculum's H4 acceptance test is quoted verbatim in M3.

## Decisions for the owner

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

## Glossary overlaps

- No new term collides with an existing one. I checked against all `gl-` ids in the built
  page.
- "Task success rate" and "Paired run" are course definitions.
- ATE is used here in the simplified sense of the starter (same frame, no alignment). The
  usual SLAM-benchmark sense includes alignment. The definition says so implicitly
  ("no alignment step is needed").
