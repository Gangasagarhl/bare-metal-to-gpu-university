# SP201 — build notes (Memory and pointers, F2-18 to F2-25)

Build date: 2026-10-09. Build machine: Linux x86_64 (cloud build container, 4 CPUs).
Toolchain as printed by the tools: `g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0`.
Cross toolchains (F2-24 only): `aarch64-linux-gnu-g++` and `riscv64-linux-gnu-g++` 13.3.0,
run under `qemu-aarch64` / `qemu-riscv64` 8.2.2 in user mode.

Every `.cpp` listing was built and run by `university/labs/run_lab.sh` with
`-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined`.
Extra builds live in each lab's `run.sh` (F2-18, F2-20, F2-22, F2-23, F2-24, F2-25) and write
`<name>.out` / `<name>.log` in the runner's format. The final re-run of all eight folders
ended with runner exit code 0 for every folder, and `python3 university/build/build.py`
reported no PROBLEM line for F2-18 … F2-25 (the 29 PROBLEM lines it printed are broken
`#gl-` links in other courses' chapters).

## Files

- **Chapters:** `F2-18.html` … `F2-25.html`. Each has all 21 template sections plus
  "Answers to Check yourself" and a forensic answer key, a jargon box, a transition box,
  claim tags resolving to its own source list, at least one inline SVG figure (two in F2-22),
  and at least one "Not verified" box.
- **Fragment checks** (own script, outside the repo): html.parser balance, ids prefixed with
  the chapter id, no duplicate ids, no URLs, no `<script>`, section order, jargon/transition
  boxes present, every claim tag resolving, every `data-src` / `data-run` file existing.
- **Word counts:** about 5,000–5,700 words per chapter including tables, code walk-through
  tables, lab steps and answers. See decision 1.
- **`glossary.json`:** 61 entries (term, simple, analogy, precise, source, related, chapters,
  anchor), taken from the jargon boxes plus "AddressSanitizer (ASan)". Every `#gl-` link in
  the eight chapters resolves. Entries that reuse the exact term of an existing entry, so the
  build merges the chapter lists and keeps the earlier course's definition: Address (KID101),
  Address space (HW204), Stack frame (HW202), sizeof, Alignment, Padding (SP101),
  Bounds check, Off-by-one error (MA101/KID101), std::array (KID102).
  "Undefined behaviour (UB)" is deliberately a separate term from SP102's "Undefined behaviour".
- **Labs:** `university/labs/F2-18` … `F2-25`.

## Listings run

Legend: pass = built and ran, exit code recorded; report = a sanitizer report was the expected
result (exit code 1, or 0 for UBSan, which continues); expected-fail = must not compile and did
not; emulated = cross-built and run under QEMU user mode, untested on real hardware.

| Chapter | Pass | Report (intended) | Expected-fail | Other |
|---|---|---|---|---|
| F2-18 | boxes, bytes_of_int, worked, addr_lab, backwards, addr_lab_plain (run.sh, -O0 no sanitizer) | — | — | — |
| F2-19 | pointers, swap, find_fixed, ptr_lab, ptr_lab_answer | find_null (UBSan + ASan SEGV), past_end (stack-buffer-overflow) | wild (-Werror=uninitialized) | — |
| F2-20 | frames, heap_lifetime, heap_lifetime_void, frames_plain, stack_limit (run.sh) | scope_bug (stack-use-after-scope), deep (stack-overflow) | — | deep_trimmed |
| F2-21 | new_delete, leak_fixed, asan_lab_fixed | leak, leak_q5, exception_leak (LSan), double_delete, asan_lab (heap-use-after-free) | mismatch (-Wmismatched-new-delete) | — |
| F2-22 | unique, shared, shared_q5, cycle_fixed, list | cycle (LSan), deep_list (stack-overflow in destructor chain) | unique_copy | cycle_trimmed, deep_list_trimmed |
| F2-23 | lifetime, ub_fixed, moving_fixed | overflow (UBSan, exit 0), view_dangle (heap-use-after-free), moving_asan | max_dangle (-Wdangling-reference) | moving_release and moving_debug: exit 139 (SIGSEGV), intended evidence; moving_*_build: compiler messages (-Wuse-after-free); view_dangle_trimmed |
| F2-24 | layout, bytes_dump, traits, layout_lab, header_bug, header_fixed | misaligned (UBSan, exit 0) | — | emulated: layout_aarch64, layout_riscv64, layout_lab_aarch64, layout_lab_riscv64 |
| F2-25 | arrays, checked, bump, freelist_reference, off_by_one_plain (run.sh, -O2, 3 runs) | stack_overrun (UBSan + stack-buffer-overflow), off_by_one (heap-buffer-overflow) | decay (-Wsizeof-array-argument) | stack_overrun_trimmed, off_by_one_trimmed |

Nothing in SP201 needs special hardware; "untested on hardware" applies only to the four
emulated F2-24 runs (their logs say so in a `hardware:` line).

## Not verified (boxes in the chapters)

- F2-18: 64-bit Windows `long` is 4 bytes (LLP64) — no Windows toolchain. ASan's fake stack
  as the cause of the 0x7f… local addresses — documentation not checked.
- F2-19: pointer provenance and integer-to-pointer conversion rules.
- F2-20: default `std::thread` stack size on Linux.
- F2-21: Linux overcommit and the OOM killer.
- F2-22: size of a `unique_ptr` with a function-pointer deleter (not measured).
- F2-23: implicit object creation (C++20) and `std::start_lifetime_as` (C++23) details;
  glibc reusing the just-freed block for the next similar-size request — this is the
  inference behind the forensic lab's explanation of why the crash moves.
- F2-24: `alignof(std::max_align_t) == 16` and which processors trap on misaligned loads;
  whether Listing 4's misaligned read was one load instruction (disassembly not inspected).
- F2-25: Ubuntu GCC default hardening (stack protector, `_GLIBCXX_ASSERTIONS`); `<span>`
  in freestanding implementations; what lies after a 16-byte glibc block and the -O2 loop
  code in the "Right by luck" forensic lab.
- All D-sources (C++ working draft, CS:APP, C++ Core Guidelines, GCC manual, sanitizer
  documentation, psABI documents) are cited by title only: dossier gate G1 open.
- A few "Check yourself" answers are rule-based predictions that were not run, and say so:
  F2-24 Q3 (it was checked with a throwaway build outside the repo, which matched: 6/2 and
  4/2, but no run record exists), F2-25 Q4 and Q6, and F2-25 lab step 5.

## Analogy mappings proposed (world F2, restaurant kitchen)

Registered and used: pointer = note with a shelf number; RAII = tool wall; main memory = pantry.
Proposed for registration:

- byte = one numbered shelf slot; address = the slot number (F2-18).
- stack = the stack of prep trays: a new tray for each task, cleared when the task ends (F2-20).
- heap = shelves lent out on request, returned only when someone says so (F2-20, F2-21).
- static storage = permanent shelves stocked before opening (F2-20).
- new/delete = borrowing/returning a lent shelf; leak = never returned; double free =
  returning the same shelf twice; use after free = writing on a shelf already returned (F2-21).
- unique_ptr = a borrowed tool with exactly one name tag, moved by handing over the tag;
  shared_ptr = a sign-out sheet, last to sign out returns it; weak_ptr = reading the sheet
  without signing it (F2-22).
- object lifetime = from stocking a shelf to clearing it; undefined behaviour = an order the
  rule book has no page for (F2-23).
- alignment = crates may start only at slots that are multiples of their size; padding =
  empty slots left so the next crate lines up (F2-24).
- array = a run of slots with no fence at the end; span = a note with a first slot number AND
  a count; bump allocator = counter space handed out left to right, wiped at closing (F2-25).

## Decisions for the owner

1. **Length.** Chapters run about 5,000–5,700 words including tables and answers, above the
   ~3,000-word L2 prose target. The extra is mostly code-walk tables, sanitizer outputs and
   answer keys. Trimming would cut those first.
2. **Trimmed outputs (AH-25).** Long sanitizer reports are shown trimmed, produced by
   `run.sh` from the full `.out`, with every cut marked "[... N line(s) trimmed ...]". The
   trimmed record's log says "exit code: 0 (of the trimming step …)" and points to the
   program's own log. Files: deep_trimmed (F2-20), cycle_trimmed, deep_list_trimmed (F2-22),
   view_dangle_trimmed (F2-23), stack_overrun_trimmed, off_by_one_trimmed (F2-25).
3. **Builds without `-Werror` or sanitizers in run.sh.** Forensic evidence needs "release"
   builds: F2-23 `moving_crash.cc` is built at -O2 without `-Werror` (with it, GCC's
   -Wuse-after-free stops the build — that warning is itself shown as evidence); F2-18
   `addr_lab_plain`, F2-20 `frames_plain`, F2-25 `off_by_one_plain` are built without
   sanitizers. `moving_crash.cc` uses the `.cc` extension so run_lab.sh does not compile it
   with the course flags.
4. **Crashing runs recorded as evidence.** moving_release / moving_debug exit 139 (SIGSEGV)
   by design; their logs say so.
5. **Grader-only reference.** `labs/F2-25/freelist_reference.cpp` (first-fit free list with
   headers, split and coalesce, 20,000-step randomised test) is the course project's reference
   solution. It is built and run by run_lab.sh but not shown on the page; F2-25 only mentions
   that it exists. Move it elsewhere if graders' material must not sit in the public lab
   folder. Likewise `ptr_lab_answer.cpp` (F2-19), `asan_lab_fixed.cpp` (F2-21), the list
   reference (F2-22, shown in the answers), `ub_fixed.cpp` (F2-23), `header_fixed.cpp` (F2-24).
6. **Emulated architectures.** F2-24 shows AArch64 and RISC-V 64 layouts from QEMU user mode,
   labelled "untested on real hardware". They show the compilers' ABI choices only.
7. **Course forensic "The crash that moves"** is in F2-23 (lifetime and UB) rather than F2-21,
   because it needs the UB framing; F2-21's forensic is a leak instead.
8. **Exam P (fix three memory bugs from sanitizer output)** is not written as an exam paper;
   the F2-21, F2-23 and F2-25 labs are framed as practice for it. An exam author can reuse
   asan_lab, view_dangle and off_by_one with their reports.
9. **Course project** brief and 20-point rubric are in F2-25 ("Course project: two allocators
   with tests"); the mini-project there (placement-new `make<T>`) builds toward it.

## Owner rulings applied

1. **Length** — ruling **A1**: accepted as written; nothing trimmed for length. The verification
   pass removed only an unsourced hedge ("cache lines of, typically, 64 bytes", F2-24).
2. **Trimmed outputs (AH-25)** — decided by verifier: kept. Every cut is marked
   "[... N line(s) trimmed ...]", the full `.out` stays in the lab folder, and the trimmed
   record's log names the program's own log (consistent with A5/A6).
3. **Builds without `-Werror` or sanitizers in run.sh** — decided by verifier: kept; each such
   build is named in the chapter and in its run record (R-entries), and is the forensic evidence.
4. **Crashing runs recorded as evidence** (moving_release / moving_debug, exit 139) — ruling
   **A6** (a lab output that is honest evidence may stay); kept.
5. **Grader-only references in the public lab folders** — ruling **B7** covers practical exams
   (`labs/<C>-P/`), not these lab reference solutions. Decided by verifier: they stay where they
   are; the chapters already say they exist and that they are reference solutions. If an exam
   author reuses one as a practical's hidden evidence, it moves to `labs/<C>-P/` under B7.
6. **Emulated architectures (F2-24)** — rulings **C3**/**C4**: published as "untested on real
   hardware"; the logs carry a `hardware:` line and the chapter says what a real test needs
   (an AArch64 or RISC-V 64 Linux machine running the same two programs).
7. **Course forensic "The crash that moves" in F2-23** — decided by verifier: stays in F2-23
   (it needs the UB framing).
8. **Exam P** — ruling **B4**: the exam papers are written by the exam pass (EXAM_BRIEF), not in
   the chapters; the F2-21, F2-23 and F2-25 labs stay framed as practice.
9. **Course project brief and 20-point rubric (F2-25)** — ruling **B3**: the handbook's proposed
   split is approved as written.
10. **Analogy mappings** (list above) — ruling **A3**: approved and registered.
11. **Teaching stand-ins** — ruling **A2**: SP201 uses none (the bump and free-list allocators are
    exercises, not stand-ins for a named tool).
12. **Licence** — ruling **A10**: no `LicenseRef-Uni-Lab` placeholder exists in the SP201 lab
    files; nothing to change.
13. **Committed test keys** — ruling **A8**: none in SP201.
14. **Recorded runs** — ruling **A5**: the recorded `.out`/`.log` files of 2026-10-09 are kept
    (see "Verification pass" for why the labs could not be re-run with the recorded toolchain).
15. **Status** — ruling **C4**: F2-18 … F2-23 and F2-25 are "internally checked"; F2-24 is
    "internally checked · hardware steps untested" (emulated AArch64/RISC-V runs).

## Verification pass

Third session, 2026-10-25, with full web access (WebFetch and curl). Dossiers written for all
eight chapters (`university/_dossiers/F2-18.dossier.html` … `F2-25.dossier.html`), QA records in
`university/qa/F2-18.json` … `F2-25.json`.

**Opened:** the C++20 working draft N4861 (2020-04-01, open-std.org PDF) and the C++23 draft N4950
(2023-05-10); the C11 draft N1570 (7.20.1.1); the x86-64 psABI Version 1.0 (March 12, 2025); the
AAPCS64 (release 2025Q4) and the RISC-V ABIs Specification (calling conventions, 2.0-rc0 dev);
Intel SDM Volume 1 (253665-093US, September 2026); the C++ Core Guidelines (dated Jun 14, 2026);
the GCC 13.3.0 manual (Warning, C++ Dialect, Optimize, Instrumentation options; type attributes;
structure-layout pragmas; other built-ins); GCC 13.3.0 source files (libsanitizer
`asan_flags.inc`, libstdc++ `shared_ptr_base.h`, `unique_ptr.h`); Clang's AddressSanitizer,
LeakSanitizer and UndefinedBehaviorSanitizer pages; the sanitizers wiki (AddressSanitizerAlgorithm,
Flags, UseAfterReturn, LeakSanitizer design document); the ASan paper (USENIX ATC 2012, PDF);
OSTEP chapters 13, 14 and 17; glibc's MallocInternals; Linux kernel "Overcommit Accounting";
pthread_create(3) (man-pages 6.19); Microsoft "Data Type Ranges". CS:APP (third edition) and
A Tour of C++ (third edition): only the authors' tables of contents were opened; claims tagged to
them are checked against the section titles and, where possible, backed by a second opened source.
**Not opened:** Ubuntu's "ToolChain/CompilerFlags" wiki page (HTTP 404 twice).

**Corrected (main items):** a C++ byte holds the basic execution character set and the 8-bit
UTF-8 code units (F2-18); two wrong address prefixes in prose (F2-18 0x7ffe→0x7fff, F2-20
0x7ffd→0x7ffe); `Loose order{}` is aggregate initialisation, so zero padding is observed, not
guaranteed (F2-24 prose, table, answer and the comment in `labs/F2-24/bytes_dump.cpp`); the
shared_ptr count: the standard requires freedom from data races, the atomic operations are GCC's
library (F2-22); "same machine code as manual delete" replaced by what the GCC source shows
(F2-22); unsourced hedges and numbers removed or sourced (F2-24 max_align_t, cache line, two
accesses, packed members, memcpy built-in); the F2-23 figure no longer states the inferred
block reuse as fact; tags moved to the right source where CS:APP did not cover the claim
(F2-23 Heisenbug, F2-25 bump allocator → own runs).

**Boxes resolved and removed:** Windows `long` (F2-18), ASan fake stack default (F2-18; conflict
between the GCC 13.3 manual and its runtime source recorded in the dossier), Linux overcommit
(F2-21), implicit object creation / `std::start_lifetime_as` (F2-23).
**Boxes kept with updated reasons:** pointer provenance (F2-19), std::thread stack size (F2-20,
pthread rule now stated), unique_ptr size with a function-pointer deleter (F2-22), glibc reuse of
the freed block (F2-23), misaligned-load traps and the single-load question (F2-24), Ubuntu
hardening defaults, `<span>` in freestanding (C++20/23 part resolved) and the bytes after the
16-byte block (F2-25).

**Labs:** the recorded toolchain (g++ 13.3.0, aarch64/riscv64 cross compilers, qemu-user) is not
installed on the verification machine (Ubuntu 25.10 with g++ 15.2.0 only). The recorded files are
kept unchanged (A5). As a scratch check outside the repository, all eight lab folders were run with
g++ 15.2 by `run_lab.sh`: every exit code matched the record and the outputs differed only in
addresses, process ids, sanitizer-runtime line numbers and the compiler's quote characters, except
(a) F2-24's cross builds (no cross compiler: not run) and (b) F2-25 `off_by_one`, which on Ubuntu
25.10 stopped on a libstdc++ assertion (exit 134) before AddressSanitizer saw the read — evidence
for the hardening box, not a lab failure. The only lab source change (a comment in
`bytes_dump.cpp`) was re-run in scratch with g++ 15.2 and printed output identical to the record.
