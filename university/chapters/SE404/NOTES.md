# SE404 — Leading projects: author notes

Chapters F12-23 to F12-27, level L5, 3 credits. Prerequisites: SE301, SE402 (plus MA202 for percentiles, expected value and Bayes' rule). Maps to: Brooks ("The Mythical Man-Month"), Reilly ("The Staff Engineer's Path"); the course card names no curriculum milestone, so the chapters map to the curriculum's own process rules instead: section 0 (one milestone at a time, universal definition of done), 19.2 ("write the plan", "stop and ask … present options with consequences", "respect licences", never flash or write to undesignated hardware) and 19.4 (log entry with date and session length), and to MP8 (guide 11.6: milestones, gates R0–R4, rubric with "leadership and communication 20 %").

Labs are in `university/labs/F12-23` to `university/labs/F12-27`. All five pass `university/labs/run_lab.sh university/labs/<ID>` with exit status 0 (final sweep 2026-10-10, after the last change). `python3 university/build/build.py` reports no PROBLEM line for these chapters.

## Toolchain (from the logs)

- g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0, `-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined`, Linux x86_64 cloud build container. No other tool is used; no hardware, GPU or emulator is involved in this course.

## Listings run

All programs are the university's own small C++ tools. Every number they print comes from inputs that are **judgements of fictional teams or constructed teaching cases** (stated in each input file and in the chapter), or from a **seeded simulation** (F12-23 schedule, F12-27 benchmark trials). No real team, project, person or measurement is represented. Simulations use `std::mt19937` with fixed seeds; the triangular sampling is implemented by hand, so outputs are reproducible with this toolchain.

| Chapter | Run (`.log`) | Program | Exit | Status |
|---|---|---|---|---|
| F12-23 | schedule | schedule.cpp < schedule.in (Monte Carlo plan) | 0 | pass |
| F12-23 | schedule_narrow | schedule.cpp < schedule_narrow.in (check-yourself Q8) | 0 | pass |
| F12-23 | calibrate | calibrate.cpp < calibrate.in | 0 | pass |
| F12-23 | history | history.cpp < history.in (forensic evidence) | 0 | pass |
| F12-23 | history_key | history.cpp analyse < history.in (answer key) | 0 | pass |
| F12-24 | uncertain | uncertain.cpp < uncertain.in | 0 | pass |
| F12-24 | rewrite | uncertain.cpp < rewrite.in (forensic) | 0 | pass |
| F12-24 | bad_probs | uncertain.cpp < bad_probs.in (forensic draft) | 2 | expected-fail at run time (probabilities sum to 1.1; exit 2 is the program's documented refusal; run.sh checks for 2) |
| F12-24 | accuracy06 | uncertain.cpp < accuracy06.in (check-yourself Q7) | 0 | pass |
| F12-25 | mentor_log | mentor_log.cpp < mentor_log.in | 0 | pass |
| F12-25 | drift | mentor_log.cpp < drift.in (forensic) | 0 | pass |
| F12-26 | status_lint | status_lint.cpp < status_lint.in (engineer's draft) | 0 | pass |
| F12-26 | status_rewrite | status_lint.cpp < status_rewrite.in | 0 | pass |
| F12-26 | watermelon | watermelon.cpp < watermelon.in (forensic) | 0 | pass |
| F12-26 | watermelon_hl | status_lint.cpp < watermelon_headlines.txt (forensic) | 0 | pass |
| F12-27 | claim_lint | claim_lint.cpp < claim_lint.in (forensic report) | 0 | pass |
| F12-27 | bench_sim | bench_sim.cpp (simulated trials, not timings) | 0 | pass |
| F12-27 | release_draft | claim_lint.cpp < release_draft.in | 0 | pass |
| F12-27 | release_final | claim_lint.cpp < release_final.in | 0 | pass |
| F12-27 | claim_fixed | claim_lint.cpp < claim_fixed.in (answer key) | 0 | pass |

No listing is "untested on hardware": none needs hardware. No compile-time expect-fail listings.

## Claims that could not be verified (sources not opened in this build)

All tagged "title only — not opened during this build (dossier gate G1 open)":

- Brooks, "The Mythical Man-Month" (registry 4.9): F12-23 (people and months not interchangeable; adding people to a late project can make it later; onboarding and communication cost), F12-24 (planning for a first system to be discarded; second-system over-design), F12-25 (cost of new people learning from existing staff), F12-27 (conceptual integrity as the leaders' responsibility). Kept conceptual; no quotations; chapter/edition to be confirmed.
- Reilly, "The Staff Engineer's Path" (registry 4.9): F12-23 (planning and leading cross-team projects, stalled projects), F12-24 (decisions made in the open and written down), F12-25 (mentoring and sponsorship), F12-26 (stakeholders, alignment, visibility), F12-27 (senior engineers as examples). Each attribution is an outline from the author's memory of the book's scope; the Source Researcher must confirm each one or the sentence must be rewritten.
- "Site Reliability Engineering" (registry 4.9, SE402): F12-26 (incident communication role, post-mortem culture), consistent with SE402's use.
- ACM Code of Ethics and Professional Conduct; IEEE Code of Ethics: F12-27. **Not in the guide's source registry.** Proposal: add both to registry 4.9 for SE404 (tier: professional bodies' own codes; treat like tier 1 for "what the code says").
- The triangular distribution's inverse CDF and mean, nearest-rank percentiles, expected value, regret, Bayes' rule and the value of information are shown as arithmetic/definitions in the chapters and computed by the listings (no external source needed beyond MA202).

## Unverified boxes

1. F12-25 lab — the peer programme's safeguarding policy (named contact, approved places and online tools, informing parents/guardians, mentor checks, record storage and retention) is not defined anywhere in the guide or curriculum and depends on local law.
2. F12-27 Layer 2 — the outline of what the ACM and IEEE codes say (written without opening them).
3. F12-27 Layer 3 — legal protection for reporting concerns outside an organisation, regulated-profession duties and data-protection rules for robots/drones recording people (country-dependent; nothing stated).

## Decisions for the owner

1. **Safeguarding for the peer-mentoring lab (blocking for running the SE404 course lab).** Mentees in Years 1–2 may be children. The chapter states minimum course rules (shared visible places or approved online sessions; no private channels; a peer mentor is never the supervising adult for physical labs; same-day reporting to a named contact) and asks the owner to publish the programme's policy with the people responsible for child protection in the owner's organisation. Also: who signs the lab's verification item 5.
2. Add the ACM and IEEE codes of ethics to the source registry (4.9).
3. The status-colour rule (earned/planned: green ≥ 0.90, amber ≥ 0.75, red below) is presented as a course convention; confirm or replace it university-wide (it is also suggested for MP8 status reports).
4. Commitment percentiles (P50 for internal milestones, P80–P90 for the fixed-date MP8 defence) are presented as this course's recommendation for learner projects, not a standard; confirm.
5. Session-record fields and flag rules of `mentor_log.cpp` (F12-25) are the course's own; confirm they may be used in the peer programme and how records are stored (privacy of mentees).

## Analogy registry proposals (F12 — building a house as a team)

New mappings used (registered mappings for design doc, code review, post-mortem were reused where relevant; F12-03's proposed "reversible = where the table stands, irreversible = where the drain goes" is reused in F12-24):

- F12-23: estimate = how many weeks the builders say; range = "most likely fourteen, surely by seventeen"; critical path = foundation → walls → roof → painting; merge point = roofers waiting for walls and tiles; "90 % done" = bricks bought / hours spent; finished task = a room you can stand in when it rains; discovered work = the drain nobody drew. Breaks: software tasks are less repeatable; "done" is invisible without an acceptance test; discovered work changes finished work.
- F12-24: scenarios = firm or soft ground; spike = digging a test hole before choosing the foundation; value of information = what the hole is worth before digging; bad outcome from a good decision = a storm damaging a well-built house. Breaks: software spikes can be cheap and accurate; the future (load) depends on the design; reversibility can be designed.
- F12-25: mentor = experienced builder beside an apprentice; learner drives = handing over the trowel; specific feedback = "this joint is wider than the others"; reflection = the builder's evening notebook; sponsorship = recommending the apprentice for the next wall. Breaks: code hides faults a wall shows; mentee goals vary; peer mentoring has no hierarchy.
- F12-26: non-engineers = the family who will live in and pay for the house; status update = telling them in their words; watermelon status = a house painted outside with no plumbing. Breaks: users cannot inspect software; options are rarely clean; non-engineers are many audiences.
- F12-27: building codes and inspector = safety rules and reviews; wiring behind the wall = a defect nobody would see; telling the family = honest reporting; letter about the bad batch = a correction; neighbours = stakeholders without a voice. Breaks: software has no binding building code in most areas; defects copy at scale; software changes after delivery.

## Glossary

`glossary.json`: 31 entries. Qualified to avoid clashes: "Critical path (project schedule)" (vs HW102/HW202 critical path), "Calibration (estimates)" (vs MA202 Calibration), "Expected value (decision)" (vs MA202), "Escalation (raising a concern)" (vs SE402 "Acknowledgement and escalation" and SE301 "Escalation (review disagreement)"); each chapter mentions the other meaning once. Links used to other courses' entries: gl-median-and-percentile, gl-decision-matrix, gl-reversible-decision, gl-expected-value-mean-of-a-distribution, gl-bayes-rule, gl-risk-matrix-severity-likelihood, gl-code-review, gl-functional-safety, gl-threat-model, gl-calibration, gl-acknowledgement-and-escalation (all present at build time).

## Other notes

- Exam P (project plan defence) is prepared by F12-23's mini-project; the course project (MP8 team leadership) is fed by the decision log (F12-24), the mentoring guide (F12-25), the defence opening (F12-26) and the responsibility statement (F12-27).
- The card's forensic lab "The project that was always 90 % done" is F12-23's forensic lab; F12-26's forensic lab reuses the same project's data as its status history (cross-chapter continuity).
- The text tools (status_lint, claim_lint) are deliberately crude; their known false negatives ("races", "refactoring", "fully tested") and false positive (" to " in "added to be safe" counted as a range) are shown in the chapters as part of the lesson.
- Word counts (prose plus tables, excluding code and SVG, by a rough tag-stripping count): F12-23 ≈ 7,200; F12-24 ≈ 6,200; F12-25 ≈ 5,750; F12-26 ≈ 5,550; F12-27 ≈ 6,550 — within the L5 range of 3,000–8,000 plus project material.
