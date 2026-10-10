# KID102 — build notes (My first C++ programs, F0-29 to F0-38)

Build date: 2026-10-09. Build machine: Linux x86_64.
Toolchain as printed by the tools: `g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0`,
`GNU gdb (Ubuntu 15.1-1ubuntu1~24.04.1) 15.1`.
Every listing was built and run with `university/labs/run_lab.sh` (flags
`-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined`).
The last full re-run of all ten folders (and of `F0-37/run_gdb.sh`,
`F0-29/build_by_hand.sh`, `F0-36/help_flags.sh`) finished with runner exit code 0
for every folder; the gdb transcripts were byte-identical to the ones embedded
in F0-37.

## Files

- Chapters: `F0-29.html` … `F0-38.html` (all 20 sections + "Answers to Check
  yourself", jargon box and transition box; one inline SVG figure each).
- `glossary.json`: 59 entries (term, simple, analogy, precise, source,
  related, chapters, anchor `gl-…`).
- Labs: `university/labs/F0-29` … `F0-38`, each with a `README.md` listing every
  file, its role and the expected result.

## Listings run (78 programs)

| result | count | which |
|---|---|---|
| built and ran, exit code 0 | 55 | all ordinary listings |
| expected compile failure (`.expect-fail`) | 16 | F0-30 empty_jar, const_change; F0-31 join_bug; F0-32 assign_bug, no_brackets; F0-34 no_return, order_bug; F0-35 int_counter, array_push, no_vector_include; F0-36 missing_semicolon, misspelled, wrong_type, unused, cascade, missing_brace |
| intentional exit code 134 (uncaught `std::out_of_range`) | 4 | F0-35 out_of_range, out_of_range_flush; F0-37 steps, trace |
| intentional exit code 1 (test function reports failure) | 2 | F0-34 cups_broken; F0-38 game_test_break |
| intentional exit code 124 (`.timeout`, 2 s) | 1 | F0-33 never_ends |

No listing failed unexpectedly.

Also run: `F0-29/build_by_hand.sh` (→ `build_by_hand.txt`, BuildID replaced by
`<hash removed>`), `F0-36/help_flags.sh` (→ `help_flags.txt`),
`F0-37/run_gdb.sh` (→ `walk_gdb.txt`, `steps_gdb.txt`, `cooking_time_gdb.txt`,
`gdb_help.txt`; process numbers replaced by `<pid>`).
`F0-37/cooking_time_gdb_first_try.txt` is kept as evidence of the first attempt
(LeakSanitizer fatal error under ptrace); it is no longer regenerated.

## Outputs that change between runs

- `F0-38/random_secret.out`, line 2 (five secrets from `std::random_device`):
  different on every run by design. The chapter quotes no number from it.
- `F0-33/never_ends.out`: the number of lines depends on how many passes fit
  into the 2 s limit. The chapter relies only on the pattern and exit code 124.

## "Not verified" boxes

1. F0-29 (Lab): toolchain installation steps — only a machine with g++ already
   installed was used.
2. F0-31 (Layer 3): sizes of `int`/`double` on other machines — check the
   fundamental types clause of ISO/IEC 14882.
3. F0-37 (hardware): how gdb implements breakpoints on this machine — check the
   GDB manual.
4. F0-38 (Layer 3): whether another standard library gives secret 22 for seed
   2026 — check the random number clause of ISO/IEC 14882.
5. F0-38 (hardware): where `std::random_device` gets its values on Linux.

## Claims that rest only on title-only sources (gate G1)

All D sources (Stroustrup "A Tour of C++", GCC manual, GDB manual,
ISO/IEC 14882, Petzold "Code") are cited by title only and carry the note
"Title only — not opened during this build; the Source Researcher must confirm
the edition and section (dossier gate G1 open)". Claims tagged only with D
sources that the Source Researcher should check first:

- F0-37 hardware section: breakpoints as an inserted special instruction; `-g`
  as the map from machine code to lines; `backtrace` (its help text was not
  saved, so it rests on D2).
- F0-38 Layer 3: `mt19937` is the Mersenne Twister, deterministic, not suitable
  for security.
- F0-38 hardware: `std::random_device` asks the system for hard-to-predict
  values (behaviour seen in R3; the mechanism is unverified, box 5).

## Decisions for the owner

- **Word counts.** The validator counts all prose outside `pre`, `code`,
  tables and SVG. By that count F0-29, F0-30, F0-31, F0-34, F0-35, F0-36, F0-37
  and F0-38 are 3,038–3,640 words. Without the jargon box, Sources and Answers,
  every chapter is 2,000–2,700 words. If the 3,000-word limit includes Sources
  and Answers, F0-37 and F0-38 need trimming of about 500–650 words.
- **Fixed seed in F0-38.** The game uses seed 2026 so that the recorded run
  (guesses from `game.in`) can be repeated; the chapter explains this and shows
  `std::random_device` for real play.
- **gdb and LeakSanitizer.** Inside gdb the sessions set
  `ASAN_OPTIONS=detect_leaks=0`; normal runs keep every check. The chapter and
  `F0-37/README.md` explain why.
- **Course project and exam.** F0-38's Mini-project holds the KID102 project
  brief (≥ 3 functions + a test function that runs first) and the practical exam
  practice (times table). Rubric weights are proposals.

## Notes on tooling

- `run_lab.sh` writes `-o name` in each `.log`, while the binary it actually
  builds is `.bin_name` (and removes it). The chapters quote the logged command.
- Chapters were assembled from drafts with helper scripts kept outside the
  repository (`/home/claude/.kid102-work`: macros for line-by-line tables,
  verbatim file embedding, glossary links, jargon boxes; a validator for tag
  balance, id prefixes, no URLs, no scripts, section order, allowed SVG classes
  and banned phrases). The built fragments in this folder are the source of
  truth; the helper scripts are not needed to read or publish them.

## Owner rulings applied

- **Word counts (F0-29 … F0-38 above the level's target)** — ruling **A1**: accepted as written; nothing
  was trimmed for length. The verification pass only reworded or removed text that was wrong or
  unsupported (listed under "Verification pass").
- **Fixed seed 2026 in F0-38** — decided by verifier: keep the fixed seed for the recorded run (in the
  spirit of **A5**, recorded runs must be repeatable) and keep `std::random_device` for real play. The
  C++ working draft N4861 confirms the reason the chapter gives: `mt19937` is fully specified
  (§26.6.5), but the mapping to 1–100 is not (§26.6.2.6, §26.6.8.2.1), so the promise stays "same seed,
  same secret, same toolchain".
- **gdb and LeakSanitizer (`ASAN_OPTIONS=detect_leaks=0` inside gdb only)** — decided by verifier: keep.
  The switch is documented by the sanitizers project (F0-37 D6); normal runs keep every check.
- **Course project and practical-exam practice in F0-38; rubric weights** — ruling **B3**: the proposed
  rubric splits are approved as written. Ruling **B4** (quizzes, midterm, final, practical) is carried
  out by the exam pass (`build/verify/EXAM_BRIEF.md`), not by this verification unit; nothing changed here.
- **Exercise values** (rubric points, "100" and "1000" in the guessing game) — ruling **A4**: approved as
  exercise values; none could be mistaken for a real product's value.
- **A8 (teaching keys)** — not applicable: KID102 commits no keys.
- **A10 (licence)** — no `LicenseRef-Uni-Lab` placeholder exists in the KID102 lab files; nothing to change.
- **C1 / C2 (sources)** — applied: every cited document was opened or, for the two books, located on the
  author's or publisher's page (edition and table of contents confirmed); new sources added under C2:
  the C++20 working draft N4861 (with the ISO/IEC 14882:2020 catalogue page), the GCC 13.3.0 and GDB
  manuals, Linux man-pages (elf(5), execve(2), ptrace(2), random(4)), the RISC-V RV32I specification page,
  the Mersenne Twister home page and the sanitizers project's LeakSanitizer page.
- **C3 (untested on hardware)** — KID102 has no hardware steps: every lab runs on the build computer.
  The one remaining environment item (toolchain installation on the learner's own computer, F0-29)
  stays in a "Not verified" box that now says what is needed to check it.
- **C4 (status)** — with gates G1–G8 passed in this pass, the ten chapters are "internally checked".
- **D1 (reference kit)** — `build/KIT.md` did not exist during this pass; KID102 needs only a computer with
  a C++ toolchain, so no kit item is named.

## Verification pass

Date 2026-10-10, Fact-Checker agent. Per chapter: a source dossier in
`university/_dossiers/<ID>.dossier.html` (documents table with the URLs opened, fact list keyed to every
tagged claim, "not found" list, toolchain) and a QA record in `university/qa/<ID>.json`.

**Opened.** "A Tour of C++" Third edition (author's page, table of contents, free sample chapter 12
"Containers"); Petzold "Code" 2nd Edition (publisher's page with table of contents, companion site);
ISO/IEC JTC1/SC22/WG21 N4861 (C++20 working draft, about 45 sections) and the ISO catalogue page of
ISO/IEC 14882:2020; "Using the GNU Compiler Collection (GCC)" for GCC 13.3.0 (sections 3.2, 3.4, 3.7,
3.8, 3.10, 3.12); "Debugging with GDB" Tenth Edition (current online version 19.0.50; sections 4.1,
4.4, 5.1.1, 5.2, 8.2, 17.1); Linux man-pages 6.19 elf(5), execve(2), ptrace(2), random(4); RISC-V
RV32I page (v20260120, §1.1.5); the Mersenne Twister home page; the sanitizers wiki
(AddressSanitizerLeakSanitizer, AddressSanitizerFlags).

**Sources sections.** All "Title only — not opened" notes were replaced by the edition and sections as
printed. Claims that were tagged only with the two books (whose chapter text could not be opened) are now
tagged with the opened tier-1/tier-2 sources; the books stay listed as further reading, except
"A Tour of C++" §12.2/§12.2.2, which was opened and is cited in F0-35.

**Corrected** (details in each `qa/<ID>.json`):
- F0-29: "each statement ends with a semicolon" narrowed to "each of these statements"; the compile
  stages now mention assembling (GCC manual §3.2: four stages).
- F0-30: an uninitialised `int` "inside a function" has an indeterminate value (N4861 §6.7.4).
- F0-31: removed the unsupported "because it stores numbers in binary"; the "Not verified" box on type
  sizes became a note confirmed by N4861 §6.8.1 (int at least 16 bits, rest implementation-defined).
- F0-32, F0-33: hardware sections no longer claim a separate "compare instruction" on the build
  machine; general "compare, then maybe jump", with RISC-V branches as the named example.
- F0-34: non-void function without return is undefined behaviour, which g++ warns about and `-Werror`
  turns into an error (N4861 §8.7.3, GCC §3.8); arguments are passed according to a calling convention
  (no unsupported register/memory detail).
- F0-35: `std::size_t` described as in N4861 §17.2.4 (the claim that it "matches the type that size()
  returns" was removed); uncaught exception → `std::terminate` → `abort()` added with sources.
- F0-36: removed the unsourced linker message "undefined reference"; "suggestions are guesses based on
  spelling" → "suggestions are only guesses"; message-format claims re-tagged to GCC §3.7 (they were
  tagged with the Tour).
- F0-37: the "Not verified" box on breakpoints became a note confirmed by the GDB manual §5.1.1; the
  ptrace sentence now quotes ptrace(2).
- F0-38: the box "seed 2026 gives 22 elsewhere?" became a note answered from N4861 (engine fixed,
  distribution not); `std::random_device` description corrected (the standard allows a fallback engine).
- Figures: F0-30, F0-32, F0-37 text alternatives corrected to match the drawn labels.
- `glossary.json`: sources rewritten from the jargon tags; two "precise" texts synced with the chapters.

**Left unverified** (still in "Not verified" boxes): F0-29 toolchain installation on the learner's
computer; F0-38 where g++ 13.3.0's `std::random_device` gets its values on Linux.

**Labs.** All ten folders re-run with `run_lab.sh` (plus `build_by_hand.sh`, `help_flags.sh`,
`run_gdb.sh`, run with bash because they are not marked executable): runner exit code 0 for every
folder; all expected-fail, exit-134, exit-1 and exit-124 results reproduced. Only the dates in the
`.log` files and the `random_secret.out` second line (random by design) differed, so the recorded files
were restored (ruling A5). No lab code changed.
