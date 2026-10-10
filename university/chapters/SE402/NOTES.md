# SE402 — Incidents and post-mortems (author notes)

Course SE402, levels L4–L5. Fragments: `university/chapters/SE402/F12-16.html` … `F12-19.html`.
Labs: `university/labs/F12-16` … `F12-19`. Glossary proposals: `glossary.json` (34 new terms,
generated from the chapters' jargon boxes; existing terms such as page and ticket, SLO, error
budget, burn rate, game day, trace and span, structured log, incident timeline and root cause
and contributing factor are linked, not redefined).

`university/labs/run_lab.sh university/labs/<ID>` exits 0 for all four labs (re-run 2026-10-10
after the last change). `python3 university/build/build.py` reports no PROBLEM line mentioning
F12-16…F12-19 or SE402, and no broken link to a target these chapters use. Every fragment passes
the local check (html.parser balance, id prefixes, no URL, no script, section order, source
anchors defined).

## Toolchain (as recorded in the logs)

g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0,
`-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined`.
No GPU or other hardware is needed by any listing; nothing is "untested on hardware".

## Listings and runs

| Lab | Run (`.log`) | What | Result | Status |
|---|---|---|---|---|
| F12-16 | gameday | game-day model, coordinated response (`gameday.in`) | exit 0 | pass |
| F12-16 | gameday_chaos | same fault, no roles (run.sh) | exit 0 | pass |
| F12-16 | gameday_reveal | timeline up to t+10 (run.sh, arg 10) | exit 0 | pass |
| F12-17 | triage | cohort comparison with confounder (seed 1) | exit 0 | pass |
| F12-17 | bisect | noisy bisection, 1000 trials | exit 0 | pass |
| F12-17 | odds | analytic prediction for bisect | exit 0 | pass |
| F12-17 | restart | forensic pack (restart "fixed it") | exit 0 | pass |
| F12-18 | incident_pack | forensic incident pack (kv store, disk full) | exit 0 | pass |
| F12-18 | pm_lint | linter on a bad draft: 20 findings | exit 1 | expected (intended: findings → exit 1) |
| F12-18 | pm_lint_revised | linter on reference post-mortem: 0 findings | exit 0 | pass |
| F12-19 | convert | wrapping / saturating / checked int16 | exit 0 | pass |
| F12-19 | cast_ub_default | UB cast, default flags: no diagnostic | exit 0 | pass (shows the gap) |
| F12-19 | cast_ub | UB cast + float-cast-overflow: runtime error reported | exit 0 | pass (diagnostic shown) |
| F12-19 | drift | 0.1 s truncation and float/double accumulation | exit 0 | pass |
| F12-19 | units | lbf·s read as N·s | exit 0 | pass |
| F12-19 | units_typed | strong unit types refuse the mix | compile failed as expected | expected-fail |
| F12-19 | uptime | forensic pack (gateway float uptime) | exit 0 | pass |

Real build failures during authoring (kept in F12-17/F12-19 mistakes tables): `convert.cpp`
without `<string>`; `drift.cpp` without `<initializer_list>`.

## Unverified boxes (Source Researcher, dossier gate G1)

No source was opened during this build; every D-source is "title only".

- **F12-16** (1 box): incident roles (IC, operations, communications, planning), "declare early",
  on-call load, attributed to "Site Reliability Engineering" (D1); Incident Command System
  origins (FEMA ICS material, D2). All numbers come from the model runs R1–R3.
- **F12-17** (1 box): hypothesis-driven troubleshooting (SRE book, D1); USE method (Gregg,
  "Systems Performance", D2); `git bisect` behaviour and options (git documentation, D3);
  confounding (D4).
- **F12-18** (1 box): post-mortem practice (SRE book, D1); hindsight, counterfactuals,
  accountability (Dekker, "The Field Guide to Understanding 'Human Error'", D2); Swiss cheese
  model (Reason, "Human Error", D3); five whys (Ohno, "Toyota Production System", D4);
  the term "gray failure" (Huang et al., "Gray Failure: The Achilles' Heel of Cloud-Scale
  Systems", named in the box, not in the source list).
- **F12-19** (2 boxes): (1) the summaries of Ariane 5 flight 501 (inquiry board report, D1),
  Mars Climate Orbiter (MIB Phase I report, D2), Patriot at Dhahran (GAO report, D3; check the
  report number, register width, ~100 h and ~0.34 s, compare with drift.out's 23-bit row) and
  Therac-25 (Leveson and Turner, D4) — all from memory, given only as a reading list;
  (2) compiler conversion-warning option names and coverage (`-Wconversion`, GCC/Clang manuals).
  Also title-only: C++ standard conversion subclauses (D5), IEEE 754 (D6), NIST SP 811 (D9).
  The float-cast-overflow behaviour is verified only for g++ 13.3.0 by runs R2/R2a.

## Decisions for the owner

1. **Analogy extensions (F12 world, house built by a team)** — only "post-mortem = fire drill"
   is registered. Proposed and used:
   - F12-16: a burst pipe on site with roles (one person directs, one shuts the water, one
     talks to the owner, one writes down) = incident roles.
   - F12-17: tracing a water stain to its leak by tests that split the possibilities =
     debugging under pressure, bisection.
   - F12-19: reading the published inspection report of another builder's sagging roof, and
     asking "do we build roofs like that?" = learning from documented failures, transfer.
   Please register or replace.
2. **Proposed registry sources:** Dekker (Field Guide), Reason (Human Error), Ohno (TPS),
   Gregg (Systems Performance), git documentation, FEMA ICS material, and the four
   investigation documents of F12-19 (ESA/CNES Ariane 501 board report, NASA MCO MIB Phase I,
   GAO Patriot/Dhahran report, Leveson & Turner IEEE Computer). Copyright: quote short
   excerpts only.
3. **Real game day on MP2/MP6 and bisection of a real repository** are learner activities,
   untested in this build (the labs use deterministic models). Decide whether a reference
   MP2 deployment should be provided for the timed game day and Exam P.
4. **Course project** split across F12-18 (part 1: post-mortem of a learner's own failure) and
   F12-19 (part 2: transfer analysis), with the default project rubric (guide 11.5).
5. **Exam P (incident simulation)**: `gameday.cpp` accepts other scenario files; examiners
   could write unseen ones. No examiner scenario was written.
6. **SE401 links**: SE401 chapters did not exist when these were written, so F12-17 links
   to the course anchor `#SE401` rather than to a chapter id; update when SE401 lands.
7. F12-19 links forward to `#F12-20`, `#SE403`, `#SE404` (existing per the curriculum).
