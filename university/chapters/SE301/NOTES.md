# SE301 Design documents and code review — author notes

Chapters F12-01 to F12-05 (L3–L4). Labs in `university/labs/F12-01` … `F12-05`; all pass with
`university/labs/run_lab.sh`. Toolchain: g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0, GNU diffutils 3.10,
Linux x86_64 cloud build container. No listing needs hardware, so nothing is "untested on hardware".

## Course card mapping

| Card item | Where |
|---|---|
| Ch 1–5 | F12-01 Blueprints before bricks · F12-02 Writing a design document · F12-03 Alternatives and trade-offs · F12-04 Giving code review · F12-05 Receiving code review |
| Lab: design docs for a driver | F12-01 (plan for the RX ring), F12-02 (receive-path design doc, checker), F12-03 (decision matrix) |
| Lab: peer review of real university project code | F12-04 (review of F4-04 `rtc.cc` / `rtc_decode.h`, read-only) and F12-05 (answer a review of your own change) |
| Forensic "The review that missed the bug" | F12-04 forensic lab (frame parser: array size 16 vs kMaxPayload 32, test 17 changed to 40) |
| Exam P (review a change, write comments) | F12-04 lab and check questions; comment format checked by `comment_lint.cpp` |
| Project (design doc for next project, two peer reviewers) | F12-05 mini-project, with rubric |
| Curriculum rule 10 | F12-01 (whole chapter), quoted from section 19.2 |

## Claims not verified (dossier gate G1 open)

- "Software Engineering at Google" (D1 in F12-01..05): title only; no content attributed. Unverified box in F12-01.
- ISO C++ standard (F12-01 D4, F12-02 D5 chrono), C++ Core Guidelines (F12-02 D4), GNU diffutils manual
  (F12-04 D4), Patterson & Hennessy (F12-03 D6): title only; claims are backed by runs instead.
- No numbers on review effectiveness (defect yield, change size, reading speed): unverified box in F12-04.
- F12-01 forensic: the replay with a fixed ring was predicted, not run (stated in the key).

## Listings and runs

- F12-01: `rx_ring` (0 of 5 plan checks failed), `rx_ring_noplan` (silent loss, drop counter 0).
- F12-02: `doc_lint` (9 findings, exit 1 expected), `doc_lint_v2` (0), `timeout_units`,
  `units_typed` (expected compile failure, line 16).
- F12-03: `decide`, `decide_team`, `decide_fixed`, `lookup_alt` (sanitizer build), `lookup_alt_O2`.
- F12-04: `comment_lint` (9 findings, exit 1), `comment_lint_v2` (0), `change_diff`, `v2_old_tests` and
  `incident` (ASan stack-buffer-overflow, exit 1 expected; trimmed, full reports in `*.full.txt`).
- F12-05: `ledger` (exit 1, not ready), `ledger_v2`, `frame_v3` (15 cases pass), `ledger_forensic`
  ("ready to merge"), `dev_diff`, `dev_v3`, `unplug` (exit 124 under `timeout 3`, expected).

## Decisions for the owner

1. F12-03 quotes timing numbers from `lookup_alt_O2.out` / `lookup_alt.out`; they are specific to the build
   container. Re-running the lab changes them and the prose must then be updated (the chapter says so).
2. The course forensic ("the review that missed the bug") lives in F12-04; F12-05 has a second forensic
   ("resolved, but not fixed").
3. The course project lives in F12-05's mini-project section.
4. F12-04 reviews F4-04's `rtc.cc` and `rtc_decode.h` by reference (`data-src="F4-04/..."`); those files
   are read-only for this course. If F4-04 changes, the line numbers in F12-04's comments must be checked.
5. The forensic scenarios (F12-04 robot frames, F12-05 data logger) are fictional; all evidence comes from
   real runs of the lab code. Commit ids in threads (8b41c07, e41c9b7) are story ids, not repository commits.
6. F12-05 links forward to F12-06 (SE302), a planned chapter at the time of writing.

## Proposed analogy mappings (F12, building a house as a team)

- Plan before code (rule 10) = the cutting list and order of work agreed before the first cut.
- Failure mode = "what if it rains before the roof is on?" written on the plan.
- Decision matrix = comparing three roof options on a scored sheet before buying.
- Reversible decision = paint colour (cheap to change) vs foundation (not).
- Review comment label = notes marked "must fix before plastering" vs "nice to have".
- Review response / ledger = the answer pinned under each note; the board of notes and answers.
- Fix with commit and test = the photo of the new lintel sent back to the reviewer.
- Breaks: review threads are kept for years; fixes can be tested, not only looked at; patterns are copied.
