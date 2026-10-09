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
