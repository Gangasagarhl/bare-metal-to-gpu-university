# MP7 — build notes (Author and Lab Engineer)

Mega project MP7 "A drone flight stack on PX4 or ArduPilot: SITL → HITL → supervised outdoor
test" (guide 11.6). One handbook, `MP7.html` (level L5, `course: MP`), with the 21 template
sections in order plus "Answers to Check yourself" (forensic key as an h3), adapted to a project:
milestones M1–M5 in Layer 2 (entry criteria, tasks, evidence, chapters built on), gates R0–R4
with checklists and signers, deliverables, the card's rubric with level descriptors, and design
arguments in Layer 3; risks and safety under "Common mistakes"; an incident write-up template
under "Forensic lab". Two inline SVG figures (architecture; milestone timeline with gates).

Build date 2026-10-10. Container: Linux x86_64, g++ 13.3.0, Python 3. No internet, no PX4, no
ArduPilot, no flight controller, no vehicle, no field.

## Starter lab: `university/labs/MP7`

A scaled-down skeleton of milestone M1, built and run in the container with
`university/labs/run_lab.sh university/labs/MP7` (overall exit 0, re-run at the end of the build).
It reuses the DN401 course simulator `labs/F10-33/dronesim.hpp` unchanged (included by relative
path, as F10-35 and F10-38 do).

| Program / step | Status | Role |
|---|---|---|
| guard_unit | pass, exit 0 (12 of 12) | unit tests U1–U12 of `est_guard.h` |
| sitl_feature | pass, exit 0 | SITL matrix: N1–N5, M0/M1, G0–G3, K1; writes `sitl_results.csv` |
| run.sh: repeat | pass, exit 0 | rebuild + two runs + recorded output: identical hashes |
| run.sh: report | pass, exit 0 | `report.py`: SITL test report, "COMPLETE", K1 open |
| forensic_mission | pass, exit 0 | forensic evidence (Team B's guard: LAND on first leg) |
| forensic_key | **expected-fail, exit 1 by design** | answer key: our suite fails U2 and U11 on Team B's guard |

Untested on hardware / untested in this build: everything beyond the course simulator — the
port into PX4 or ArduPilot, real SITL, HITL, bench, tethered hover and outdoor flight
(milestones M1 in a real stack to M5). The handbook says so in a "Watch out" box and in the lab.

One scratch run (not a lab listing) supports Check-yourself answer 3: `EG_POS_HOLD = 0.6` in a
copy of the lab gave U5 "latency 0.61 s" (11 of 12) and G2 0.60 s (1 scenario missed).

Design facts of the starter lab (all the course's own, labelled in the handbook):
- `EG_*` parameters, fault names (MAG_FIELD, GNSS_JUMP, GNSS_STALE), the status struct;
- integration rule: first guard fault → failsafe LAND with horizontal command zeroed
  ("degraded landing"); drift 24.7–24.8 m in G1/G2 is discussed as a design cost;
- GNSS freeze/jump inject only the horizontal part (altitude kept, as a barometer would);
- M0 reproduces F10-38 Log A exactly (same events at 187.12 s and 224.52 s; 179.6 m out);
- K1 (compass rotated 70°, field strength normal) is a declared known gap: no fault, the
  vehicle never got home within 400 s.

## Unverified boxes (2) and other honesty markers

1. Layer 2: what PX4/ArduPilot already implement (estimator checks, pre-arm checks, failsafes);
   the learner must read the documentation and source of the release built before R1.
2. Hardware section: no PX4/ArduPilot module API, parameter, topic, message, log field,
   SITL/HITL command, simulator, airframe or tool is named; no defaults.
- Safety box (university safety rules, F10 rules, LiPo, R3 before any flight; no rule details).
- "Watch out — untested on hardware" box.
- Sources D1–D5 title only / per learner / per kit (dossier gate G1 open); no URL anywhere.

## Glossary

`glossary.json`: 6 new four-part entries (Estimator health guard, Detection latency, Control
run (feature off), Known gap (test matrix), Degraded landing, Tethered hover). Existing terms are
linked, not redefined (SITL, HITL, Module (PX4), Innovation gate, Fault injection, Failsafe
escalation, Geofence, Safety case (flight operation), Go/no-go checklist, Evidence currency,
Flight log, Hazard log, Latched safe state, Design-review gate, Gate outcome, Independent
reviewer, Readiness review, Post-mortem, LiPo battery, Bench test, Negative control, Test ladder,
Compass interference).

## Validation

- `MP7.html`: html.parser balance clean; 54 ids, all prefixed `MP7-`, no duplicates; no URL; no
  `<script>` or event handler; every `src` tag resolves; all `data-src`/`data-run` files present.
- `python3 university/build/build.py`: no PROBLEM line mentions MP7 (one broken anchor,
  `#gl-module-px`, was found and fixed to `#gl-module-px4`). Remaining PROBLEM lines belong to
  other projects written in parallel (`gl-goal-area`, `gl-cut-list`, ...).
- Prose length: about 10,000 words including tables (L5 target "about 3,000 plus project
  material"); trimming candidates are the code walk-through tables.

## Decisions for the owner

1. **R0 for MP7.** The card lists gates R1, R2, R3, R4; the guide's general rule lists R0–R4.
   The handbook adds a short R0 (path, feature, hardware, supervisor, country). Keep, or merge
   R0 into R1.
2. **R3 placement.** The handbook holds R3 before *any* flight, including the tethered or netted
   hover (M4b), so M4 is split into M4a (props off, before R3) and M4b (after R3). Confirm.
3. **Rubric level descriptors** (4 levels per criterion) are this handbook's proposal; the
   weights are the card's. Proposal: level 1 in "safety process" fails the project. Confirm.
4. **Who signs.** Proposed: two independent peer reviewers at every gate, mentor where
   available, and the supervisor must sign R3. The guide leaves the choice to the owner.
5. **Supervision rules for real-hardware steps** are referenced, not defined: the owner must
   define who may supervise bench, tethered and outdoor tests (age, qualification).
6. **Worked feature.** The handbook works the "custom estimator check" example in detail; the
   mission-behaviour and companion-interface options are described more briefly.
7. **Planning weeks** in Figure 2 (16 weeks) are a proposal, not a rule.
8. **Analogy sub-mappings proposed** (F10, registered world "carrying a tray of drinks while
   walking"; DN401's cast Leila, Tomás, the head waiter reused): garden banquet = supervised
   outdoor test; rope handrail = tethered hover; Leila's own "stop when my senses disagree" rule
   = the learner's module; the head waiter's three conditions = R3.
9. **Real-tool run.** To move M1–M3 from "untested in this build" to "tested", schedule a build
   with the chosen stack's source and SITL installed, and a bench flight controller (AH-26).

## Sources

C1 guide (11.6, safety, F10 rules, course cards); C2 F12-28/F12-29; C3 DN401 chapters and
simulator; D1 PX4 Autopilot User Guide; D2 ArduPilot documentation; D3 MAVLink Developer Guide;
D4 national aviation authority's current rules (per learner); D5 kit datasheets (per kit);
R1–R6 lab runs of this build. No document was opened (dossier gate G1 open).
