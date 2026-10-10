# MA101 Numbers and algebra — author and Lab Engineer notes

Batch 1 (Foundation year). Author/Lab Engineer run in the cloud build container,
2026-10-09. No internet access; no document was opened. Everything below is for the
Source Researcher, Fact-Checker, Editor and the owner.

## Files

- Chapters: `F0-11.html` … `F0-19.html` (fragments per `../FRAGMENT_FORMAT.md`; all 21
  sections plus "Answers to Check yourself" with a "Forensic lab answer key" h3).
- `glossary.json`: 50 four-part entries proposed by this course.
- Labs: `university/labs/F0-11/` … `university/labs/F0-19/` (33 listings, each with
  `.out` and `.log`).

Levels: F0-11 to F0-15 are L0 (maths ladder rung 1); F0-16 to F0-19 are L1 (rung 2).

## Sources pattern used in every chapter (read this first)

- **D1** "Definitions and arithmetic worked out in this chapter". Guide 4.2 says that for
  MA101 "the Researcher writes from first principles and checks every statement against
  a named textbook chosen by the Dean". **No MA101 textbook has been chosen yet**, so every
  D1 is marked "dossier gate G1 open". Decision for the Dean: choose the textbook.
- **R1 / R2 / R3** are lab runs in this build (toolchain line from the logs:
  `g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0`, flags
  `-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined`).
  R1 = the chapter's listing(s), R2 = `checks.cpp` (recomputes every number in the text,
  guide 7.3 row 9), R3 = the deliberately broken forensic evidence program.
- **B1** Petzold, "Code"; **B2** Stroustrup, "A Tour of C++". Both cited *title only — not
  opened during this build* for conceptual computer statements and C++ language rules.
- **U1** the University Authoring Guide itself (sections 2.2, 5.5, 14, AH-27), cited only
  for "what later courses teach" (the F6 grid formula, KiB in F0-23, sanitizers).

## Per chapter: claims not verified against a document, boxes, listings

| Chapter | Claims resting on title-only books (G1 open) | Unverified boxes | Listings run (all exit 0) |
|---|---|---|---|
| F0-11 | B1: computers store numbers with two-symbol place value | none | place_value, checks, button_count_bug |
| F0-12 | B1: processors add base-two numbers with switch circuits; B2: no division by zero, integer types have a largest value | none | four_ops, estimate (+ .in), checks, egg_total_bug |
| F0-13 | B1: numbers with fractional parts stored in base two with fixed places (0.1 inexact) | none | fractions, tenths, checks, grapes_bug |
| F0-14 | B1: two's complement exists as the rule for negatives; B2: int division of negatives, unsigned has no negatives | none | negatives, wrap, checks, thermometer_bug |
| F0-15 | B1: bits, bytes, memory cells, addresses, "n address bits pick 2^n places"; B2: CHAR_BIT | **1**: "How the hardware actually does it" — no real device memory size is stated; learners must source any such number themselves; Researcher to confirm B1 covers the address paragraph | powers, byte_bits, checks, switches_bug |
| F0-16 | B1: variables live in memory; B2: assignment meaning, `*` before `+` | none | recipe, checks, tickets_bug |
| F0-17 | B1: computer executes steps exactly and in order | none | balance, balance_try (+ .in), checks, balance_bug |
| F0-18 | B2: C++ functions may have side effects | none | robot_sim, checks, prediction_bug |
| F0-19 | B1: memory has no walls between lists; B2: vector `[]` not range-checked, out-of-range is undefined behaviour | none | bounds, helpers_plan (+ .in), checks, bounds_bug |

Measured-on-this-machine facts (AH-23), labelled as such in the text: 8 bits per byte
(`CHAR_BIT`, F0-15), `unsigned` 5 − 7 printed 4294967294 (F0-14), 0.1 + 0.2 printed as
0.30000000000000004 with 17 digits (F0-13), `-7 / 2` printed −3 (F0-14).

No URLs, no hardware numbers, no product names, no kit names anywhere.

## Things I was unsure about / decisions for the owner or Dean

1. **New analogy mappings** (guide 8.2 says the Dean registers them): beans in cups and
   bowls (place value), children in cars (rounding up), flatbread slices (fractions), lift
   floors and thermometer (negative numbers), paper folding (powers of two), labelled jar
   for a maths variable (registered for *program* variables; used here with the stated
   difference), balance scale (equations), function machine, numbered jar shelf (allowed
   range / bounds check), "tables and seats → tickets" (index formula). Please register or
   replace.
2. **"Halfway rounds up"** (45 → 50) is stated as *this course's* convention; the chosen
   textbook may present it differently.
3. **"Billion = a thousand million"** is stated as this course's definition, not as a
   universal fact.
4. **Bit and byte** appear in F0-15's jargon box but are not proposed in `glossary.json`,
   to avoid two definitions (owners: F0-02 for bit, F0-23 for byte). The F0-15 text says
   "on the machine that built this chapter, a byte had 8 bits" (measured).
5. **F0-15 mini-project = course project starter** (powers-of-two poster, each example
   sourced). F0-19 mini-project = practice for the practical exam (helpers and jobs).
   Course-level exams (Q, M, F, P) and the project brief are not written here (not in
   my deliverables).
6. **Forensic labs**: all nine use the course's "The wrong total" theme; each evidence pack
   is the real output of a deliberately broken program (R3). Answer keys are inline under
   "Forensic lab answer key" as the brief requires (guide 7.1 would put them in `_keys/`
   for graded use).
7. **Lab Engineer catch**: the F0-16 number-check run caught an arithmetic slip in my draft
   (3a + 2b with a = 1, b = 10 was written as 32; the run printed 23). Fixed before hand-off.
   This is the reason every chapter's numbers are recomputed by `checks.cpp`.
8. **Reading level**: Layer 1 average sentence length by a simple script is 7.5–10.2 words
   per chapter (L0 target about 12, L1 about 15). A few heading+sentence joins show as
   "long" in the script; the Editor should still do the read-aloud test (10.4).
9. **Prose length**: running prose (excluding code, SVG, tables, jargon lists, numbered
   lists, quiz, sources and answers) is 2,054–2,512 words per chapter (F0-14 lowest,
   F0-18 highest), inside the 1,500–3,000 band; including all of those it is 3,400–4,200.
10. **Cross-links** to chapters of other courses (#F0-02, #F0-20, #F0-23, #F0-24, #F0-30,
    #F0-31, #F0-32, #F0-33, #F0-34, #F0-36, #KID102, #MA102) assume those ids exist in
    the integrated page; glossary links #gl-bit and #gl-byte assume the KID101/MA102
    entries.
11. The SVG in F0-15 and F0-18 uses a `transform="rotate(...)"` attribute on axis labels
    (geometry only, no colour); acceptable under 9.1 as I read it.

## Owner rulings applied

Rulings from `university/OWNER_RULINGS.md`, applied in the verification pass on 2026-10-10.

- **Textbook for MA101 (decision for the Dean, "Sources pattern" above).** Decided by verifier, under C1/C2: OpenStax "Prealgebra 2e" is the textbook for rung 1 (F0-11 to F0-16) and OpenStax "Elementary Algebra 2e" for rung 2 (F0-17 to F0-19). OpenStax "Intermediate Algebra 2e" (logarithms, functions, interval endpoints) and "Contemporary Mathematics" (place value, expanded form, other bases) are added where those books do not cover a claim. All four are free to read online, so every cited section was actually opened. In every chapter, D1 is now the textbook. The old "worked out in this chapter, dossier gate G1 open" entry is gone.
- **B1 (Petzold, "Code") and B2 (Stroustrup, "A Tour of C++"), cited by title only.** Decided by verifier, under C1/C2: C++ language claims now cite the C++20 working draft N4868 (D2, tier 1), whose sections were opened. B2 was removed from every chapter. For B1 only the publisher's catalogue page and table of contents (2nd Edition, 2022) could be opened. B1 stays only in F0-12 and F0-15, and only to support "Not verified" boxes about circuits and memory cells. Every other B1 claim was re-sourced to D2 or D5.
- **1. Analogy mappings:** approved by **A3** (registered by the build lead in the analogy registry). No change.
- **2. "Halfway rounds up":** decided by verifier. D1 §1.1 rounds up when the digit is 5 or more, the same as the chapter, so it is kept.
- **3. "Billion = a thousand million":** decided by verifier. It is kept as the course's definition and is consistent with D1 §1.1 (periods of three digits: thousands, millions, billions).
- **4. Bit and byte not in glossary.json:** **A7**. The first course's wording is canonical (KID101 F0-02 for bit, MA102 F0-23 for byte), so F0-15 keeps linking to those entries.
- **5. Exams and project brief:** **B4**. The course exams are written in the exam pass (`build/verify/EXAM_BRIEF.md`), not in this verification pass. The F0-15 and F0-19 mini-projects stay as starters.
- **6. Forensic answer keys inline:** **A6** (honest evidence may stay). Decided by verifier: the keys stay under "Answers to Check yourself", as FRAGMENT_FORMAT requires.
- **8. Reading level:** checked. Layer 1 averages 7.3–9.4 words per sentence (script). Two long sentences added in this pass (F0-12 Layer 3) were split.
- **9. Prose length:** **A1**. Accepted as written; nothing was trimmed for length.
- **10. Cross-links:** checked by `build.py` (no PROBLEM lines for MA101).
- **11. SVG `transform="rotate(...)"`:** decided by verifier. It is acceptable (geometry only, no colour; guide 9.1).
- **A2 teaching stand-ins:** F0-18 now says plainly that the toy-robot simulator is the course's own teaching program and names the real tool it stands for (the Gazebo simulator, ruling D2).
- **A4 exercise values:** the made-up inputs (87 grains per spoon, 64 spoons) were already labelled as made-up example numbers. No product values appear.
- **A5 recorded runs:** all nine labs were re-run. Only the date lines changed, so the recorded `.log` files were restored. The exception is F0-13 `checks`, whose program changed (see below).
- **A8 keys, A10 licence, D4 e-stop, B1/B2 gates:** not applicable. MA101 has no keys, no licence placeholder, no hardware and no mega project.
- **C3/C4:** MA101 has no hardware steps, so no chapter carries an "untested on hardware" item.

## Verification pass

Done on 2026-10-10 by the Fact-Checker agent (Source Researcher, Fact-Checker, Diagram reviewer, Editor and Accessibility reviewer roles).

- **Opened (tier 1–5):**
  - OpenStax "Prealgebra 2e", sections 1.1, 1.3, 1.5, 2.1, 2.2, 3.1, 4.1 and 5.3, plus the key-concept pages of chapters 1–7 and 10.
  - OpenStax "Elementary Algebra 2e", section 1.2 and the key-concept pages of chapters 1–4.
  - OpenStax "Intermediate Algebra 2e", sections 2.5, 3.5 and 10.3, plus the key-concept pages of chapters 3 and 10.
  - OpenStax "Contemporary Mathematics", sections 4.1 and 4.3.
  - The C++20 working draft N4868: [basic.fundamental], [expr.mul], [expr.ass], [expr.pre], [intro.memory], [intro.execution], [sequence.reqmts], [vector.overview], [defns.undefined], [iterator.requirements.general], [iostream.syn], [std.manip], [numeric.limits.members] and [climits.syn].
  - Goldberg, "What Every Computer Scientist Should Know About Floating-Point Arithmetic" (1991; Oracle reprint).
  - The GCC 13.3.0 manual, "Instrumentation Options".
  - The publisher's catalogue page for Petzold, "Code", 2nd Edition (contents only).
  - Every URL is recorded in the dossiers (`university/_dossiers/F0-11.dossier.html` … `F0-19.dossier.html`). The chapters contain none.
- **Checked:** 139 tagged statements in the nine chapters. Every claim tag now names a section (for example "D1 §1.5" or "D2 §7.6.5").
- **Corrected (32 recorded corrections; details in `university/qa/F0-1*.json`):**
  - F0-14: C++ `int` division truncating toward zero and unsigned wrap-around (modulo 2<sup>N</sup>) are now stated as C++ rules, not only as observations. Two's complement is tied to the C++20 rule.
  - F0-19: the claim that an out-of-range write "would quietly write into somebody else's spot; nothing in the hardware shouts" is replaced by the undefined-behaviour wording (it may change other data or crash; nothing is promised). The sanitizer claim now cites the GCC manual.
  - F0-13: "computers usually store decimals in base two" is replaced by a measurement. `labs/F0-13/checks.cpp` now prints `std::numeric_limits<double>::radix`, which was 2. The lab was re-run, and the new `checks.out` and `checks.log` are kept.
  - F0-15 and F0-16: the hardware paragraphs are rebuilt on the C++ memory model.
  - F0-11 and F0-18: notes on textbook conventions ("and" in number names; y = mx + b).
  - Hedge words were removed (AH-20).
  - Two figure descriptions were corrected to match their drawings (F0-12 Figure 1, F0-19 Figure 1).
  - `glossary.json` sources now point to the verified section tags.
- **Left unverified (4 boxes):**
  - F0-11: uniqueness of base-ten representation, and comparing from the left (general rules).
  - F0-12: the processor's adder circuits and shift-and-add steps (B1 titles only).
  - F0-13: the 2s-and-5s rule for ending decimals.
  - F0-15: memory built from one-bit cells, and memory sizes being powers of two.
- **Labs:** all 33 listings were re-run with `run_lab.sh`. All exit 0, and the outputs are identical apart from timestamps (restored, ruling A5), except the extended F0-13 `checks`.
- **Still open:** none of the open items needs the owner. The four unverified boxes need a number-theory textbook (F0-11, F0-13) or HW102's sources (F0-12, F0-15).
