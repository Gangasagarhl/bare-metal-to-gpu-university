# SS402 Functional safety — author's notes

Chapters F11-18 to F11-23 (L4–L5). Each has every template section, an "Answers to Check yourself" section with the forensic answer key as an h3, inline SVG figures, claim tags and a Transition box. Every listing and output was produced by `university/labs/run_lab.sh` in this build. The build container has g++ 13.3.0 with AddressSanitizer and UBSan, and nothing ran on hardware.

## Honest scope
This course teaches concepts only. F11-18 states, as the course card requires, that certification needs the purchased standards, qualified assessors and an organisation's processes. The other chapters repeat the point where it matters (F11-20 integrity levels, F11-23 the safety case). The course scale "CT0–CT3" (course targets) is the university's own. It is not a SIL, ASIL or DAL, and the chapters say so.

## Sources
- None of the standards was opened (dossier gate G1 is open; AH-6). Each one is cited "Title only — not opened during this build":
  - IEC 61508 (D1)
  - ISO 26262 (D2)
  - DO-178C (D3)
- Every statement about the content of a standard is in a "Not verified" box. That includes:
  - definitions
  - SIL, ASIL and DAL tables and bands
  - PFD formulas as given in IEC 61508-6
  - MC/DC requirements per level
  - the work products required for a safety case
  - independence requirements
- These sources are cited but are not in the guide's registry. I propose adding them:
  - D4: ISO 12100 (machinery risk assessment) and sector standards for robots (ISO 10218, ISO 13849, IEC 62061, ISO 13482), named as reading references only.
  - D5: IEC 60812 (FMEA), IEC 61025 (fault tree analysis), IEC 61882 (HAZOP).
  - D6 (F11-23): the GSN Community Standard (Assurance Case Working Group) and OMG SACM.
  - Leveson's STPA, which F11-19 mentions as an alternative method. Title only.
- Fixed numbers inside the labs (speed limits, failure rates, proof-test intervals, β) are illustrative. The chapters label them that way, and none of them are hardware data.

## Unverified boxes, by chapter
- **F11-18:**
  - the definitions of harm, hazard, risk, functional safety, EUC and safety function as worded in IEC 61508 and ISO 26262
  - the safety lifecycle phases
  - an "untested on hardware" box for the arm simulation
- **F11-19:**
  - the risk-graph parameters and calibration in IEC 61508-5
  - ISO 26262 S/E/C classes
  - FMEA/FTA/HAZOP per the IEC standards
  - the course risk graph, which is the university's own
- **F11-20:**
  - the SIL bands for PFDavg and PFH
  - the ASIL determination table
  - the DO-178C levels A–E and their objective counts
  - the simplified PFD formulas as given in IEC 61508-6
  - the claim that integrity levels are not comparable across standards
- **F11-21:**
  - diagnostic-coverage and safe-failure-fraction concepts
  - the fault-tolerant time interval terms in ISO 26262
  - watchdog requirements
  - the timing model, which is untested on hardware: one CPU, no interrupts or caches
- **F11-22:**
  - the DO-178C coverage objectives per level, and MC/DC as defined there (the course uses the unique-cause form)
  - independence requirements
  - ISO 26262 requirements on verification and confirmation measures
- **F11-23:**
  - which standards require a "safety case" and under what name
  - the GSN and SACM publishers
  - where the cited definition of a safety case comes from
  - an "untested on hardware" box: G32 and G7 are open goals

## Listings run (all pass, rc=0, deterministic)
| Lab | Listing | Result |
|---|---|---|
| F11-18 | safety_function (3 scenarios), forensic (release 0.3 monitor on encoder A) | pass; forensic shows the HAZARD it is meant to show |
| F11-19 | hazard_log (course log: 0 findings), fault_tree, forensic (team 4: 4 findings) | pass |
| F11-20 | pfd_model (formula vs Monte Carlo, fixed seeds, N=1e6), forensic | pass |
| F11-21 | voter, watchdog_test (all mutants killed), estop_timeline (comms-thread e-stop, forensic case "The arm moved after e-stop"), estop_fixed | pass |
| F11-22 | trace_check (1 gap: T1 needs hardware), mcdc, forensic (3 gaps) | pass |
| F11-23 | case_check (0 defects, 2 open goals), forensic (4 defects) | pass |

No listing is meant to fail when it runs (no expected-fail runs). The "failing" verdicts in forensic outputs are program output from runs with exit code 0. F11-23 reads the committed `.out` files of F11-18 to F11-22, so run those labs first after any change. Nothing ran on hardware. Every claim about real robot hardware is in an "untested on hardware" box or is an open goal.

## Analogies
- Registered (F11): functional safety = seatbelts and airbags, designed and tested to rules. Where it breaks: certification needs evidence, not only mechanisms.
- Proposed sub-mappings, for the owner to register or reject:

| Analogy | Concept | Chapter |
|---|---|---|
| crash list | hazard analysis | F11-19 |
| go-kart brake vs school-bus brakes | integrity level | F11-20 |
| train driver's vigilance button | watchdog | F11-21 |
| two brake circuits | redundancy | F11-21 |
| air brakes that clamp when pressure is lost | fail-safe | F11-21 |
| airbag warning light | diagnostics | F11-21 |
| batch label | traceability | F11-22 |
| inspection folder plus the inspector's sheet | safety case | F11-23 |

## Glossary
`glossary.json` has 54 terms.
- The pipeline "Hazard" of HW202 would collide with this course's hazard, so the term here is "Hazard (functional safety)". F11-18 says that the word is not the pipeline hazard of F1-26.
- "Safe state" is shared with RB403. build.py merges it, and RB403's definition wins because RB403 comes first alphabetically. SS402 only adds F11-18 and F11-21 to its chapter list. The definitions agree.
- Terms owned by other courses are linked only, not redefined:
  - Controlled stop, Watchdog (software), Kick source, Latched safe state (RB403)
  - Emergency stop, Latched (RB101)
  - Watchdog timer (HW204)
  - Fault and failure (DS301)
  - Real-time system (RB304)
  - Mutation testing (DR401)
  - Regression test (SP202)
  - Reproducible build

## Decisions for the owner
1. **Course target scale.** Approve or replace the course risk graph (points = 2S+E+A; CT0 ≤7, CT1 8–9, CT2 10–11, CT3 12–13) and its CT rules:
   - CT1 needs its own input, a proof test, tested requirements and a second-learner review.
   - CT2 adds a diagnostic, a PFD argument, traceability on the release build, MC/DC on the decision logic, mutation tests and a reviewer from another team.
   - CT3 is never built by learners and must be designed out.
2. **Course robot safety case.** The case keeps two goals open:
   - G32: the hard-wired e-stop chain timed on the real robot.
   - G7: teach-mode pinch, which is the F11-23 mini-project.

   F11-22's trace keeps T1 (hardware) as a gap on purpose. MP6's R3 gate is the natural place to close them. Please confirm that R3 should require this.
3. **New sources.** Add D4 to D6 and STPA (listed above) to the registry, or tell me to remove them.
4. **Link to BR-08.** F11-22 links to BR-08, which is not written yet. Check that the catalogue entry resolves it, or remove the link.
5. **Practical exam.** The P exam (safety-case review) uses the checklist and rubric weights in F11-23. The exam cases and keys still need to be written and kept outside the learner pages.
6. **MP7 regulation.** The MP7 project brief tells learners to cite their national aviation authority's current rules from the authority's own document. No regulation is stated in the chapters.
