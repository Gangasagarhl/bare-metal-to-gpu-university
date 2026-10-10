# MP8 The final integrated project — author's notes

Written by the Author / Lab Engineer agent for MP8 (mega-project batch). Status: draft handbook
with its starter lab run (G3 + G5); no dossier opened (no internet), no independent fact-check
(G6) yet. Level L5. The handbook builds on the SE501 chapters F12-28 (plan, contracts, walking
skeleton), F12-29 (gates) and F12-30 (defence, claims ledger, game-day, retrospective) and links
to them instead of repeating them. MP1–MP7 are linked by id only.

## Files

- `university/chapters/MP8/MP8.html` — the handbook (21 template sections adapted to a project;
  milestones M0–M8 in Layer 2; gates R0–R4 with checklists and signatories, deliverables and the
  rubric with level descriptors in the "Mini-project" section, because for a mega project the
  mini-project is the project itself; incident write-up template in the forensic lab).
- `university/labs/MP8/` — the starter lab (scaled-down skeleton of milestones M0–M2 for the
  running example).
- `university/chapters/MP8/glossary.json` — 5 new terms (Goal area, Stand-in backend, Fault
  matrix, Risk register (project), Cut list). Existing terms linked, not redefined.

## Running example

A kit robot whose obstacle-inflation kernel is the learner's own CUDA/HIP code (GPU area,
MP3/MP4) consumed by the MP6 robot stack (robotics area); the OS-image area (MP1 / RB403 image)
is on the example's cut list. Chosen to differ from SE501's example (drone ground station + Raft
store). Fictional team: Ines (GPU), Tomas (robot); reviewers Kofi and Mei.

## Reuse of the learner's earlier code (real, compiled from the other lab folders)

- `../F9-52/house.hpp` (simulated house) and `../F9-54/planner.hpp` (`rb::inflate`, the scatter
  reference) — included by `perception.hpp`.
- `../F6-05/cuda_check.h` — included by `inflate.cu`.
- `../F12-28/mp8plan.cpp` — compiled by `run.sh` to check `rover_plan.in`.
- `../F12-29/gatecheck.cpp` — compiled by `run.sh` to check `rover_r0.in`.
If any of these files changes, rerun `university/labs/run_lab.sh university/labs/MP8`.

## Listings run (`university/labs/run_lab.sh university/labs/MP8` from the repo root: exit 0)

Toolchain: g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0 with the sanitizer flags of run_lab.sh;
nvcc Cuda compilation tools release 12.0, V12.0.140; hipcc HIP 5.7.31921-0 (gfx90a). Linux x86_64
cloud build container, no GPU.

| Run | Program / input | Exit | Status |
|---|---|---|---|
| contract_tests | contract_tests.cpp | 0 | pass (16 of 16) |
| skeleton | skeleton.cpp < skeleton.in | 0 | pass (nominal) |
| skeleton_freeze | skeleton.cpp < skeleton_freeze.in (run.sh) | 0 | pass (fault injection) |
| skeleton_noage | skeleton.cpp < skeleton_noage.in (run.sh) | 1 | expected-fail at run time (forensic evidence; run.sh checks for 1) |
| rover_plan | ../F12-28/mp8plan.cpp < rover_plan.in (run.sh) | 0 | pass |
| rover_r0 | ../F12-29/gatecheck.cpp < rover_r0.in (run.sh) | 0 | pass (PASS WITH CONDITIONS) |
| inflate | inflate.cu (nvcc) | 1 | untested on hardware: built for real; run ends with cudaErrorNoDevice |
| inflate_hip | inflate_hip.hip (hipcc) | 1 | untested on hardware: built for real; run ends with hipErrorInvalidDevice |
| resources | ptxas -v (sm_80) and AMDGPU resource remarks (gfx90a) (run.sh) | 0 | compiled only; no GPU needed |

No compile-time expect-fail listings. All simulated times are simulated milliseconds, not
measurements. The skeleton's perception uses a perfect sensor (ground-truth map), stated in the
handbook as a limitation. Check-yourself Q5 (max_age 350) was run by the author during writing
(SAFE_STOP at 2,260 ms, stopped at 2,450 ms, budget 2,460 ms); it is not a recorded listing —
learners run it themselves ("predict, then run").

## Untested on hardware

- The CUDA and HIP kernels (M3 device comparison): no GPU in the container.
- Every real-hardware step of M3, M5 (timing on target), M7 (supervised real-robot run) and all
  R3 physical safety checks: taken from the guide's MP6/MP7 cards and safety rules; nothing
  physical was run.

## Unverified boxes / claims (need owner or Source Researcher)

1. Layer 2 unverified box: whether a GPU vendor runtime can run on an OS other than the vendor's
   supported ones (relevant to the card's example "computer runs the learner's OS image"). No
   vendor document was opened. The handbook suggests the RB403 Linux image as the realistic
   reading and leaves the decision to R0.
2. D3 (Shostack, "Threat Modeling") — title only, used only by reference to F11-14.
3. Contract C-grid v1 (values 0/100/255, integer cell_mm, max age) is the university's own
   teaching design; any resemblance to ROS 2 / Nav2 message or costmap conventions was not
   checked and is not claimed.

## Decisions for the owner

1. **Goal-area counting rule** (proposal): an area counts only when the learner brings a finished
   MP of that area whose acceptance tests pass from a clean checkout at R0. The card only says
   "at least two goal areas"; confirm or amend.
2. **Gate checklists, signatories and rubric level descriptors** are this handbook's proposal on
   top of the card's weights and SE501's gate rules (PASS / PASS WITH CONDITIONS / NOT YET; safety
   items never conditions; independent reviewers only). Proposed signatories: two independent
   peer reviewers (+ mentor where available) at R0–R3, one reviewer per goal area from R1, R3
   safety items signed in person by the supervisor the owner designates, R4 panel named by the
   owner. Confirm who may supervise real-robot/drone runs (supervision rules are the owner's).
3. **Pass without the real-hardware run**: the handbook applies the MP7 card's rule ("may pass
   without the outdoor flight if regulations or supervision do not allow it; the gate records
   why") to any MP8 real run. Confirm.
4. **Public defence format** (length, audience, recording) is left to the owner, as in F12-30.
5. **Mini-project section** holds gates, deliverables and rubric (the template has no dedicated
   section for them). Confirm this placement for all MP handbooks or name another.

## Analogy registry proposals (F12 — building a house as a team)

MP8 = the last house of the apprenticeship, built from trades the learner already mastered;
finished MPs = the trades; thermostat line between heating and electrics = interface contract
across goal areas; temporary heater on the same line = stand-in backend; list of drills = fault
matrix; fire drill with witnesses = game-day; handover walk-through = public defence (as F12-30).
Breaks (in the handbook): a house's trades do not disagree about time; a temporary heater is
visibly not a boiler, a stand-in can look identical at the contract and still differ on the
device; a house's drill does not change the house, software builds change daily.

## Known limitations

- The handbook is long for L5 prose (about 7,500 words of text outside code, tables and figures)
  because it carries the project material (milestones, gates, rubric, templates).
- The skeleton's controller drives along one straight lane; enough for one contract and one
  fault, deliberately not a navigation stack (that is M4 with MP6's stack).
