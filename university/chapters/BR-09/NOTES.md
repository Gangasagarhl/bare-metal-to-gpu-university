# BR-09 From hobby code to production — author notes

Bridge chapter, L3–L4, placed before SE301 (prerequisite of F12-01, whose front matter
already lists BR-09). Build date 2026-10-10, Linux x86_64 cloud build container.
Toolchain as printed in the logs: g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0, Ubuntu
clang version 18.1.3 (1ubuntu1), cmake version 3.28.3, gcov 13.3.0, Python 3.13.16,
git version 2.43.0, ldd (Ubuntu GLIBC 2.39-0ubuntu8.9).

## Files

- `university/chapters/BR-09/BR-09.html`: all 21 template sections in order, plus
  "Answers to Check yourself" and a forensic answer key (`BR-09-forensic-key`); 2 inline
  SVG figures (Figure 1: before/after side by side; Figure 2: the life of one change);
  claim tags on technical statements (all resolve; every source is cited); Transition box;
  3 unverified boxes. Validated: html.parser balance, all ids prefixed `BR-09-`, no URLs,
  no `<script>` or event handlers, section ids in order. `build.py` reports no PROBLEM
  line for BR-09 (the one remaining PROBLEM line, a duplicate glossary id gl-finite-state-machine-fsm, belongs to other courses).
- `glossary.json`: 17 new terms (Production software, Failure path, "Works on my machine",
  Unspecified order of evaluation, Undocumented assumption, Semantic versioning, Breaking
  change, Source compatibility, Changelog, Known-issues list, Software licence, SPDX licence
  identifier, Third-party record, Dependency inventory, Security update, Error-rate alert,
  Exposure). Existing terms are linked, not redefined: Code review, Review checklist,
  Interface contract (SE301); CI, Version control, Tag, Commit, Regression test, Code
  coverage, Sanitizer, Build type, Warnings as errors, Test harness (SP202); Precondition,
  Unit test (SP101); Exit status (OS201); NaN and infinity (MA302); Golden master (SE403);
  ABI (SP301); Vulnerability (SS401); Blast radius, SLO, burn-rate alert (DS402); On-call
  rotation, Runbook, Post-mortem (SE402); CI pipeline, Hermetic test (SE302).
- Lab `university/labs/BR-09/`: `exposure.cpp` (built by run_lab.sh), `monitor.cc`,
  `fleet_inputs.txt`, `run.sh` (all other steps), `prod/` (the production-grade project,
  version 1.0.0), `traps/` (wom_order.cc, wom_order_fixed.cc, wom_path.cc, fast_median.cpp).
  The earlier project is read from `university/labs/F2-26/template` (read-only; not modified).

## The "earlier project"

The lab's earlier project is the SP202 course project, i.e. the F2-26 template
(`stats` library + `stats_cli`), labelled version 0.1.0 here. `prod/tests/uni_test.h` is a
byte-identical copy of F2-26's harness (SHA-256 recorded in `prod/THIRD_PARTY.txt`);
`prod/tests/golden_v0_1.txt` is real 0.1.0 output, regenerated and compared with `cmp` on
every lab run (step `golden_record`). If F2-26's template changes, these two checks will
show it.

## Listings run (`run_lab.sh university/labs/BR-09`, runner exit 0, about 35 s)

| result | steps |
|---|---|
| pass, exit 0 (16) | exposure, docs, before_failpaths (contains the two exit-134 crashes as evidence), before_coverage, regex_ignores_exit, wom_order (g++ FAIL / clang PASS recorded as evidence), wom_order_fixed, wom_path (second run fails as evidence), golden_record, ci, after_failpaths, ctest_list, compat, deps, monitor_v0, monitor_v1 |
| expected non-zero, checked by run.sh (5) | clang_asan_missing (link failure: clang sanitizer runtime not installed), compat_break (planted breaking header, compile errors), licence_scan (findings), release_check_bad (version mismatch), review (quick fix: findings, and its failing unit test) |
| untested on hardware | none (no hardware involved) |

Normalised in outputs: work-folder paths (`<work>`, `<lab>`, `<F2-26 template>`), bash's
`Aborted` pid, and `ldd` load addresses. git runs use fixed author/dates and an isolated
HOME, so the commit ids in `review.out` are reproducible for identical sources (they
change whenever any file in `prod/` changes; the chapter quotes none of them).

Untested in this build: clang with sanitizers (runtime missing, recorded as R5); any OS or
CPU other than Linux x86_64 (KI-3); the CTest `WORKING_DIRECTORY` fix for trap 2 was
explained, not run as a separate step; Check yourself question 5's variation was run once
by the author outside the recorded steps (stated in the answer).

## Corrections found while building (honesty record)

- First hypothesis was that CTest's `PASS_REGULAR_EXPRESSION` test would pass even if the
  program **aborted** after printing. The run showed CTest reports "Subprocess aborted" and
  fails the test; only a wrong **exit status** passes. The lab step, the chapter and the
  comment in `prod/tests/expect.sh` were corrected to match the run (R3).
- CMake 3.28.3's own `WILL_FAIL` help text misspells the property as `WILL_FALL`; quoted
  as printed.

## Claims not verified (dossier gate G1 open)

- D1 "Software Engineering at Google", D2 "Site Reliability Engineering": title only;
  only conceptual claims (review shares knowledge, actionable alerts, on-call duties).
- D3 ISO C++ (argument evaluation order, `std::sort` strict weak ordering, `from_chars`,
  `midpoint`), D4 GCC/Clang manuals, D9 IEEE 754: title only; the behaviours used are
  also shown by runs.
- D6 Semantic Versioning 2.0.0 and D7 SPDX specification / License List: title only and
  **not in the guide's source registry** — proposed additions for registry 4.9 (F12).
  Unverified box in Layer 3 (versioning) and Layer 3 (licences).
- D8 licence texts: title only; the chapter states no licence's terms and gives no legal
  advice (unverified box).
- Monitoring threshold (5 %) and the exposure scenario numbers are the lab's story
  choices, labelled as such; no response times or industry thresholds are claimed
  (unverified box in Layer 3 "Operating it").

## Decisions for the owner

1. **The university's licence for lab code.** `prod/` uses the placeholder SPDX id
   `LicenseRef-Uni-Lab` and `licences.allow` contains only that. The owner should choose
   the real licence(s); then replace the placeholder in prod/ (and consider SPDX lines in
   all lab code).
2. **Maintainer / response promise** in `prod/README.md` is left "to be filled in by the
   owner"; no on-call arrangement is invented.
3. **Source registry:** add Semantic Versioning 2.0.0 and the SPDX specification + License
   List to registry 4.9 (F12), or tell authors to drop them.
4. **Analogy mappings (F12, building a house as a team) proposed for registration:**
   hobby code = a shed you build for yourself; production = a house a family lives in;
   CI = the inspector's checklist run on every change; release notes/changelog = the
   handover letter; known-issues list = the snag list; on-call = the builder's phone
   number; monitoring = the damp meter; security update = a hinge recall; dependency
   inventory = which batch of hinges went into which house; licence = conditions written
   on borrowed drawings; exposure = nights under a leaking roof. Breaks are in the chapter.
5. **Length:** about 11,000 words outside code and tables (including jargon box, forensic
   lab, answers and sources), above the L4 aim of 5,000 for prose; the bridge card asks
   for ten "changes" and four traps in depth, each with a run. The editor may move the
   Layer 3 subsections into SE301/SE302 cross-references if the page must be shorter.
6. Figure 1 and the forensic scenario are fictional framings (Lan, Ines, Kofi, Sipho's
   team, a "pasted" `fast_median.cpp` written for the lab); every number in the evidence
   comes from the recorded runs.
7. Check-yourself question 8 and the lab extension leave the `E-INTERNAL` branch of
   `prod/app/main.cpp` untested on purpose (a teaching point); `monitor.cc` keeps
   `std::stod` on its threshold argument on purpose (question 9).
