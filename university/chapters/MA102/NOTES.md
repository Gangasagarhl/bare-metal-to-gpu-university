# MA102 Binary, hexadecimal and logic — author and lab engineer notes

Batch 1 (Foundation year). Written 2026-10-09 in a sandbox without internet access, following
`university/build/AUTHOR_BRIEF.md` and `university/chapters/FRAGMENT_FORMAT.md`.

## Files

- Chapters: `F0-20.html` … `F0-28.html` (all 21 sections + "Answers to Check yourself",
  forensic answer key inline under an `<h3>` in the Answers section).
- Glossary proposals: `glossary.json` (44 four-part entries, each with its page anchor).
- Labs: `university/labs/F0-20/` … `university/labs/F0-28/` (30 listings; `.cpp`, `.in`,
  `.out`, `.log`, `.expect-fail`).

## How every chapter handles sources

No document could be opened. Every chapter therefore uses three kinds of source entry:

- **W1** — the chapter's own worked arithmetic/logic, true by definition and shown step by step.
- **R1, R2, …** — real runs of the chapter's listings in this build (toolchain line and date
  are inserted from each `.log`). Toolchain: `g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0`,
  flags `-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined`,
  Linux x86_64 cloud build container.
- **D1, D2, …** — books and standards cited **by title only**, each marked "not opened during
  this build … dossier gate G1 open". The Source Researcher must confirm edition and section
  for every D entry before publication.

No URLs, no `<script>`, no hardware numbers except those printed by our own runs
(`CHAR_BIT`, `sizeof(int)`, `sizeof(void*)` in F0-23, labelled as measurements on the build
machine).

## Per chapter

### F0-20 Counting with two symbols (L0)
- Listings run: `count_binary.cpp`, `place_values.cpp`, `three_switches.cpp` (forensic
  evidence) — all exit 0.
- Unverified boxes: none.
- Title-only claims (G1 open): bits held by two-state circuits (Petzold "Code"; Nisan &
  Schocken). Required diagram (guide 9.2): row of switches with place values — Figure 1.
- Forensic: overflow of a 3-bit counter (evidence from a real run).

### F0-21 Converting between binary and decimal (L0–L1)
- Listings run: `convert.cpp`, `reversed.cpp` (forensic evidence) — exit 0.
- Unverified boxes: none.
- Title-only claims: conversion happens when programs read/print numbers (Petzold; Bryant &
  O'Hallaron). "Horner's rule" name in the Layer 3 note is not tagged to a specific source
  beyond D1 — Fact-Checker should confirm or drop the name.

### F0-22 Hexadecimal: binary's shorthand (L1)
- Listings run: `hex_table.cpp`, `colour_mixer.cpp` (+ `.in`, the course lab "hex colour
  mixer"), `colour_bug.cpp` (+ `.in`, forensic evidence) — exit 0.
- Unverified box (1): use of #RRGGBB outside this course (web pages / CSS colour
  specification, title only). In the chapter #RRGGBB is defined as our own exercise rule.
- Title-only: which C++ standard version introduced binary literals and digit separators
  (ISO/IEC 14882) — the chapter deliberately names no version; behaviour shown by the
  F0-25 build with `-std=c++20`.

### F0-23 Bytes, words and units (kB versus KiB) (L1)
- Listings run: `units.cpp`, `two_reports.cpp` (forensic), `two_reports_fixed.cpp` (key) — exit 0.
- Unverified boxes (2):
  1. Naming of binary prefixes (kibi/mebi/gibi) → IEC 80000-13 (title only); decimal
     prefixes → "The International System of Units (SI)", BIPM SI Brochure (title only).
     The arithmetic 1024 = 2^10 vs 1000 is shown and checked by the run.
  2. That some processor manuals use "word" for a size smaller than the register width —
     check the vendor architecture manual named in the dossier.
- Measurements: `CHAR_BIT` = 8, `sizeof(int)` = 4, `sizeof(void*)` = 8 on the build machine
  only (labelled as such, AH-23).
- House rule cited: guide 13.3 (units) — not a technical source.

### F0-24 Negative numbers in binary: two's complement (L1)
- Listings run: `twos.cpp`, `print_int8.cpp`, `temperature.cpp` (course forensic "Why is my
  number negative?"), `temperature_fixed.cpp` (key) — exit 0, no sanitizer messages.
- Unverified box (1): C++20 (ISO/IEC 14882:2020, title only) requiring two's complement and
  defining out-of-range conversion to signed types as modular; also the
  implementation-defined signedness of plain `char`. Behaviour is demonstrated by the run
  (200 → −56, 127 + 1 → −128 with UBSan enabled and silent).
- Title-only: sign extension, one adder for signed and unsigned (Bryant & O'Hallaron; Nisan &
  Schocken).

### F0-25 Masks and shifts (L1)
- Listings run: `masks.cpp`, `light_bug.cpp` (forensic), `light_fixed.cpp` (key) — exit 0;
  `precedence.cpp` is a deliberate `.expect-fail` listing (compile failed as expected with
  `-Werror=parentheses`; the real message is shown).
- Unverified box (1): exact C++ rules for integral promotion and shifting signed values
  (ISO/IEC 14882, title only).
- The worked example decodes a **made-up** 8-bit status byte (practice for the course exam P).

### F0-26 Truth tables: AND, OR, NOT, XOR (L0–L1)
- Listings run: `switch_circuits.cpp` (the university's own tiny C++ text circuit model; no
  simulator is named), `hall_light.cpp` (forensic), `hall_light_fixed.cpp` (key) — exit 0.
- Unverified boxes: none. Safety box added: models only, no mains work, real circuits in
  KID103 with battery kits designed for children and an adult present.
- Title-only: switches in series/parallel as AND/OR, two-way switches (Platt "Make:
  Electronics"; Petzold); NAND universality (Nisan & Schocken); short-circuit evaluation
  (Stroustrup).

### F0-27 Boolean algebra and De Morgan's laws (L1)
- Listings run: `demorgan.cpp`, `three_inputs.cpp`, `alarm.cpp` (forensic), `alarm_fixed.cpp`
  (key) — exit 0; `compiler_knows.cpp` is a deliberate `.expect-fail` listing.
- Observation from the build, kept as a lesson: the first version of `three_inputs.cpp`
  compared `!(a || b || c) == (!a && !b && !c)` in one expression and GCC refused it with
  `-Werror=tautological-compare` ("self-comparison always evaluates to true"). That became
  Listing 3 (`compiler_knows.cpp`); `three_inputs.cpp` now stores each side in a variable.
- Title-only: Boolean algebra laws and De Morgan (Harris & Harris; Petzold). The names
  George Boole and Augustus De Morgan are tagged to Petzold (title only) — the Fact-Checker
  must confirm or remove the attributions.

### F0-28 Invariants: things that stay true (L1)
- Listings run: `invariant.cpp`, `marbles.cpp` (forensic), `marbles_fixed.cpp` (key) — exit 0.
- Unverified boxes: none.
- **Decision for the owner/Dean:** loop invariants are cited to Cormen, Leiserson, Rivest and
  Stein, "Introduction to Algorithms" (title only), which is **not in the guide's source
  registry (section 4)**. Approve it or name another book.
- Title-only: parity bits in memory/communication (Petzold; Patterson & Hennessy); `assert`
  (Stroustrup).

## Decisions left to the owner

1. Approve publication with the 5 unverified boxes (F0-22 ×1, F0-23 ×2, F0-24 ×1, F0-25 ×1)
   (AH-19), or have the Source Researcher close them first.
2. Add "Introduction to Algorithms" (or another text) to the registry for F0-28.
3. Glossary overlap: **Bit**, **Byte**, **Overflow** and possibly **AND/OR/NOT** may also be
   defined by KID101 (F0-02, F0-03). The Integrator must merge so that one wording is used
   (guide 10.3). MA102's definitions are in `glossary.json` with `chapters` listing the
   defining chapter first.
4. Levels: F0-20 is L0, F0-21 and F0-26 are L0–L1, the rest L1 (course card is L0–L1).
5. Course project (binary-to-hex-to-decimal converter with tests, written in KID102) and the
   course exams (Q, F, P "decode a made-up 8-bit status register") are not part of this
   deliverable; F0-25's worked example and mini-project prepare for exam P, and F0-28's
   Transition box introduces the round-trip test for the project.

## Reading level (measured by a small script, Layer 1 sections)

Average words per sentence in Layer 1: 8–13 across the nine chapters (L0 target about 12,
L1 about 15). Layer 2 averages 10–21; the higher values (F0-23, F0-25) come from
unverified boxes and code-heavy sentences — the Editor should review them.

## Process notes

- All 30 listings were built and run with `university/labs/run_lab.sh` on each folder
  F0-20 … F0-28: 28 exit code 0, 2 "compile failed as expected". No sanitizer messages.
- Every `data-run` and `data-src` reference in the fragments was checked to exist.
- Fragments validated: balanced tags (python `html.parser`), all ids prefixed with the
  chapter id and unique, 21 sections present and in order, no URLs, no scripts.
- Incident: while re-running labs with the shell glob `labs/F0-2?`, the runner was also run
  on `labs/F0-29` (KID102, another author's folder). It only regenerated that folder's
  `.out`/`.log` files (same programs, same toolchain, exit 0, new dates); no source file was
  touched. The KID102 author may want to re-run it themselves.
