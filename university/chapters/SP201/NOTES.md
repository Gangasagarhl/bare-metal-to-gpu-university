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
