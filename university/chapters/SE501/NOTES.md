# SE501 Capstone studio — author's notes (F12-28, F12-29, F12-30)

Written by the Author / Lab Engineer agent for SE501 (batch for L5). Status of all three
chapters: draft with listings run (G3 + G5); no dossier was opened (no internet), no
independent fact-check (G6) yet.

## Sources actually opened in this build

- `UNIVERSITY_AUTHORING_GUIDE.html` (local): sections "Honesty first", 2, 3 (AH rules),
  5.17 (SE501 card), 6, 7, 8 (F12 analogy world), 9, 10, 11 (in particular 11.6 mega
  projects and the MP8 card), 12.2–12.3, 13. Cited as D1 in every chapter, as this
  project's own rules (AH-4), not as a technical source.
- `SYSTEMS_CURRICULUM.html` (local): section 0, 13.4, 19.2–19.4. Cited as D2.
- Course card "Maps to: All": there is no single curriculum milestone; the chapters use
  the curriculum's universal rules (milestone rhythm, acceptance tests, definition of
  done, log template, measurement protocol) and the MP8 card's milestones verbatim.

## Title-only sources (dossier gate G1 open)

- F12-28 D3 "The Mythical Man-Month" (Brooks); D5 "Software Engineering at Google"
  (Winters, Manshreck, Wright, eds.). Used only as "where a dossier should look"; no
  claim rests on them alone.
- F12-30 D4 "Site Reliability Engineering" (Beyer, Jones, Petoff, Murphy, eds.): the
  meaning of "blameless" post-mortem (one sentence, kept conceptual). D5 the Raft paper
  (Ongaro, Ousterhout): cited only as where DS302/MP2 study elections; no Raft behaviour
  is stated.

## Unverified boxes (need owner approval, AH-19)

1. F12-28 Layer 2: "walking skeleton" and "contract test" are this course's own
   definitions; no source opened defines them or attributes them.
2. F12-28 Layer 3: the ratio-based re-forecast of Listing 1 is a model with stated
   assumptions; its predictive accuracy is not established by any source.
3. F12-30 Layer 3: how Raft (or any real protocol) guarantees that a new leader holds
   every acknowledged/committed write is not stated; the paper was not opened.

## Untested on hardware

- F12-29 "How the hardware actually does it" and F12-30 "How the hardware actually does
  it": R3 physical safety checks and any live hardware demonstration are taken from the
  guide's MP6/MP7 cards and safety rules; nothing physical was run (no robot, drone,
  board or GPU in the container). Marked with "Watch out" boxes.
- All listings are host C++ planning/teaching tools; no listing needs hardware.

## Listings run (all with `university/labs/run_lab.sh university/labs/<ID>`, g++ 13.3, sanitizers)

| Chapter | Run | Exit | Note |
|---|---|---|---|
| F12-28 | mp8plan (mp8plan.in) | 0 | worked example |
| F12-28 | mp8plan_team (run.sh) | 3 | forensic evidence: problems found by design |
| F12-28 | mp8plan_fixed (run.sh) | 0 | answer key |
| F12-29 | gatecheck (gatecheck.in) | 0 | R2 worked example, PASS WITH CONDITIONS |
| F12-29 | gatecheck_r3team (run.sh) | 1 | forensic evidence: NOT YET by design |
| F12-29 | gatecheck_r3after (run.sh) | 0 | answer key, PASS |
| F12-30 | gsdemo (gsdemo.in) | 0 | demonstration script, both checks pass |
| F12-30 | claims (claims.in) | 1 | first ledger, 3 unbacked claims by design |
| F12-30 | gameday (run.sh) | 1 | forensic evidence: check A1 fails by design |
| F12-30 | gameday_fixed (run.sh) | 0 | fix verified |
| F12-30 | claims_fixed (run.sh) | 0 | revised ledger |

No expected-fail compiles. Every non-zero exit is a deliberate, recorded program result
(the build's `run_ok` accepts them because the log has an `exit code:` line).
F12-29's forensic lab also shows `F12-30/gameday` output (same course, cross-lab reference).

## Decisions for the owner

1. **Analogy mappings (F12, building a house as a team) proposed for registration** — not
   in guide 8.1 yet:
   - mega project = a whole house built by several trades; component = one trade's work;
     interface contract = agreed position and size of openings where pipes and cables
     cross; contract test = checking the hole with the agreed pipe before the plumber
     comes; walking skeleton = one finished room with water, power and a door; hidden
     work = moving a pipe nobody wrote on the job list (F12-28).
   - design-review gate = building inspection at a fixed stage; entry criteria = walls
     still open for the wiring inspection; pass with conditions = "move in, railing fixed
     by Friday"; not yet = "call me when the alarm works"; safety item = the smoke alarm
     (F12-29).
   - final defence = handover walk-through; demonstration script = the walk-through
     route; recorded fallback = folder of inspection records; claims ledger = promises
     with certificates; stated limitation = "the attic is not insulated yet" (F12-30).
2. **Gate content is a course proposal.** The guide fixes only the gate names, written
   approval, and the MP3/MP6/MP7 examples. The per-gate entry criteria, the three
   outcomes (PASS / PASS WITH CONDITIONS / NOT YET), "safety items are never conditions"
   and "only independent reviewers count" are this course's rules. Please confirm or
   amend; they should then go into guide 11.6.
3. **Running example.** All three chapters use one MP8: a drone ground station backed by
   a replicated store (MP7 + MP2, one of the card's examples), with team names Amara,
   Joon, Farah. The store in F12-30 Listing 1 is our own deterministic teaching model,
   explicitly not Raft.
4. **Measurement threshold.** F12-30 Listing 2 uses 20 runs (the worked example's R1
   plan, after curriculum 13.4's GPU measurement protocol). Learners are told to use
   their own plan's number.
5. **Defence format** (length, audience, who examines) is not fixed by the guide; F12-30
   gives only an order of parts (Figure 2) and leaves lengths to the reviewers.

## Glossary notes

- 21 new four-part entries in `glossary.json`, all with chapter-specific or qualified
  names to avoid clashes.
- Deliberately NOT defined here (owned by other courses, referenced by chapter link):
  critical path, three-point estimate, slack (SE404, F12-23); post-mortem, game-day
  (SE402, F12-16..19); design document (SE301, F12-02); acceptance test (curriculum).
  Linked to `gl-threat-model` (SS401) only, which exists.

## Known limitations

- Chapters are long for L5 (about 6,000–7,000 words of page text each including
  tables, answers and keys; prose is nearer the 3,000–4,000 aim).
- Listing 1 of F12-28 assumes unlimited parallel people; the chapter says so and gives
  the resource-dependency workaround.
