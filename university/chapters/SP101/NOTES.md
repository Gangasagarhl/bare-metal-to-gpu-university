# SP101 — build notes (C++ I: programs that work, F2-01 to F2-08)

Build date: 2026-10-09. Build machine: Linux x86_64.
Toolchain as printed by the tools: `g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0`,
GNU binutils (`nm`, `c++filt`), `file`.
Every listing was built and run with `university/labs/run_lab.sh` (flags
`-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined`).
Multi-step and multi-file runs live in each lab's `run.sh`. Those runs write
`<name>.out` (commands and their output) and `<name>.log` (run record). Each
`run.sh` starts with the same helper. The last full re-run of all eight folders
finished with runner exit code 0 for every folder.

## Files

- Chapters: `F2-01.html` … `F2-08.html`. Each has all 21 template sections plus
  "Answers to Check yourself" and a forensic answer key, a jargon box, a
  transition box, and one inline SVG figure. All eight pass the fragment checks:
  html.parser balance, ids prefixed with the chapter id, no URLs, no `<script>`,
  all section ids in order, every claim tag resolves, and every
  `data-src`/`data-run` file exists.
- `glossary.json`: 64 entries (term, simple, analogy, precise, source,
  related, chapters, anchor `gl-…`), taken from the chapters' jargon boxes.
  Terms that already exist in KID102, MA102 and other courses are linked, not
  redefined. Examples: `gl-compiler`, `gl-declaration`, `gl-std-vector`,
  `gl-two-s-complement`. One SP101 term was renamed to "Floating-point
  tolerance" because HW101 already defines "Tolerance" with a different meaning
  (component tolerance).
- Labs: `university/labs/F2-01` … `F2-08`.

## Listings run (58 recorded runs)

| result | count | which |
|---|---|---|
| built and ran, exit code 0 | 36 | all ordinary listings and `run.sh` "ok" steps |
| expected compile failure | 13 | F2-01 no_declaration; F2-02 narrowing, wconversion; F2-03 assign_in_if, fallthrough; F2-04 const_change, ambiguous, bind_literal; F2-05 too_many, sign_compare; F2-06 designator_order; F2-07 no_guard; F2-08 static_fail |
| expected link failure | 5 | F2-01 link_missing, forensic_no_main; F2-07 forgot_file, header_body, forensic_build (the course forensic "Undefined reference") |
| intentional exit code 134 (failed `assert`) | 2 | F2-08 assert_fail, forensic_debug |
| intentional exit code 1 (AddressSanitizer report) | 2 | F2-05 forensic_last_order, forensic_unitbuf |

Total: 36 exit 0 + 22 expected failures = 58. No run failed unexpectedly. No log
contains "UNEXPECTED" or "STEP FAILED".

Some runs print undefined-behaviour diagnostics but exit 0 by design:
F2-02 `overflow` (UBSan "signed integer overflow" message) and F2-02 `pantry`
(unsigned wrap-around, no diagnostic).

No GPU or other hardware is involved. Nothing in this course is "untested on
hardware".

### Outputs normalised by `run.sh` (so reruns are byte-stable)

- Absolute lab paths are removed (`university/labs/F2-0x/` prefix stripped).
- Process ids are shown as `==<pid>==`, and temporary object names as `/tmp/cc<random>.o`.
- F2-05 ASan reports show addresses as `0x<addr>` and the BuildId as `<hash removed>`.

### Runs that needed a workaround

- F2-05: ASan lost the buffered `std::cout` lines. `stdbuf -oL` cannot be used
  with ASan, which gives the error "ASan runtime does not come first". So
  `evidence/last_order_unitbuf.cpp` adds `std::cout << std::unitbuf;`, and the
  chapter shows both runs.
- F2-07 `header_inline`: `-I` does not override a quoted include from the
  same folder. The step copies the sources into `header_body/inline/` and adds
  `inline` with `sed`.
- F2-08 `tests_pass`: an earlier version printed `$?` from a fresh shell,
  which is always 0. It now uses `./test_units && echo '(… exited with code 0)'`.

## "Not verified" boxes (build state 2026-10-09; see "Verification pass" below for the current state)

1. F2-01 (Lab): commands on non-Linux or non-GCC toolchains. Only Linux, GCC
   and GNU binutils were used.
2. F2-02 (Layer 2): type sizes on other machines. Check the fundamental-types
   clause of ISO/IEC 14882 and the platform ABI.
3. F2-06 (Hardware): the reasons for alignment (boundary-crossing cost, and
   processors that reject misaligned access). Check the SysV AMD64 psABI and
   CS:APP. The measured 24/16 bytes do not depend on this.
4. F2-07 (Lab): the conversion factors and the °C/°F formula in `units.cpp`
   were written from general knowledge. The "cup" unit is left for the
   student to define with a source.
5. F2-08 (Hardware): abort → SIGABRT (6) → shell exit code 128 + 6 = 134.
   Only 134 was observed. The box also covers the temperature reference points
   used in the worked example.

## Claims that rest only on title-only sources (gate G1) — resolved in the verification pass, see below

Every D-source has the note "Title only — not opened during this build; the
Source Researcher must confirm the edition and section (dossier gate G1 open)."
The cited titles are:

- Stroustrup, "A Tour of C++"
- ISO/IEC 14882
- ISO/IEC 9899 (`<assert.h>`)
- the GCC manuals, including "The C Preprocessor" and the AddressSanitizer documentation
- Levine, "Linkers and Loaders"
- Bryant and O'Hallaron, "Computer Systems: A Programmer's Perspective"
- GNU Binutils documentation
- the Itanium C++ ABI (mangling)
- the System V AMD64 psABI
- the C++ Core Guidelines
- IEEE 754 and Goldberg
- Kernighan and Pike, "The Practice of Programming"
- Linux man-pages abort(3) and signal(7)
- the GNU Bash manual

Statements tagged only with D-sources are mostly in "Layer 3" and "How the
hardware actually does it". They should be checked first. Every error message,
size, offset, symbol name and output quoted in prose comes from an R-run in
this build. Troubleshooting entries for messages that were *not* run are
written as descriptions, not as quotations. Examples: F2-01 "No such file",
F2-04 "no matching function", F2-07 "No such file or directory" for a header.

## Analogy mappings proposed for the F2 world ("the restaurant kitchen at work")

These mappings are new and need registration in the analogy table.

| concept | kitchen mapping | chapter |
|---|---|---|
| preprocessor | copy clerk who pastes in the menu pages | F2-01 |
| object file | a translated recipe card with blanks for other stations | F2-01 |
| linker | the kitchen manager who pins every order to exactly one recipe | F2-01, F2-07 |
| narrowing / truncation | pouring 2.5 cups into a 2-cup jug | F2-02 |
| switch / fallthrough | the station board; a missing `break` lets the order slide to the next station | F2-03 |
| pass by value | handing the cook a photocopy | F2-04 |
| reference | handing over the real pot | F2-04 |
| const reference | the taster: may look, may not change | F2-04 |
| overloading | same-name stations for different ingredients | F2-04 |
| std::vector | the order spike (grows as tickets arrive) | F2-05 |
| std::array | the muffin tin (fixed number of cups) | F2-05 |
| std::string | the handwriting on an order slip | F2-05 |
| struct / member | the order ticket with labelled boxes | F2-06 |
| padding / alignment | empty space so boxes start on grid lines | F2-06 |
| header file | the printed menu (names and prices, no recipes) | F2-07 |
| include guard | the stamp "menu already attached" | F2-07 |
| assertion | the head chef's check at the pass | F2-08 |
| NDEBUG | "skip the checks tonight" | F2-08 |

The recurring characters Ines (head chef) and Mateo (cook / maintainer) are
used in several chapters.

## Decisions for the owner

1. **Length.** Prose word counts are 2,245 to 2,679 per chapter. They were
   counted to Sources, without code, tables or jargon box. All are above the
   L1 target of about 2,000. Keep or trim?
2. **Normalised sanitizer output.** The F2-05 ASan reports show `0x<addr>`,
   `<pid>` and `<hash removed>` instead of real values, so reruns stay
   byte-stable. Accept this, or show one raw report?
3. **Unit factors and the °C/°F formula** (F2-07, F2-08). They are not
   verified against a standard. Assign them to the Source Researcher, and
   decide which "cup" the course uses.
4. **Course project.** F2-08's mini-project defines the SP101 final project:
   a grade book or inventory, at least three files, and a test program with a
   10-point rubric. It builds on the F2-06 and F2-07 mini-projects. Please
   confirm this matches the course card.
5. **Forward links.** Chapters link the planned chapters F2-09 (Classes and
   invariants), F2-24 (Object representation) and F2-26 (Build systems with
   CMake), and the curriculum's Foundations section, including the P1
   acceptance line quoted verbatim in F2-08 Layer 3.
6. **`assert` exit code 134 and ASan exit code 1** are recorded as intended
   failures in `run.sh` ("fail" mode), not with `.expect-fail` (which is for
   compile failures only). The runner treats them as passes.

## Owner rulings applied

1. **Length** — ruling A1: chapters are kept as written; no trimming for length. Only wrong text was changed.
2. **Normalised sanitizer output** (F2-05 placeholders) — decided by verifier under guide AH-25 (machine-specific lines may be replaced by marked placeholders) and ruling A6: accepted. The chapter says which values were replaced; no raw report is added.
3. **Unit factors, °C/°F formula, "cup"** — ruling C1: checked against NIST SP 811 (SI prefixes Table 5; 1 L = 10^−3 m^3; t/°C = (t/°F − 32)/1.8). The F2-07 and F2-08 "Not verified" boxes are resolved. Decided by verifier: the course uses the U.S. cup as listed by NIST SP 811 Appendix B.9, 2.365 882 × 10^2 mL; F2-07 now says so.
4. **Course project** — rulings B3 and B6: the F2-08 mini-project (grade book or inventory, three files plus a test program, 10-point rubric) is approved as the SP101 course project and its proposed split stands (B3). SP101 is level L1, so the L0 grading option applies (B6): quizzes 20 %, practical 50 %, project 30 % (exercise values), with written feedback; midterm and final are ungraded practice papers. Writing the exam papers (B4) is outside this verification brief's file scope and stays open for the build lead.
5. **Forward links** (F2-09, F2-24, F2-26, curriculum quotes) — decided by verifier under FC-15: `build.py` reports no broken link for SP101. The curriculum quotes were checked word for word: the P1 acceptance line (1.6) and "static assertions on structure offsets taken from specs" (1.1) are exact; F2-02's quote was corrected to the curriculum's heading "Undefined behaviour that bites kernels"; F2-07's claim that the curriculum asks you to "decode linker errors on purpose" was not in the curriculum and was replaced by a reference to 1.1 "Linking and binaries".
6. **assert exit code 134 and ASan exit code 1 as intended failures in run.sh** — decided by verifier: accepted. The logs state the expected exit code, the runner treats them as passes, and the chapters say the failure is intended.
7. **Analogy mappings** — ruling A3: approved and registered by the build lead; no change here.
8. **Committed keys (A8), licence (A10), teaching stand-ins (A2), e-stop default (D4), kit (D1)** — not applicable: SP101's labs contain no keys, no `LicenseRef-Uni-Lab` markers, no stand-in tools and no hardware.
9. **Status** — ruling C4: all eight chapters pass G1–G8 in this pass and have no hardware steps, so they are "internally checked" (ruling C3: nothing is claimed as observed on hardware).

## Verification pass

Date 2026-10-25 (third session; full web access). Dossiers written: `university/_dossiers/F2-01` … `F2-08.dossier.html`; QA records: `university/qa/F2-01` … `F2-08.json`. The partial WIP edits of commit 90c4f8f (F2-01, F2-03, F2-07) were checked and finished; the F2-01 WIP text on the phases of translation used the numbering of a later draft and was corrected to C++20's nine phases.

**Opened (URLs in the dossiers):** the C++20 working draft N4861 (HTML rendering, about 60 sections) and the ISO catalogue page of ISO/IEC 14882:2020; the GCC 13.3.0 manual (3.2, 3.3, 3.5, 3.8, 3.12, 3.15, 3.16, 3.17, 3.19.54, 9) and "The C Preprocessor" (2.1, 2.3, 2.4, 2.5); GNU Binutils (nm, c++filt) and ld (Overview); elf(5), abort(3), signal(7); the Bash manual 3.7.5; the Itanium C++ ABI (3.1.2.5, 5.1.2, 5.1.5.2); the System V AMD64 psABI 1.0 (3.1.2, 3.2.3); C11 draft N1570 (7.2, 7.12.9.7, 7.22.1.4, 7.22.4.1); the C++ Core Guidelines (ES.23, ES.46, F.16, F.17, F.20, I.5, I.6); Goldberg's floating-point paper (Oracle reprint) and the IEEE 754-2019 catalogue page; the google/sanitizers AddressSanitizer wiki; the Unicode UTF FAQ; NIST SP 811 (4.3, 5.1.1, B.9); "A Tour of C++" (author's page, table of contents, sample chapter 12); CMU 15-213 Fall 2026 lecture slides (Bits, Bytes, and Integers; Machine-Level Programming I–IV; Linking), which follow CS:APP3e.

**Not openable:** the text of Levine's "Linkers and Loaders" (chapter pages return 404), CS:APP3e (not online), Tour of C++ chapters other than 12, The Practice of Programming chapter 6. These are kept as further reading only; every claim that rested on them was re-checked against the documents above.

**Corrected (main items):** F2-01 phases of translation (nine, linking in phase 9); F2-02 type-size box resolved, tolerance advice nuanced (Goldberg), curriculum quote; F2-03 continue/break as jumps; F2-04 overload ambiguity rule, reference passing per the Itanium ABI; F2-05 ASan "aborting" → exit code 1, vector capacity wording; F2-06 uninitialised members, alignment reasons; F2-07 inline "one copy" → same address, ODR "no diagnostic required", factors box resolved with NIST, cup decided, curriculum cross-reference; F2-08 SIGABRT/134 box resolved, unsourced "one comparison and one jump" removed, "water freezes/boils" replaced by the NIST formula's reference points. Per-chapter details are in the QA records.

**Left unverified (boxes kept):** F2-01 Lab — options of non-GCC compilers; F2-06 Hardware — whether some processors refuse misaligned accesses altogether (needs a named processor's architecture manual).

**Glossary:** `glossary.json` "precise" texts and source notes regenerated from the corrected jargon boxes (64 entries; 8 definitions changed).

**Labs:** the recorded toolchain (g++ 13.3.0) is not installed on the verification machine (g++ 14 and 15.2.0 only). A cross-check with `run_lab.sh` and g++ 15.2.0 gave the expected result for every listing in F2-01 … F2-08 (runner exit code 0 in all eight folders); the outputs differed only where the compiler version shows (header line counts, quoting style of messages, library paths), so the recorded files were restored (ruling A5). Status: lab not re-run with the recorded toolchain — toolchain missing.
