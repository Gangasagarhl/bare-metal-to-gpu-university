# KID101 How computers think — author and lab-engineer notes

Author/Lab Engineer run for batch 1 (Foundation year), 2026-10-09. Sandbox without internet:
no document was opened. Every book source is marked in the chapters as "title only — not
opened during this build; the Source Researcher must confirm the edition and section (dossier
gate G1 open)". Claims shown by arithmetic in the text, or printed by our own programs, are
tagged R1 (lab run in this build).

Toolchain for every listing (from the `.log` files): `g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0`,
`-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined`, Linux x86_64
(cloud build container). All ten folders were re-run with `university/labs/run_lab.sh` after
the last change: every folder returned 0.

## Files

- Chapters: `F0-01.html` … `F0-10.html` (all 21 sections plus "Answers to Check yourself";
  forensic answer keys inline under "Forensic lab answer key").
- `glossary.json`: 50 four-part entries; each entry also carries the `anchor` used in the pages.
- Labs: `university/labs/F0-01` … `F0-10` (sources, `.in`, `.timeout`, `.out`, `.log`).

Validation (python `html.parser` script, kept outside the repo): balanced tags, every id
prefixed with the chapter id, no duplicate ids, no URLs, no `<script>`/`<style>`/event
handlers/inline styles, the 21 headings with the exact text and order of FRAGMENT_FORMAT.md,
only the seven callout classes and the `sv-*` SVG classes, no colour attributes in SVG, no
`<marker>`, every `data-src`/`data-run` resolves to a file, no phrases from guide 13.8.
All ten fragments pass.

Prose words (from the hook to the end of "Next chapter" before Sources, excluding code,
SVG and tables; answers and sources excluded): F0-01 2192, F0-02 2290, F0-03 2186,
F0-04 2257, F0-05 2408, F0-06 2033, F0-07 2227, F0-08 2300, F0-09 2270, F0-10 2493.
Average sentence length measured by the script: 11.1–13.8 words (it counts headings glued
to the following sentence, so the real figure is a little lower). The Editor still needs to
do the read-aloud test (guide 10.4).

## Listings run (all pass; exit codes from the logs)

| Folder | Listing | Use in chapter | Result |
|---|---|---|---|
| F0-01 | recipe.cpp | Listing 1 | exit 0 |
| F0-01 | sandwich_good.cpp + .in, sandwich_bad.cpp + .in (both include literal_cook.hpp) | forensic evidence | exit 0 |
| F0-02 | count_switches.cpp | Listing 1 | exit 0 |
| F0-02 | bit_board.cpp + .in | Listing 2 | exit 0 |
| F0-02 | forensic_board.cpp + .in (byte-identical copy of bit_board.cpp) | forensic evidence | exit 0 |
| F0-03 | letters.cpp, decode.cpp + .in, alphabet.cpp | Listings 1–3 | exit 0 |
| F0-03 | decode_bad.cpp + .in (line 7 reversed) | forensic evidence | exit 0 |
| F0-04 | network.cpp | Listing 1 (24 of 24 orders sorted) | exit 0 |
| F0-04 | network_bad.cpp (last comparator removed) | forensic evidence (8 of 24) | exit 0 |
| F0-05 | kitchen.cpp | Listing 1 | exit 0 |
| F0-05 | kitchen_bad.cpp (last two steps swapped) | forensic evidence | exit 0 |
| F0-06 | night_light.cpp + .in | Listing 1 | exit 0 |
| F0-06 | night_light_bad.cpp + .in (`<` → `>`) | forensic evidence | exit 0 |
| F0-07 | stir_and_fill.cpp, add_rules.cpp (256 sums, 0 wrong) | Listings 1–2 | exit 0 |
| F0-07 | fill_forgot.cpp + .timeout (3 s) | forensic evidence | exit 124 by design (time limit) |
| F0-08 | count_bug.cpp, count_fixed.cpp | Listings 1–2 | exit 0 |
| F0-08 | robot_route.cpp + .in (includes grid_robot.hpp) | forensic evidence | exit 0 |
| F0-09 | postcards.cpp | Listing 1 | exit 0 |
| F0-09 | postcards_glue.cpp | forensic evidence | exit 0 |
| F0-10 | robot.cpp (includes robot_world.hpp) | Listing 1 | exit 0 |
| F0-10 | robot_stale.cpp | forensic evidence | exit 0 |

Real failure seen while building (used honestly in F0-08 Layer 2): the first version of
`F0-09/postcards.cpp` (message of 32 characters = 4 cards, arrival order naming card 5) was
stopped by AddressSanitizer with a heap-buffer-overflow report. Fixed by a 37-character
message, `at()` instead of `[]`, and cards that carry "n of total". That first output was
not kept, so the chapter describes it without quoting it.

`fill_forgot.out` is timing-dependent (the number of "still not full" lines in 3 s); the
chapter says so in its Sources entry.

## Unverified boxes (AH-18; need owner approval to publish, AH-19)

- F0-01: none.
- F0-02: "billions" of two-state parts is a sense of scale, not a measured number.
- F0-03: samples per second of real recordings and bytes per pixel of real image files (none given).
- F0-04: the names "selection sort" and "sorting network" and any claim about which methods
  real programs use: no algorithms textbook in the F0 source registry.
- F0-05: register counts, cache sizes and relative speeds (none given; learner may write own number).
- F0-06: none.
- F0-07: none.
- F0-08: none.
- F0-09: real packet sizes and loss rates (none given).
- F0-10: real control-loop rates and motor/battery ratings (none given).

## Claims that rest on unopened books (Source Researcher, gate G1)

Per chapter, the D-sources and what they are used for are listed in each chapter's Sources
section. The claims most worth checking first:

- F0-03: that Petzold's "Code" (edition to be recorded) covers Unicode, RGB colour of
  pixels and sound sampling; if the edition does not, another registry source is needed.
- F0-03: "the C++ rules do not promise consecutive numbers for letters" (cited to "A Tour of
  C++"; the ISO C++ working draft is the better source — the standard guarantees this only
  for the digits 0–9).
- F0-05: DRAM loses its contents without power; caches fill by hardware policy; programs
  are loaded from storage into memory (Bryant and O'Hallaron).
- F0-07: conditional jumps implement loops and decisions; adder circuits add column by
  column with a carry (Nisan and Schocken).
- F0-08: D3 is the GCC documentation for `-fsanitize=address`, which is not in the guide's
  registry; add it or replace it.
- F0-09: IP does not guarantee delivery; TCP numbers, reorders and retransmits; routers may
  drop packets; packets can be duplicated (Stevens, "TCP/IP Illustrated"); a waiting computer
  cannot tell a lost message from a slow one (van Steen and Tanenbaum).
- F0-10: robot definition and sense–decide–act cycle (Lynch and Park); motor drivers
  (Platt); multirotor flight controller adjusting motor speeds (Beard and McLain); noisy and
  drifting sensors (Thrun, Burgard, Fox).

## Decisions left to the owner or the Dean

1. **Analogy registration (guide 8.2).** F0-05 uses the registered F1/F2 restaurant-kitchen
   map (head chef, hands and cutting board, shelves near the stove, big pantry down the
   corridor, warehouse across town, corridors and service lifts). It adds two mappings that
   are not in the registry: **input = order slips through the order window** and **output =
   plates out through the serving hatch**. The Dean should register them or choose others.
   F0-06 and F0-10 use "senses / voice / muscles" for input devices, output devices and
   actuators, consistent with the registered F9 "sensors / actuators = senses / muscles".
2. **Algorithms textbook.** The F0 source registry has none; F0-04 needs one (its D2 is a
   placeholder entry asking the Dean to choose).
3. **Glossary overlaps for the Integrator (10.3).** These KID101 terms are also proposed by
   other courses: Bit, Binary, Place value, Byte (MA102); Robot, Sensor, Actuator, Simulator,
   Motor, Program, Loop, Condition, Control loop (RB101); Simulator, Sensor, Motor, Control
   loop (KID103). KID101 is a prerequisite of those courses, so its chapters should be listed
   first as the defining chapters; the wording must be merged into one definition per term.
4. **Builder behaviour to confirm.** Forensic evidence code is shown with `data-src`
   (F0-07 `fill_forgot.cpp`, F0-10 `robot_stale.cpp`) without a line-by-line table, because
   it is evidence, not a teaching listing. The three small simulators
   (`F0-01/literal_cook.hpp`, `F0-08/grid_robot.hpp`, `F0-10/robot_world.hpp`) are compiled
   with the listings but not shown in the chapters; the chapters say where they are.
5. **Cross-links** point to chapters and courses of other agents' batches (`#KID102`,
   `#F0-29`, `#F0-32`, `#F0-33`, `#F0-36`, `#F0-37`, `#MA101`, `#MA102`, `#KID103`, `#RB101`,
   `#HW101`, `#HW102`, `#DS201`), and to `#safety` and `#analogies`; the Integrator should
   confirm they resolve.
6. **Course-level items not in the chapter files.** The course exam (Q per chapter is covered
   by each chapter's "Check yourself"; the practical "write an algorithm another person can
   follow without asking questions" is practised in F0-01 and F0-08) and the course project
   (the paper binary adder: started in the F0-02 mini-project, rules and lab in F0-07, rules
   verified for all 256 four-bit pairs by `add_rules.cpp`) still need the Exam Writer's
   separate files and rubric in `_keys/`.

## Owner rulings applied

Verification pass, 2026-10-10. Rulings from `university/OWNER_RULINGS.md`.

1. **Analogy registration (F0-05 order window / serving hatch; F0-06 and F0-10 senses, voice and muscles)** — ruling **A3**: approved; registered in the "Analogy registry additions" of `UNIVERSITY.html` by the integrator. No chapter change needed.
2. **Algorithms textbook for F0-04** — ruling **C2**: the verifier opened the book site of Sedgewick and Wayne, "Algorithms, 4th Edition" (sections "1. Fundamentals" and "2.1 Elementary Sorts"). It is now F0-04's D2 and F0-01's D4. The names "selection sort" and "sorting network" are confirmed there (§2.1 and its web exercises), so F0-04's unverified box became a tagged Note.
3. **Glossary overlaps (Bit, Binary, Place value, Byte with MA102; Robot, Sensor, Actuator, Simulator, Motor, Program, Loop, Condition, Control loop with RB101; Simulator, Sensor, Motor, Control loop with KID103)** — ruling **A7**: KID101 comes first in build order, so its wording is the canonical one; later courses link to it. `glossary.json` now carries the checked wording and the verified source of every entry; four definitions changed with their chapters (Algorithm, Character code, Robot, Motor) and one more (Bus).
4. **Builder behaviour: forensic code shown with `data-src` but no line table; simulators compiled but not shown** — decided by verifier: kept. Forensic code is evidence, not a teaching listing. Ruling **A2** applied to the three course simulators: F0-01 now says the literal cook is a teaching model, not a real tool; F0-08 and F0-10 now say the grid robot and the maze robot are the course's own simulators standing in for a real robot simulator such as Gazebo (the simulator named by ruling **D2**), used from RB101 on.
5. **Cross-links to other batches (#KID102, #F0-29 … #DS201, #safety, #analogies)** — checked with `python3 university/build/build.py`: no PROBLEM line names a KID101 chapter, so all links resolve.
6. **Course exam and course project files** — ruling **B4**: the exams (quizzes from each "Check yourself", final with a forensic question, practical with a run reference solution, course project brief) are written in the exam pass (`EXAMS.html` and `_keys/KID101.keys.html`), not in this verification pass.
7. **Untested hardware** — ruling **C3**: KID101 has no hardware steps (every lab is paper, a friend, or our own simulators), so no "untested on hardware" box is needed. Status per **C4**: when G1–G8 pass, the chapters are "internally checked".
8. **Recorded runs** — ruling **A5**: all ten lab folders re-run; outputs identical; recorded `.out`/`.log` files restored with `git checkout`.
9. **Exercise values** — ruling **A4**: the night-light limit 30 on a 0–100 scale (F0-06), the sorting cards, the toy kitchen's boxes and the maze are exercise values, already labelled as the chapter's own "made-up scale" or "pretend" values.
10. **Rulings A8 (keys), A9 (axes), A10 (licence), B1–B3, B5, D1, D3, D4** — not applicable to KID101: no keys, no axes, no `LicenseRef-Uni-Lab` lines in the KID101 lab folders, no mega project, no robot image and no kit item is named.

## Verification pass

Fact-Checker / Source Researcher / Editor / Accessibility agent, 2026-10-10. Dossiers: `university/_dossiers/F0-01.dossier.html` … `F0-10.dossier.html`. QA records: `university/qa/F0-01.json` … `F0-10.json`.

**Opened** (all on 2026-10-10, addresses in the dossiers): Petzold, "Code" 2nd ed. (publisher sample: Chapters Six and Eleven, and the table of contents); Nisan and Schocken, chapters 2 and 4 (site chapters, no edition shown) and chapter 5 (2nd-edition draft) from the nand2tetris site; Bryant and O'Hallaron, CS:APP 2nd ed. sample Chapter 1 and partial Chapters 2 and 6; OSTEP v1.10 Chapter 10; the C++ working draft (eel.is rendering generated 2026-08-23; 27 section pages); GCC 13.3.0 manual, Instrumentation Options; Sedgewick and Wayne, "Algorithms, 4th Edition" book site; RFC 20, RFC 791, RFC 793, RFC 9293; the Unicode glossary; the W3C PNG Specification (Third Edition) and Web Audio API 1.1; Fall and Stevens, "TCP/IP Illustrated, Volume 1" 2nd ed. (publisher sample, Chapter 12); Kleppmann's Cambridge distributed-systems notes (2021/22); Lynch and Park, "Modern Robotics" (May 2017 preprint); the PX4 Guide "Basic Concepts" page; TI datasheets for DRV8833, LM35 and OPT3001; Thrun's report CMU-CS-00-126.

**Not openable, so replaced**: "A Tour of C++" (replaced by the C++ working draft everywhere), "Make: Electronics", "Small Unmanned Aircraft", "Probabilistic Robotics" and van Steen and Tanenbaum's "Distributed Systems" (replaced by the documents above). Petzold is now cited only for what the opened Chapter Eleven says.

**Claims**: 188 tagged claims checked (per chapter: 18, 17, 22, 16, 27, 19, 18, 8, 23, 20). **Corrected** (18): F0-01 algorithm definition; F0-02 "billions" → "millions on a single chip"; F0-03 ASCII expansion and Unicode wording; F0-04 names moved out of the unverified box; F0-05 "real CPUs work load–work–store" limited to the textbook's simple model, bus definition; F0-06 mouse sentence and "sensor gives a number" → "signal read as a number"; F0-08 the fence-post example (a closed garden fence needs 10 posts, not 11; now a straight fence), "most bugs" statistic, working definitions untagged and boxed; F0-09 "nobody tells the sender" → IP sends no receipt; F0-10 robot definition, "most common actuator", motor-driver sentence, sensor drift wording, "rules in most countries".
**Left unverified** (boxes): F0-02 scale; F0-03 sample rates / bytes per pixel / microphone physics; F0-05 product numbers; F0-08 course working definitions (check ISO/IEC/IEEE 24765); F0-09 packet sizes and loss rates; F0-10 loop rates and motor/battery ratings.

**Labs**: all ten folders re-run with `run_lab.sh` (exit 0 for every folder; `F0-07/fill_forgot` exit 124 by design). Only the dates in the logs differed; the recorded files were restored (A5). No hardware steps.

**Diagrams, edit, accessibility**: 11 SVG figures checked against guide 9.3 (role, title, desc, caption, sv-* classes, no colour-only meaning; F0-10 maze checked against `robot_world.hpp`). Sources sections now give edition or version and sections as printed; no URLs in chapters; fragments validated (balanced tags, prefixed unique ids, no scripts) and `build.py` shows no PROBLEM for KID101.
