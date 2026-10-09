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
