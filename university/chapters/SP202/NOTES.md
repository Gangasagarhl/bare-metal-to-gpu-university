# SP202 — build notes (Tooling: build, debug, test, measure, F2-26 to F2-33)

Build date: 2026-10-09. Build machine: Linux x86_64 (cloud build container).
Tool versions as printed by the tools in the lab logs: g++ 13.3.0 (and g++-12
12.4.0 for the "works on my machine" forensic), clang++ 18.1.3, CMake/CTest
3.28.3, GNU Make, Ninja 1.11.1 (installed, not used in a chapter), GDB 15.1,
LLDB 18.1.3, Valgrind 3.22.0 (Memcheck, callgrind), GNU binutils 2.42 (`nm`,
`c++filt`, `readelf`, `gprof`), gcov from GCC 13.3.0, git 2.43.0.
`perf` does **not** work in the build container ("perf not found for kernel",
exit status 2; recorded as F2-33 `perf_attempt`).

Every top-level `.cpp` in a lab folder was built and run by
`university/labs/run_lab.sh` (`-std=c++20 -Wall -Wextra -Wpedantic -Werror -g
-fsanitize=address,undefined`). Deliberately buggy programs are `.cc` files
built inside the lab's `run.sh`, so the runner does not reject them. Each
`run.sh` step writes `<name>.out` (commands and output) and `<name>.log`
(listing, toolchain, command, date, machine, exit code, note) and marks
`result: UNEXPECTED` if the exit code differs from the expected one. The last
full re-run of all eight folders (F2-26 … F2-33) finished with runner exit
code 0 for every folder; no log contains "UNEXPECTED" or "BUILD FAILED".

## Files

- Chapters: `F2-26.html` … `F2-33.html`. Each has all 21 template sections,
  "Answers to Check yourself" and a forensic answer key (`<ID>-forensic-key`),
  a jargon box, a transition box, at least one inline SVG figure (F2-26 has
  two), claim tags on every factual statement, and an "unverified" box. All
  eight pass the fragment checks: html.parser balance, ids prefixed with the
  chapter id, no URLs, no `<script>`, all section ids in order, every claim tag
  resolves to a source, every `data-src`/`data-run` file exists.
- `glossary.json`: 59 entries taken from the chapters' jargon boxes. Terms
  already defined by other courses are linked, not redefined: Compiler,
  Compiler error, Warning, Debugger, Breakpoint, Exit code (KID102); Unit test,
  AddressSanitizer, Undefined behaviour, Signed overflow, Edge case, Assertion,
  Name mangling (SP101); Template, Instantiation, Dangling reference (SP102);
  Stack frame, Memory leak, UndefinedBehaviorSanitizer (UBSan) (SP201);
  Optimisation level, Assembly language, Vectorisation, Data race (HW202);
  Watchpoint (HW303). Every `#gl-…` link in the eight chapters resolves to one
  of these or to an SP202 entry.
- Labs: `university/labs/F2-26` … `F2-33`. F2-26 holds the university template
  repository (`template/`: CMake, `stats` library, CLI, `uni_test.h` harness,
  tests, `ci.sh`, `.gitignore`), a freestanding P1 object, and the "works on my
  machine" forensic. F2-30's `uni_test.h` is byte-for-byte the template's
  (checked by the `same_harness` run).

## Listings run (85 recorded runs)

| result | count | which |
|---|---|---|
| built and ran, exit code 0 | 70 | all ordinary listings, top-level `.cpp` files, and `run.sh` steps expected to succeed |
| expected compile failure (`.expect-fail`) | 2 | F2-32 `sort_list.cpp` (std::sort on std::list), F2-32 `orders.cpp` (copying a unique_ptr; forensic) |
| intentional non-zero exit, checked by `run.sh` | 13 | F2-26 wom_ci (2: `-Werror` build on g++ 13); F2-28 config_new (139, SIGSEGV), config_fixed (2, clear error); F2-29 overflow_asan, leak_asan, reserve_annotated, menu_asan, ub_strict, incompatible (1 each), menu_busy (139); F2-30 red, order_all (1: failing tests); F2-32 sort_clang (1) |

F2-31 `ci_fails` exits 0 as a step but records "ci.sh exit status: 8" inside
the transcript (the CI failure is the evidence). Some runs print sanitizer or
UBSan reports and still exit 0 by design: F2-29 `ub_ubsan` (UBSan recovers),
F2-29 `overflow_valgrind` (Memcheck reports, exit 0), F2-30 `probe`.

No GPU or other special hardware is involved. **Untested on hardware:** none
in the GPU sense. Untested items: the P1 cross compiler (`x86_64-elf`) — the
freestanding object in F2-26 was built with the host g++ and
`-ffreestanding -fno-exceptions -fno-rtti -nostdlib`, which is not a cross toolchain;
the P1 "second machine or container" clean-clone test (F2-31 cloned in the
same container); `perf` (unavailable).

### Outputs normalised by `run.sh` (so reruns are near byte-stable)

Paths of the temporary work folders are replaced with `./` or
`<work>/<lab>`; process ids with `<pid>`; GNU build ids removed; the one URL
printed by AddressSanitizer is replaced with
"<link removed: this university prints no address it has not opened (AH-30)>";
F2-29 trims repeated shadow-memory maps after the first. git runs fix author,
committer and dates (`Amara Okafor <amara@uni.invalid>`, 2026-10-0N) and use
an isolated `HOME`, so commit hashes are reproducible. Values that still change
between runs, by nature: ASan heap and stack addresses, the uninitialised value
printed by F2-27 `bugs_run`, CMake/CTest timing lines, F2-33 wall-clock times
and callgrind program totals (tens of instructions). Chapter prose quotes only
values that were identical across the two full runs made during the build
(callgrind per-function counts, queue totals, hashes, debugger garbage values
under GDB/LLDB) and describes timings qualitatively or as "in the run shown".

## Unverified boxes and open claims (per chapter)

All D-sources are cited **title only — not opened during this build** (dossier
gate G1 open). R-sources are this build's lab runs.

- **F2-26:** CMake behaviour beyond what `cmake --help-command` printed
  (R-sources quote the built-in help of CMake 3.28.3): other generators,
  presets, multi-config builds, per-compiler default flags. The forensic proves
  by experiment that the compiler version decides the outcome; *why* g++ 13's
  library headers no longer make `std::uint32_t` visible through `<string>` is
  not stated from a source. The P1 cross compiler is not built.
- **F2-27:** full meaning of warning groups beyond the one-line `--help=warnings`
  texts; the optimisation dependence of `-Wmaybe-uninitialized` (observed,
  explained from the GCC manual title); Clang's grouping of `-Wconversion`.
- **F2-28:** how breakpoints/watchpoints are implemented (debug registers,
  trap instructions), GDB disabling address randomisation, core files; LLDB
  `frame select`, `next`, `step`, `finish` not run.
- **F2-29:** ASan internals (check sequence, quarantine, 1:8 mapping as a design
  choice; the 8-bytes-per-shadow-byte rule itself is quoted from ASan's own
  legend), sanitizer/Valgrind slowdowns (not measured), LeakSanitizer default
  on other platforms, 4 KiB pages; TSan not run (only the ASan+TSan refusal).
- **F2-30:** TDD description (Beck, title only); GoogleTest/Catch2 not
  installed; branch coverage not measured.
- **F2-31:** bisect exit status 125 = skip (path not exercised); P1 second
  machine; SHA-256 object format not used.
- **F2-32:** instruction semantics and the System V calling convention (Intel
  SDM / ABI titles); C++ rounding rule; Compiler Explorer (a website, named
  only, not opened); the signed-division rounding argument is checked by hand,
  not by a run.
- **F2-33:** the cause of the gprof-versus-callgrind disagreement (shared
  library time) is from the gprof manual title, not tested (e.g. by static
  linking); perf not runnable; no hardware counters read.

## Decisions for the owner

1. **perf.** The curriculum's natural Linux profiler cannot run in the build
   container. F2-33 teaches callgrind and gprof with real runs and presents
   perf as "try it on your own machine". If a perf-capable runner becomes
   available, add a `perf stat`/`perf record` step to F2-33's `run.sh`.
2. **gprof disagrees with callgrind** on `hotspot.cc` (gprof ranks the sort and
   `make_lines` high, `parse_line` low; callgrind: `parse_line` 60.82 %). The
   chapter keeps the disagreement as a teaching point with an unverified box.
   Option: add a static-link gprof run to confirm the explanation.
3. **P1 acceptance** ("clean clone builds on a second machine or container") is
   only half shown: same container. Needs a second runner.
4. **Cross compiler for P1** is not built here (no `x86_64-elf` toolchain in the
   container); F2-26 uses host g++ with freestanding flags and says so.
5. **Prerequisite links.** Chapters link F1-31 (reading assembly), F1-23–F1-25,
   F1-30 (multicore, for data races/TSan), F2-23 (object lifetime and UB),
   F0-36 (reading compiler errors) and MA102. Please confirm these are the
   intended cross-references; F1-30 is used as the home of data races because
   no SP203 chapter existed when this was written (F2-35 "Data races" is
   SP203's; the owner may prefer to retarget TSan pointers to F2-35).
6. **Word counts.** Chapters run about 4,100–5,900 words including tables,
   transcripts excluded; the brief's ~3,000 words of prose for L2 is exceeded
   mainly by the jargon boxes, tables and the forensic answer keys. Trim if the
   owner wants a strict limit.
7. **Forensic names** (Diego, Priya, Tomás, Lena, Amara, Wei, Rosa) are
   invented characters, not real people.

## Proposed analogy mappings (world F2: the restaurant kitchen at work)

Registered mappings used: compiler = translator; program = recipe being cooked.
New mappings proposed by this course (please register or replace):

| concept | proposed mapping | chapter |
|---|---|---|
| build system | the kitchen's prep plan on the wall (what depends on what, what to redo) | F2-26 |
| continuous integration | the same checklist run for every dish before it leaves the kitchen | F2-26 |
| compiler warning | the translator's margin note | F2-27 |
| debugger | pausing (freezing) the kitchen to inspect each station | F2-28 |
| sanitizer | a food-safety inspector who watches every move during a practice service | F2-29 |
| unit test | tasting each component on its own before plating; test isolation = a clean spoon for every tasting | F2-30 |
| version control | the recipe book's dated history where no page is ever erased | F2-31 |
| reading compiler output | reading the translator's version and her long notes | F2-32 |
| profiler | a stopwatch and a tally sheet at every station | F2-33 |

Each chapter's "Where the analogy breaks" section lists the limits of its
mapping.
