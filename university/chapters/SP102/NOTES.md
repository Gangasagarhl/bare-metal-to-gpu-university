# SP102 — build notes (C++ II: classes, RAII and the standard library, F2-09 to F2-17)

Build date: 2026-10-09. Build machine: Linux x86_64 (cloud build container).
Toolchain as printed by the tools: `g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0`,
GNU binutils (`nm`, `objdump`).

Every `.cpp` listing was built and run with `university/labs/run_lab.sh`. The flags
were `-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined`.
The extra steps live in each lab's `run.sh` (F2-10, F2-14, F2-16, F2-17):

- disassembly and symbol listings;
- one C++23 build;
- one `-fno-exceptions` build.

These steps write `<name>.out` and `<name>.log` in the runner's format. The last
full re-run of all nine folders finished with runner exit code 0 for every folder.

## Files

- **Chapters:** `F2-09.html` … `F2-17.html`. Each chapter has:
  - all 21 template sections, plus "Answers to Check yourself" and a forensic answer key;
  - a jargon box and a transition box;
  - one or two inline SVG figures (one each in F2-10 and F2-13, two in the others).
- **Fragment checks:** every chapter passes html.parser balance, ids prefixed with the
  chapter id, no URLs, no `<script>`, section ids in template order, every claim tag
  resolving to a source, and every `data-src`/`data-run` file existing.
- **Word counts:** about 3,700–4,600 words of prose per chapter, about 5,600–6,600 in total.
  See decision 1.
- **`glossary.json`:** 66 entries. Each has term, simple, analogy, precise, source,
  related, chapters and anchor `gl-…`. The entries come from the chapters' jargon boxes,
  and every `#gl-` link in the nine chapters resolves.
  - Ten entries use the exact term of an existing entry, so the build merges the chapter
    lists and keeps the first course's definition: Reference, Pass by value, Pass by
    reference, Const reference, Undefined behaviour, static_assert (SP101); const, Test
    function (KID102); Invariant (MA102); Ring buffer (HW204).
  - "Algorithm" in F2-15 is filed as "Standard algorithm", because KID101 already defines
    "Algorithm" in its general sense.
- **Labs:** `university/labs/F2-09` … `F2-17`. Each has a `README.md` listing file, role
  in the chapter and expected result.

## Listings run (82 recorded runs)

| result | count | which |
|---|---|---|
| built and ran, exit code 0 | 55 | all ordinary listings, lab reference solutions, forensic evidence, `run.sh` steps |
| expected compile failure | 15 | F2-09 stock_private, strong_types; F2-10 init_order, ticket_copy_refused; F2-12 const_error, return_local, bind_temporary; F2-13 file_copy, self_move; F2-14 deduce_fail, no_less, no_less_concept, ring_zero; F2-16 nodiscard, no_exceptions (`-fno-exceptions`) |
| intentional exit code 1: lab sabotage caught by a test | 8 | F2-09 table_lab_broken; F2-11 file_lab_sabotage; F2-12 ref_lab_byvalue; F2-13 copy_lab_forgot_move; F2-14 ring_test_sabotage; F2-15 stats_lab_byref; F2-16 error_lab_norange; F2-17 lambda_lab_byvalue |
| intentional exit code 1: AddressSanitizer report | 2 | F2-12 dangling (heap-use-after-free), F2-17 dangling_capture (stack-use-after-return) |
| intentional exit code 1: program refuses by design | 1 | F2-16 bill_fixed (forensic answer key) |
| intentional exit code 134 (uncaught exception) | 1 | F2-16 uncaught |

Total: 55 + 15 + 8 + 2 + 1 + 1 = 82.

No run failed unexpectedly. No log contains "UNEXPECTED" or "BUILD FAILED". No
GPU or other hardware is involved, so nothing in this course is "untested on hardware".

### Outputs that change between runs

- F2-12 `dangling.out` and F2-17 `dangling_capture.out`: the AddressSanitizer
  process id (`==NNNN==`), addresses and BuildId change on every run. The chapters
  quote only the kind of error, the frames' file:line and the variable names.
- F2-15 `iter_inside.out`, last line ("list neighbours 8 bytes apart? 0"): this
  depends on where the allocator puts the list nodes. It was 0 on every run here.
- F2-11 `leak.out`, `raii_file.out`, `std_raii.out`: the baseline of 4 open files
  (stdin, stdout, stderr plus the directory listing used to count them) depends on
  how the runner starts the program.
- F2-11 `lock_bug.out` lists `/proc/self/fd` (Linux only). The process id in the
  folder name is replaced by "(the folder listing used by this function)".

### Runs that needed a workaround

- F2-12 dangling: ASan did not report a dangling `std::string&`, because libstdc++'s
  string code is not instrumented. The evidence therefore reads an `int` member of a
  struct inside the vector. `std::cout << std::unitbuf` keeps the lines printed before
  the abort.
- F2-11 leak_until_fail: the sanitizer runtime itself fails once file descriptors run
  out. The program therefore lowers the limit with `setrlimit` to 32, keeps the leaked
  descriptors in a vector, closes them all and only then prints. The chapter says so in
  a note.
- F2-13 copy_lab: a direct `x = std::move(x)` is rejected by `-Werror=self-move`. The
  self-move test goes through a reference instead (Listing 7 shows the direct form
  failing).
- F2-10 shift_lab: the order of evaluation of `a() + b()` is unspecified, so the lab
  splits it into two statements, keeping the recorded output deterministic.
- F2-16: the `.cc` files (`expected23.cc` with `-std=c++23`, `no_exceptions.cc`
  with `-fno-exceptions`) are built only by `run.sh`, because `run_lab.sh` builds
  `*.cpp` with the C++20 course flags.
- F2-15 remove_bug: GCC 13.3 / libstdc++ gave **no** warning for ignoring
  `std::remove`'s result, even under `-Werror`. The chapter states this as observed.

## "Not verified" boxes (13)

| chapter | what is not verified | what would verify it |
|---|---|---|
| F2-09 | how `this` is passed (register) under the System V AMD64 psABI / Itanium C++ ABI | SP301 F2-47 reading the ABI documents |
| F2-10 | role of `_Unwind_Resume` | Itanium C++ ABI, exception-handling chapter |
| F2-11 | `flock` semantics (lock owned by the open file description; second open gets a separate lock) and non-Linux behaviour | `flock(2)` man page; runs on another OS |
| F2-12 | vector growth factor (2 → 4 seen) is a library choice | standard [vector.capacity]; libstdc++ source |
| F2-12 | ASan shadow memory / redzone description | GCC and AddressSanitizer documentation |
| F2-13 | growth sequence 1, 2, 4, …; the wording of the `move_if_noexcept` rule | standard; libstdc++ source |
| F2-14 | whether `% N` becomes a mask for constant N | generated assembly (F2-32) |
| F2-14 | `nm` letter W and merging of identical instantiations | GNU Binutils docs; ELF spec (F2-43, F2-44) |
| F2-15 | vector iterator = one pointer; list node layout (inferred from sizes) | libstdc++ headers |
| F2-16 | roles of `__cxa_allocate_exception`, `__cxa_throw`, `__cxa_free_exception`, `_Unwind_Resume`; "zero cost on the normal path" | Itanium C++ ABI, exception-handling chapter |
| F2-16 | exit code 134 = 128 + SIGABRT (6) | Bash manual; `signal(7)` |
| F2-17 | `std::function` small-buffer size and inlining at -O2 | measurement (F2-33, SP302) |
| F2-17 | how a by-reference capture is stored; lambda name numbering `{lambda(...)#n}` | standard (unspecified); Itanium C++ ABI mangling |

Every book and standard is cited by title only (dossier gate G1 open):

- D1: Stroustrup, "A Tour of C++".
- D2: ISO/IEC 14882.
- D3: C++ Core Guidelines.
- Also cited: the GCC manual, Linux man-pages, GNU Binutils, the Itanium C++ ABI, and in
  F2-14 Cormen et al. "Introduction to Algorithms" for array-based queues.

The Systems Curriculum is cited as a map only (guide AH-4).

## Analogy mappings proposed (F2 — the restaurant building and its kitchen)

Already registered and used as registered:

- RAII = the tool wall (every tool is returned to its hook).
- Pointer = a note with a shelf number (mentioned only).

New mappings, proposed for the analogy register:

| concept | mapping | chapter |
|---|---|---|
| class / object | a kitchen station with its rule card; the counter is the private part only the station's cook reaches behind | F2-09 |
| constructor / destructor | the station's opening and closing checklists | F2-10 |
| reference | "the pot on burner two" (a second name for the same pot); const reference = may taste, may not add; dangling reference = the pot was moved | F2-12 |
| copy / move | photocopy of a recipe card / handing over the original card | F2-13 |
| template | a recipe card with a blank ("stir-fry with ______") | F2-14 |
| ring buffer | the order rail: a loop of clips, oldest ticket first | F2-14 |
| iterator / range | a finger moving along the spice shelf; the painted line after the last jar | F2-15 |
| algorithm | a page in the kitchen's procedure book | F2-15 |
| error code / optional / exception | a note handed back / a plate that may come back empty / the alarm to the manager | F2-16 |
| lambda / capture | a sticky-note instruction; a number copied onto the note versus "the board by the door" | F2-17 |

"The painted line after the last jar" agrees with SP201 F2-19's glossary entry for the
one-past-the-end pointer ("the line painted after the last slot").

## Curriculum mapping

The course maps to Curriculum 1.1, language topics, first part. Section 1.1 lists topics
only and has **no milestone acceptance tests**, so no chapter claims to satisfy a
milestone. Two curriculum items are previewed explicitly and quoted as a map only:

- F2-14 uses "compile-time techniques useful for drivers" (static assertions, constexpr).
- F2-16 uses section 1.2's "No exceptions, no RTTI by default", backed by the
  `-fno-exceptions` run.

## Decisions for the owner

1. **Length.** The L2 target is about 3,000 words of prose, but the chapters run about
   3,700–4,600. The extra length is mostly the hardware sections (disassembly and symbol
   listings), the sabotage runs and the forensic answer keys. Options: accept the length,
   or move the line-by-line tables to a collapsible part.
2. **Exam P preview (F2-17).** The format is invented from the course card: a short
   specification, write the class, defend the invariant. It needs confirming by whoever
   writes the exams.
3. **Course project.** The ring buffer is the default, and the course card allows a
   matrix instead. It is built up chapter by chapter in F2-14 to F2-17: `snapshot`,
   `peek` as `std::optional`, `[[nodiscard]]`, and `for_each_oldest_first`. The final
   brief is in F2-17.
4. **C++23 content.** `std::expected` (F2-16 Listing 5) is outside the course's C++20
   flags. It is shown via `run.sh` with `-std=c++23` as Layer 3 material. Keep it or drop it.
5. **Analogy register.** Please accept or reject the mappings above.
6. **F2-17 lab step 5** (capture `limit` by reference in the factory) is offered as an
   optional exercise and was **not run**. The chapter says so.

## Owner rulings applied

Rulings from `university/OWNER_RULINGS.md`, applied in the verification pass of 2026-10-25.

1. **Length (decision 1)** — ruling **A1**: the chapters' length (about 3,700–4,600 words of
   prose each) is accepted as written. Nothing was trimmed for length; only wrong or unsourced
   sentences were changed.
2. **Exam P preview in F2-17 (decision 2)** — ruling **B4**: the course's practical is written
   with the exams of this pass, with a reference solution that is actually run, and ruling **B7**
   puts it in `labs/<C>-P/`. The F2-17 preview ("a short specification, write the class, defend
   the invariant") matches the practical described by B4 and the course card; it stays as a preview.
3. **Course project (decision 3)** — ruling **B3** (card weights fixed; default course-project
   rubric is guide 11.5). Decided by verifier: the ring buffer stays the default project, built up
   in F2-14 to F2-17; the matrix alternative allowed by the course card stays allowed.
4. **C++23 content (decision 4)** — decided by verifier: keep `std::expected` (F2-16 Listing 5)
   as Layer 3 material, built only by `run.sh` with `-std=c++23` and clearly labelled C++23. Its
   description is now checked against the C++23 working draft N4950, 22.8 "Expected objects".
5. **Analogy register (decision 5)** — ruling **A3**: all proposed F2 mappings (station, checklists,
   pot on burner two, photocopy vs original card, recipe card with a blank, order rail, finger on
   the spice shelf, procedure book, note / empty plate / alarm, sticky note) are approved and
   registered; each chapter keeps its "Where the analogy breaks" section.
6. **F2-17 lab step 5 not run (decision 6)** — decided by verifier: it stays an optional exercise.
   The chapter already says "Step 5 was not run in this build"; it is not a listing presented as
   tested (AH-26), so no change is needed.
7. **Ruling A10 (licence)** — no `LicenseRef-Uni-Lab` placeholder exists in `labs/F2-09` …
   `labs/F2-17`; nothing to change.
8. **Ruling C1 / C2 (sources)** — every cited source was opened or recorded as not opened (see
   below). New sources added under C2: the Linux kernel "Circular Buffers" document (F2-14 D6),
   the libstdc++ 13.3.0 sources (tier 4), the Itanium C++ ABI and the x86-64 psABI, the Bash manual,
   Stroustrup's paper "Exception Safety: Concepts and Techniques" (F2-16 D8).
9. **Ruling C3 (untested on hardware)** — not applicable: no step in SP102 uses hardware beyond the
   host computer; every QA record has an empty `hardware_untested` list.
10. **Ruling A5 (recorded runs)** — the recorded `.out`/`.log` files are kept (see "Verification
    pass" for why the labs could not be re-run with the recorded compiler).

## Verification pass

Date 2026-10-25 (third pass, full web access). Dossiers: `university/_dossiers/F2-09…F2-17.dossier.html`;
QA records: `university/qa/F2-09…F2-17.json`. Every URL opened is listed in the dossiers as plain text.

**Opened (tier and version as shown):**

- ISO C++ working draft N4861 (C++20; HTML rendering of 2020-04-03), about 90 section pages, plus the
  ISO catalogue page for ISO/IEC 14882:2020 (edition 6, withdrawn, replaced by 14882:2024); N4659
  [dcl.init] (C++17) and N4950 [expected] (C++23) for two specific claims.
- C++ Core Guidelines (page dated Jun 14, 2026), rules cited per chapter.
- GCC 13.3.0 manual: 3.5, 3.8, 3.12, 3.17.
- Linux man-pages 6.19: open(2), close(2), write(2), read(2), flock(2), getrlimit(2), _exit(2),
  syscalls(2), syscall(2), stdin(3), abort(3), proc_pid_fd(5), proc(5), signal(7).
- Itanium C++ ABI (3.1.2.1, 5.1.8) and its exception-handling chapter ($Revision: 1.22 $);
  x86-64 psABI (LaTeX sources, 3.2.3); GNU Binutils "nm"; generic ABI "Section Groups".
- libstdc++ sources at `releases/gcc-13.3.0`: stl_vector.h, vector.tcc, stl_iterator.h, stl_list.h,
  stl_algo.h (also at 15.2.0), std_function.h.
- AddressSanitizer wiki pages (AddressSanitizer, AddressSanitizerAlgorithm,
  AddressSanitizerUseAfterReturn) and the Clang AddressSanitizer page; Linux kernel "Circular
  Buffers"; Bash manual 3.7.5; Stroustrup "Exception Safety: Concepts and Techniques".
- "A Tour of C++", Third edition: author's page, table of contents and the free sample chapter 12
  only. No claim rests on the unopened chapters; every former D1 tag was re-anchored to the
  standard or the Guidelines (F2-15 keeps D1 for the sample chapter's advice on vector and list).

**Not opened:** Cormen et al., "Introduction to Algorithms" (MIT Press page: HTTP 403 twice; F2-14
D3 kept as further reading, no claim rests on it); Arm developer documentation on caches (HTTP 403);
Ubuntu wiki "ToolChain/CompilerFlags" (HTTP 404).

**Main corrections** (details in each QA record):

- F2-09: constructor definition made precise; `this`-passing box resolved (ABI first parameter, `rdi`).
- F2-10: globals are not always constructed before `main` (implementation may defer); implicit
  `noexcept` of destructors qualified; `_Unwind_Resume` box resolved.
- F2-11: `flock` box resolved; "cannot be returned" made precise (`return f;`).
- F2-12: vector-growth and AddressSanitizer boxes resolved (standard + libstdc++ source + ASan docs).
- F2-13: `move_if_noexcept` box resolved; sink-parameter advice no longer misattributed to the Guidelines.
- F2-14: template checking limited to dependent parts; ring-buffer definition neutral about head/tail
  naming; W/COMDAT explanation corrected; CLRS replaced by the kernel document.
- F2-15: cache claim re-sourced; "undefined" for a bad comparator softened to what the standard says;
  iterator-layout box resolved; wrong "Listing 5" reference fixed; new note: libstdc++ 15.2.0 marks
  `std::remove` `[[nodiscard]]` (13.3.0 does not).
- F2-16: `[[nodiscard]]` warning is recommended practice, not required; `__cxa_*` and exit-code-134
  boxes resolved; unsourced generalisations removed; stack protector stated as observed.
- F2-17: explicit-capture advice labelled as the course's; `std::function` storage rule read from
  libstdc++; lambda-numbering claim corrected (the ABI leaves TU-local closures to the compiler).

**Left unverified (7 boxes):** F2-11 non-Linux file locking; F2-14 `% N` as a mask, COMDAT use by
g++ 13.3; F2-16 cost of a throw and table size; F2-17 `std::function` inlining at -O2, g++ lambda
numbering as shown by the demangler. (The 13-row table above is the state before this pass.)

**Labs:** g++ 13.3.0 (the recorded toolchain) is not installed on the verification machine (only
g++ 14 and 15.2.0), so the labs were not re-run with `run_lab.sh` in the repository and the recorded
files are kept (QA records: "lab not re-run: toolchain missing"). For information, a scratch copy of
all nine folders was built and run with g++ 15.2.0: every listing behaved as recorded (same exit codes,
same expected compile failures, same sanitizer reports) except F2-15 `remove_bug.cpp`, which no longer
compiles because libstdc++ 15 marks `std::remove` `[[nodiscard]]`; F2-15 now says so.

**Glossary:** the `source` field of every entry now names the opened document and section; the
definitions of Constructor, Ring buffer and [[nodiscard]] follow the corrected chapter text.
