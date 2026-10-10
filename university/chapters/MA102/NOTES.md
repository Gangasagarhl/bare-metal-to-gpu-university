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

## Owner rulings applied

Applied on 2026-10-10 in the verification pass (see `university/OWNER_RULINGS.md`).

1. **Publication with the 5 unverified boxes (decision 1)** — ruling **C1** (verify against the real
   documents) and AH-18/AH-19. All five original boxes were checked: F0-22 (#RRGGBB, W3C CSS Color 4
   §5.2), F0-23 ×2 (SI Brochure §3, NIST binary prefixes; RISC-V manual "word" = 32 bits) and F0-24
   (C++ N4861 §6.8.1, §7.3.8) and F0-25 (N4861 §7.3.6, §7.6.2.1, §7.6.7) were confirmed and turned
   into notes. Five new boxes now hold what could not be confirmed from an openable source:
   F0-21 (the name "Horner's rule"), F0-26 (how real stair lights are wired), F0-27 ×2 (names of the
   laws; "perfect induction" and chip design tools), F0-28 (the textbook presentation of loop
   invariants). Ruling **C4**: the chapters pass G1–G8 and are "internally checked"; MA102 has no
   hardware steps.
2. **"Introduction to Algorithms" for F0-28 (decision 2)** — ruling **C2**: a new source is approved
   for the registry only if the verifier could open it or an official description. The MIT Press
   site refused automated access and the web budget ran out, so the book is **not** added. F0-28 now
   rests on its own proofs (W1) and names the book only in an unverified box.
3. **Glossary overlap with KID101 (decision 3)** — ruling **A7**: Bit, Binary, Place value and Byte
   have the same meaning in KID101 (F0-02/F0-03), which comes first in build order, so KID101's
   wording is canonical. `build.py` keeps the first entry and merges the chapter lists, so MA102's
   chapters link to the KID101 entry; no change to other units' files was needed. Overflow and
   AND/OR/NOT are not defined by KID101 and stay as MA102 entries.
4. **Levels (decision 4)** — decided by verifier: kept as written (F0-20 L0; F0-21 and F0-26 L0–L1;
   the rest L1), matching the course card's L0–L1. Measured Layer 1 averages are 8–13 words per
   sentence.
5. **Course project and exams (decision 5)** — ruling **B4**: exams (Q from each "Check yourself",
   M, F with a forensic question, P "decode a made-up 8-bit status register") and the project brief
   are written by the exam pass (`build/verify/EXAM_BRIEF.md`), not by this verification.
6. **Other rulings checked** — A1 (no trimming for length: only wrong or unsupported text was
   changed), A2 (the F0-26 circuit model is labelled as the university's own model; the real
   circuits are KID103's battery kits), A4 (the F0-25 status byte and the F0-24 oven are labelled as
   made up), A5 (recorded runs kept, see below), A8 (no keys in MA102 labs), A10 (no
   `LicenseRef-Uni-Lab` placeholder in MA102 files; nothing to change), C3 (no hardware steps, so
   no "untested on hardware" items).

## Verification pass

Done 2026-10-10 by the Fact-Checker / Source Researcher agent. Dossiers:
`university/_dossiers/F0-20.dossier.html` … `F0-28.dossier.html`; QA records:
`university/qa/F0-20.json` … `F0-28.json`.

**Opened** (URLs in the dossiers): Petzold, "Code", 2nd ed. — publisher sample with Chapter Six
"Logic with Switches" and Chapter Eleven "Bit by Bit by Bit"; Nisan and Schocken, chapters 1 and 2
as published on the Nand to Tetris site; C++ working draft N4861 (sections listed per chapter), and
N4140/N3337 [lex.icon]; GCC 13.3.0 manual "Warning Options"; BIPM SI Brochure 9th ed. (V4.01) §3 and
the BIPM "SI prefixes" page; NIST "The binary prefixes"; IEC 80000-13:2025 catalogue page; W3C CSS
Color Module Level 4 §5.2; RISC-V Unprivileged ISA v20260120 (Introduction, RV32I); Microsoft Learn
"assert Macro"; MacTutor biography of De Morgan. Catalogue or table-of-contents pages only:
Bryant and O'Hallaron 3/E (TOC), Platt "Make: Electronics" 3rd ed., Harris and Harris (ARM ed.,
chapter 2 listing), Stroustrup "A Tour of C++" 3rd ed.
**Could not open:** Intel SDM (download refused), Arm documentation (JavaScript only), C11 draft
N1570 beyond clause 5, glibc manual, MIT Press pages, NIST DADS (web budget exhausted).

**Corrected** (details in each QA record): F0-21 hardware paragraph (keyboards do not "send" the
number; the program converts characters, shown by R1) and the shift/division equivalence (only for
non-negative values that fit); F0-22 "nibble" no longer called an informal engineers' name, C++14
named as the version that added binary literals; F0-23 the standard symbol for the bit is "bit" (b
is a shorthand), C++ byte size is implementation-defined, exact-width types are optional; F0-24 and
F0-25 hardware paragraphs grounded in the RISC-V manual; F0-26 figure source corrected and the
real-wiring claim boxed; F0-27 double negation "is its own twin", synthesis-tool and "perfect
induction" claims boxed; F0-28 parity paragraph no longer claims which devices use parity. Books
that could not be read were replaced by openable tier-1 sources (C++ draft instead of Stroustrup,
RISC-V manual instead of Patterson and Hennessy / Bryant and O'Hallaron).

**Left unverified:** 5 boxes, listed under "Owner rulings applied", item 1.

**Labs:** all 30 listings re-run with `run_lab.sh` on 2026-10-10: 28 exit code 0 with no sanitizer
messages, 2 "compile failed as expected" (F0-25 `precedence.cpp`, F0-27 `compiler_knows.cpp`).
Outputs were identical to the recorded ones; only log dates differed, so the recorded files were
restored (ruling A5). The Sources lists now quote the dates of the recorded logs.

**Other edits:** glossary anchors without trailing dashes; `glossary.json` sources and
`last_verified` updated; long new sentences split for the reading level. Fragments validated
(balanced tags, prefixed unique ids, no URLs, no scripts) and `build.py` reports no problem for
MA102.
