# SE302 — build notes (Testing strategy, F12-06 to F12-10)

Build date: 2026-10-10. Build machine: Linux x86_64 (cloud build container,
4 CPUs visible, shared with other jobs). Tool versions as printed in the lab
logs: g++ 13.3.0 (ASan, UBSan, TSan, `-fsanitize-coverage=trace-pc`),
aarch64-linux-gnu-g++ 13.3.0, clang++ 18.1.3 (libFuzzer runtime **not**
installed), LLD 18.1.3, QEMU 8.2.2 (qemu-system-x86_64 with TCG and SeaBIOS
1.16.3-2; qemu-aarch64 user mode), nvcc and cuobjdump from CUDA 12.0. No GPU,
no robot computer, no kit robot, no hosted CI service.

Every top-level `.cpp` in a lab folder is built and run by
`university/labs/run_lab.sh` (`-std=c++20 -Wall -Wextra -Wpedantic -Werror -g
-fsanitize=address,undefined`). Helper sources and deliberately buggy programs
are `.hpp`, `.hh` or `.cc` files, built by the lab's own `run.sh`. Each
`run.sh` step writes `<name>.out` (real output) and `<name>.log` (listing,
toolchain, command, date, machine, exit code, and a hardware line where
relevant). The last full re-run of all five folders finished with runner exit
status 0 for every folder.

## Files

- Chapters: `F12-06.html` … `F12-10.html`. Each has all 21 template sections,
  "Answers to Check yourself" with a forensic answer key
  (`<ID>-forensic-key`), a jargon box, a transition box, one inline SVG figure,
  claim tags, and at least one "Not verified" box. All five pass the fragment
  checks (html.parser balance, ids prefixed with the chapter id, no duplicate
  ids, no URLs, no `<script>`, section order, every claim tag resolves and every
  source is cited, every `data-src`/`data-run` file exists). Approximate word
  counts (text including listings' captions): F12-06 6,200; F12-07 6,700;
  F12-08 6,800; F12-09 7,600; F12-10 6,200. All are above the L3 (4,000) and
  L4 (5,000) targets; the owner may want them trimmed.
- `glossary.json`: 27 new entries from the jargon boxes. Terms already defined
  by other courses are linked, not redefined: Unit test, Regression test,
  Continuous integration (CI), Code coverage, Test isolation, Test harness,
  Bisect (SP202); Mutation testing, Test plan, Soak test (DR401); Fuzzing
  (SP301); Coverage-guided fuzzing, Corpus (SS302, F11-09: found during this
  build and removed from SE302's own list to avoid a duplicate);
  Hardware-in-the-loop (HIL) (RB403); Software in the loop (SITL), Lockstep
  simulation, Flaky test, Data age (DN401); Data race, Race condition,
  ThreadSanitizer (TSan), Condition variable, Cache coherence (SP203); Seed
  (KID102); Pseudo-random number generator (MA202); Multiboot, Serial port
  (UART) (OS302); GDB stub (QEMU) (OS301); Pipeline (shell), Exit status
  (OS304); Acceptance test (SE301); Reproducible build (RB403); Sim-to-real
  gap; Reference model; Sanitizer; AddressSanitizer (ASan); UndefinedBehavior-
  Sanitizer (UBSan). "Stub" (DS301) has the RPC meaning, so the testing meaning
  is entered as "Test double". Every `#gl-…` link in the five chapters
  resolves.
- Labs: `university/labs/F12-06` … `F12-10` (listed per chapter below).

## Listings run, per chapter

Status words: **pass** = exit status as expected for a correct program;
**expected-fail** = the program or step is meant to fail and did; **untested on
hardware** = ran only in an emulator or not at all on the device it is for.

### F12-06 The test pyramid and beyond (L3)
- `portfolio.cpp` — pass. Catch matrix of five planted mutants over four
  levels; deterministic.
- `suite_report.cpp` (forensic) — pass (the program reports the inherited
  suite's blind spot: M4 never killed, 24,001 units of work).

### F12-07 Simulation and hardware-in-the-loop tests (L3–L4)
- `sil_matrix.cpp` — pass (8 of 24 scenarios fail by design: latency ≥ 400 ms).
- `ci_sim.cpp`, `field_log.cpp` (forensic "Green CI, broken robot") — pass;
  `field_log` is the simulator standing in for a field log (stated in the
  chapter), not a real robot log.
- `fixed.cpp` — pass.
- `run.sh`: `pil_target` (readelf on the AArch64 controller), `pil_x86`,
  `pil_aarch64`, `pil_compare` (identical, 11 lines) — pass;
  `pil_aarch64` is **untested on hardware** (qemu-aarch64 user mode).

### F12-08 Property-based and fuzz testing (L3–L4)
- `pbt_rle.cpp` — pass (finds the planted uint8 wrap, shrinks 442 → 256 bytes).
- `pbt_ring.cpp` — pass (finds the planted ring-buffer bug; returns 0 when the
  bug is found).
- `run.sh`: `fuzz_blind` pass (no crash in 300,000); `fuzz_guided`
  **expected-fail** (ASan stack-buffer-overflow at `pkt.cc:32`, execution
  38745, deterministic); `regress_old` **expected-fail**; `regress_fixed`
  pass; `fuzz_fixed` pass; `no_libfuzzer` **expected-fail** (link error: the
  clang fuzzer runtime is not installed here).

### F12-09 Continuous integration for kernels, GPU code and robots (L4, course lab)
- `host_tests.cpp` — pass.
- `vadd.cu` (run by run_lab) — **expected-fail**, **untested on hardware**:
  "no CUDA-capable device is detected".
- `run.sh`: `ci` pass ("CI result: PASS with skipped stages: gpu-run");
  serial logs `serial_all`, `serial_panic`, `serial_hang`, `serial_none` (the
  last three are planted failures that the harness must and did report as
  FAIL); `ci_broken` (forensic: the old script reports PASS for a panicking
  kernel, exit 0 — the evidence of the bug); `forensic_fixed`
  **expected-fail** (exit 1, "FAIL (PANIC in the serial log; QEMU exit
  status 35)"). All kernel runs are **untested on hardware** (QEMU TCG only).

### F12-10 Flaky tests (L4)
- `flaky_suite.cpp` — pass (two tests FLAKY by design; the repaired test 0
  failures). The `async_wait_sleep` count varies between runs (103–105 of 200
  seen); the chapter quotes no exact count for it.
- `flake_math.cpp` — pass (deterministic table).
- `forensic_history.cpp` — pass (replays 400 CI runs; 3 failures with `-nan`).
- `forensic_fixed.cpp` — pass (0 failures).
- `run.sh`: `race_runs` — counting script, exit 0; the pass count of the racy
  binary swung between 0, 1 and 20 of 20 across lab runs on this shared
  machine; the chapter says so and quotes no single number as typical.
  `race_tsan` **expected-fail** (exit 66, data race reported at
  `race.cc:13`).

## Unverified boxes (what the Source Researcher must check)

- **F12-06:** attribution of the "test pyramid" name to Cohn, *Succeeding with
  Agile* (D3); size versus scope in *Software Engineering at Google* (D1);
  origin of mutation testing in DeMillo, Lipton and Sayward 1978 (D4); Myers
  for test levels (D5).
- **F12-07:** ROS 2, PX4, ArduPilot SITL/HITL tooling (no commands or
  parameter names given, by rule); delay and phase lag in Åström and Murray
  (D3); the C++ standard's specification of engines versus distributions (D5);
  QEMU user-mode documentation (D4). Warning box: the AArch64 PIL run is
  untested on hardware.
- **F12-08:** QuickCheck (D1), Miller et al. 1990 as the origin of fuzzing
  (D2), libFuzzer features (D4, not even linkable here), production fuzzers'
  edge coverage, comparison tracing and dictionaries (D4, D5); GCC
  instrumentation and the sanitizer interface (D3) verified only by behaviour.
  The claim that the uint8 wrap is defined behaviour (D7) rests on the
  standard's title only, though UBSan's silence in R1 is consistent with it.
- **F12-09:** configuration syntax of any CI service (not given); ROS 2 / PX4 CI
  tools (not given); Multiboot header values and the command-line field offset
  written from memory (D5; the run shows QEMU accepting them); the
  isa-debug-exit formula (v << 1) | 1 inferred from two observed values (D3);
  the 16550 line-status bit used by `putc` from memory (D6). Warning box:
  kernel runs untested on hardware; the CUDA program never ran on a GPU.
- **F12-10:** findings of Luo et al. 2014 on causes of flaky tests (D2, no
  figures quoted); large-company retry/quarantine practice (D1); retry and
  quarantine features of CI services (not given); that floating-point division
  by zero has its own GCC sanitizer option (from memory, D4; observed: UBSan
  silent on 0.0/0.0); TSan's line attribution at -O1 (our reading of R4).

## Decisions for the owner

1. **Word counts.** All chapters exceed the level targets by 25–50 %. The bulk
   is in the code walk-through tables and the forensic keys. Trim, or accept.
2. **Own fuzzer instead of libFuzzer.** clang's libFuzzer runtime is missing in
   the build container, so F12-08 uses a 140-line fuzzer built on GCC's
   `trace-pc` hook. If the dossier wants libFuzzer (or AFL++) taught by name,
   the container needs `libclang-rt-18-dev` (package name from memory) and a
   re-run; the chapter already explains how production fuzzers differ.
3. **CI service.** No hosted CI service was available or named; `ci.sh` is a
   plain script that any service can call. Decide whether the university
   standardises on one service and adds an unverified-then-verified appendix.
4. **Glossary overlaps.** "Coverage-guided fuzzing" and "Corpus" were defined
   by SS302 during this build; SE302 links to them. "Software in the loop
   (SITL)" (DN401) and "Hardware-in-the-loop (HIL)" (RB403) are the shared
   definitions; F12-07's jargon box shows them with the SIL/HITL synonyms but
   does not add glossary entries.
5. **Course-lab scope.** The card asks for "A CI pipeline running QEMU kernel
   tests with timeouts (curriculum 0)". F12-09's lab adds harness self-tests,
   a GPU build/skip stage and the robot matrix stage. Confirm this is wanted,
   or cut stages 5–7 for time.
6. **Exam P (write a test plan for a provided system).** No separate exam
   artefact was written. Each chapter's mini-project builds part of a test
   strategy; F12-10's mini-project combines them in the 5.5 format. The exam's
   "provided system" could be the F12-06/F12-07 robot (governor + parser +
   simulator) or the F12-09 test kernel; both have full sources in the labs.
7. **Story names** (Lina, Ahmed, Amira) are invented characters; no real people.

## Proposed F12 analogy mappings (building a house as a team)

| Concept | Analogy | Chapter |
| --- | --- | --- |
| Test levels (unit, integration, system) | Inspections at three scales: a brick, a wall, the whole house | F12-06 |
| Test double | A wooden frame standing in for the window while the wall is built | F12-06 |
| Simulation test | The test rig that pretends to be the house (fake rooms, fake weather) | F12-07 |
| Hardware-in-the-loop | The real boiler controller in its box, wired to that rig | F12-07 |
| Sensor latency missing from the simulator | The rig's thermometer reacts instantly; the real one sits behind a thick wall | F12-07 |
| Property-based testing | The rule "every door closes", tried on many doors from many angles | F12-08 |
| Shrinking | Finding the smallest push that still makes door 88 bounce | F12-08 |
| Fuzzing | The clumsy young visitor who leans on rails, slams cupboards, hangs on handles | F12-08 |
| Continuous integration | The site inspector who checks each day's work before the next trade may build on it | F12-09 |
| Harness self-test | The inspector testing her gauge on a pipe with a known leak | F12-09 |
| Skipped stage | "Lift not inspected: engineer absent", in red in the site book | F12-09 |
| Flaky test | A smoke alarm that sometimes beeps without smoke, and so gets ignored when there is some | F12-10 |
| Quarantine | Tape on the alarm: "under repair, warden checks the room by hand until Friday" | F12-10 |
