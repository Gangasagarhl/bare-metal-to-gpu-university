# SS302 — build notes (Memory safety and secure coding, F11-07 to F11-12)

Build date: 2026-10-10. Build machine: Linux x86_64 (cloud build container).
Toolchain as printed by the tools in the lab logs: `g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0`,
`clang-tidy` 18.1.3, GNU Binutils `readelf` 2.42. Host-only course: no GPU, no QEMU, no
hardware. All six lab folders pass `university/labs/run_lab.sh university/labs/<ID>`
(status 0) in their last run. Absolute paths are stripped from `.out` files.

Analogy world: F11 — a castle with gates and guards (registered mapping, guide 8.1:
"memory safety = writing only on your own page of the notebook").

## Files

- Chapters `F11-07.html` … `F11-12.html`: all 21 template sections plus "Answers to
  Check yourself" with a forensic answer key woven into the forensic section's method;
  jargon box; transition box; one inline SVG figure each; claim tags; line-by-line tables
  for the main listing of each chapter. All six pass the fragment checks (html.parser
  balance, ids prefixed with the chapter id, section ids present and in order, no URLs,
  no `<script>`, every `data-src`/`data-run` file exists).
- `glossary.json`: 28 entries (four-part). Eight terms already defined by earlier courses
  are **reused** (linked, not redefined) so no term is defined twice: `Buffer overflow`,
  `Use after free` (SP201), `std::span` (SP201), `Fuzzing` (SP301), `Sanitizer` (SP202),
  `SMEP`, `SMAP` (OS303), `IOMMU` (HW204). Terms that exist elsewhere with a different
  scope are given an explicit `(SS302)` qualifier and a distinct anchor: `Lifetime (SS302)`,
  `Ownership (SS302)`, `Dangling pointer (SS302)`, `Descriptor (SS302)`, `DMA (SS302)`.
- Labs: `university/labs/F11-07` … `F11-12`.

## Decisions for the owner

- **Course forensic "the crash input"** is placed in **F11-09** (fuzzing): a fuzzer-found
  input + sanitizer report, minimised and fixed. Each other chapter also has its own
  forensic lab from a real run, as the template requires.
- **The lab fuzzer is the university's own** small coverage-guided mutation fuzzer
  (`fuzz.cc` + `cov.cc`): AFL-style shared-memory edge bitmap across `fork()`, targets
  built with `-fsanitize-coverage=trace-pc`, fork-per-input so a crash kills only the
  child. No external fuzzing library (libFuzzer's runtime was not linkable in the
  container — see below). This is our code and we ran it; it is the honest teaching
  version of libFuzzer/AFL++, and the chapters say so and point to the real tools.
- **MAVLink and USB details are deliberately NOT reproduced faithfully.** The MAVLink
  frame scanner uses a toy checksum, not the real CRC-16 + CRC_EXTRA; the USB descriptor
  parser uses a simplified layout. Both carry unverified boxes naming the MAVLink
  Developer Guide and the USB specification. The taught bug (trusting a wire/device length)
  is independent of these details. A Source Researcher should confirm the real framing
  before any of this is used against real vehicles or devices.
- **SMEP/SMAP** are kernel features already built and tested under QEMU in **OS303
  (F3-29)**. F11-10 cross-references that evidence and marks them untested-in-this-lab
  rather than duplicating a kernel boot in this host-only course.

## Listings and runs (recorded steps per chapter; all exit as expected)

| chapter | steps (run_lab.sh) | result |
|---|---|---|
| F11-07 | safe (0, ALL TESTS PASSED); adjacent_plain (admin granted by 20-byte name); adjacent_asan (UBSan: index 16 out of bounds); hijack_plain (handler hijacked to grant_root); intoverflow_asan (ASan heap-buffer-overflow WRITE) | pass |
| F11-08 | safe (0); dangling (ASan heap-use-after-free, reallocation); useafterfree (ASan heap-use-after-free, unique_ptr reset) | pass |
| F11-09 | elf_tests (0); mav_tests (0); elf_fuzz (crash @10 execs, 203 B); elf_minimise (203→168 B, still crashes); elf_report (ASan heap-buffer-overflow READ elf_target.cc:85); elf_fixed_fuzz (~16k execs, edges 46→51, no crash); mav_fuzz (crash @3 execs, 12 B); mav_minimise (12→2 B); mav_report (ASan heap-buffer-overflow READ toy_sum); mav_fixed_fuzz (~18k execs, no crash) | pass |
| F11-10 | canary_on (stack smashing detected, exit 134); canary_off (SIGSEGV 139); nx (SIGSEGV 139, NX enforced); aslr (addresses differ across 3 runs); checksec (lab reader + readelf 2.42 agree: weak = exec-stack/no-PIE, hard = non-exec-stack/PIE/RELRO) | pass |
| F11-11 | good (0); tidy_bad (4 clang-tidy findings); tidy_good (0 findings); narrowing (builds plain → prints 4464; -Wconversion -Werror → build fails) | pass |
| F11-12 | desc_tests (0, ALL TESTS PASSED); buggy_evil (ASan heap-buffer-overflow READ parse_config); fixed_evil (rejected, fail closed, exit 1); fixed_good (accepted, 2 descriptors) | pass |

No log contains "BUILD FAILED", "UNEXPECTED" or "STEP FAILED".

### Expected non-zero exits (by design)

These demonstrate the bug/defence and are recorded honestly in the `.log`:
- F11-07 adjacent_asan (UBSan, 1), intoverflow_asan (ASan, 99); F11-08 dangling, useafterfree
  (ASan, 99); F11-09 *_report (ASan, 99); F11-10 canary_on (134), canary_off (139), nx (139);
  F11-11 narrowing (second build fails by design); F11-12 buggy_evil (ASan, 99), fixed_evil
  (1, malformed input rejected).

### Untested on hardware / in this build

- **SMEP, SMAP and user-pointer validation** (B12): demonstrated in OS303 F3-29 under QEMU,
  not re-run here. F11-10 cites that evidence and has an unverified box for the exact Intel
  SDM CR4 bit numbers (SMEP 20, SMAP 21) and the XD bit (63).
- **ASLR** depends on the host kernel's `randomize_va_space`; the three lab runs differed in
  this container, but F11-10 has an unverified box and never prints a fixed address.
- **No GPU/QEMU** used; this is a host C++ course.

## Per-chapter unverified boxes (listed for the QA record)

Every D-source is "title only — not opened during this build" (dossier gate G1 open);
claims that rest on runs cite R-sources. Explicit `co unverified` boxes in the chapters:

- **F11-09**: the MAVLink frame framing (start byte 0xFE, field order) is at the lab's level
  only; the real CRC-16 and per-message CRC_EXTRA are NOT reproduced (toy checksum). Check
  the MAVLink Developer Guide.
- **F11-10**: ASLR depends on the host kernel setting; and the Intel SDM CR4 bit numbers
  (SMEP 20, SMAP 21) and XD bit (63) are cited by title and must be confirmed.
- **F11-12**: the USB configuration-descriptor layout (field offsets, type codes, nested
  interface/endpoint descriptors) is simplified; confirm against the USB specification
  (and OS305 F3-34) before parsing real USB data.

No other unverified boxes. No numbers about real hardware are stated except those printed by
the lab's own runs (sanitizer exit codes, address differences, byte counts), which are cited
as R-sources.

## Claims written from memory (flagged)

- The x86-64 no-execute bit position (63), EFER.NXE, and CR4 bits 20/21 (SMEP/SMAP) are from
  memory of the Intel SDM; stated in F11-10 with an unverified box (D2/D5).
- CERT rule id `ERR34-C` (atoi) is taken from the actual clang-tidy output (R1), not memory;
  other CERT rule names are described, not quoted with ids.
- The MAVLink v1 start byte 0xFE is stated from memory as the lab's chosen marker and is
  inside the unverified box's scope.
