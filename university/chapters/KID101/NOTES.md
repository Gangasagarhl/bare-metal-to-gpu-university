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
